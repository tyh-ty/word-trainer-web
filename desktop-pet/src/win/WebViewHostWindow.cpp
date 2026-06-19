#include "WebViewHostWindow.h"

#include "../util/Logger.h"
#include "../util/StringUtils.h"

#include <WebView2.h>
#include <boost/json.hpp>
#include <shlwapi.h>
#include <atomic>
#include <sstream>

namespace {

template <typename T>
void safeRelease(T*& value) {
    if (value) {
        value->Release();
        value = nullptr;
    }
}

std::wstring fileUriFromPath(const std::wstring& path) {
    wchar_t buffer[4096];
    DWORD length = static_cast<DWORD>(std::size(buffer));
    if (SUCCEEDED(UrlCreateFromPathW(path.c_str(), buffer, &length, 0))) {
        return std::wstring(buffer, length);
    }

    std::wstring normalized = path;
    for (wchar_t& ch : normalized) {
        if (ch == L'\\') ch = L'/';
    }
    return L"file:///" + normalized;
}

std::string jsonString(const boost::json::object& obj, const char* name) {
    if (auto* value = obj.if_contains(name); value && value->is_string()) {
        return std::string(value->as_string());
    }
    return {};
}

class EnvironmentCompletedHandler
    : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {
public:
    explicit EnvironmentCompletedHandler(WebViewHostWindow* owner) : m_owner(owner) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER;
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)) {
            *object = static_cast<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ref = --m_ref;
        if (ref == 0) delete this;
        return ref;
    }

    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* environment) override {
        if (m_owner) m_owner->onEnvironmentCreated(result, environment);
        return S_OK;
    }

private:
    std::atomic<ULONG> m_ref{1};
    WebViewHostWindow* m_owner = nullptr;
};

class ControllerCompletedHandler
    : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {
public:
    explicit ControllerCompletedHandler(WebViewHostWindow* owner) : m_owner(owner) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER;
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)) {
            *object = static_cast<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ref = --m_ref;
        if (ref == 0) delete this;
        return ref;
    }

    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override {
        if (m_owner) m_owner->onControllerCreated(result, controller);
        return S_OK;
    }

private:
    std::atomic<ULONG> m_ref{1};
    WebViewHostWindow* m_owner = nullptr;
};

class WebMessageReceivedHandler : public ICoreWebView2WebMessageReceivedEventHandler {
public:
    explicit WebMessageReceivedHandler(WebViewHostWindow* owner) : m_owner(owner) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER;
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_ICoreWebView2WebMessageReceivedEventHandler)) {
            *object = static_cast<ICoreWebView2WebMessageReceivedEventHandler*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ref = --m_ref;
        if (ref == 0) delete this;
        return ref;
    }

    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) override {
        if (m_owner) m_owner->onWebMessage(args);
        return S_OK;
    }

private:
    std::atomic<ULONG> m_ref{1};
    WebViewHostWindow* m_owner = nullptr;
};

}  // namespace

WebViewHostWindow::WebViewHostWindow(HINSTANCE hInstance)
    : m_hInstance(hInstance) {}

WebViewHostWindow::~WebViewHostWindow() {
    destroy();
}

bool WebViewHostWindow::create(const std::wstring& title,
                               const std::string& webPath,
                               const std::string& userDataDir) {
    m_title = title;
    m_webPath = StringUtils::utf8ToWide(webPath);
    m_userDataDir = StringUtils::utf8ToWide(userDataDir);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WebViewHostWindow::WndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"WordTrainerWebViewHost";
    if (!RegisterClassW(&wc)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            LOG_ERROR("Failed to register WebView host window class: %lu", error);
            return false;
        }
    }

    m_hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        m_title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1180, 760,
        nullptr, nullptr,
        m_hInstance,
        this);

    if (!m_hWnd) {
        LOG_ERROR("Failed to create WebView host window: %lu", GetLastError());
        return false;
    }

    initializeWebView();
    return true;
}

void WebViewHostWindow::show() {
    if (!m_hWnd) return;
    ShowWindow(m_hWnd, SW_SHOWNORMAL);
    UpdateWindow(m_hWnd);
}

void WebViewHostWindow::destroy() {
    safeRelease(m_webView);
    safeRelease(m_controller);
    safeRelease(m_environment);

    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
    if (m_loader) {
        FreeLibrary(m_loader);
        m_loader = nullptr;
    }
}

LRESULT CALLBACK WebViewHostWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    WebViewHostWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = reinterpret_cast<WebViewHostWindow*>(cs->lpCreateParams);
        if (self) self->m_hWnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<WebViewHostWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) return self->handleMessage(msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT WebViewHostWindow::handleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE:
        resizeWebView();
        return 0;
    case WM_DESTROY:
        safeRelease(m_webView);
        safeRelease(m_controller);
        safeRelease(m_environment);
        return 0;
    default:
        return DefWindowProcW(m_hWnd, msg, wp, lp);
    }
}

void WebViewHostWindow::initializeWebView() {
    m_loader = LoadLibraryW(L"WebView2Loader.dll");
    if (!m_loader) {
        LOG_ERROR("WebView2Loader.dll not found");
        return;
    }

    using CreateEnvironmentFn = HRESULT (STDAPICALLTYPE*)(
        PCWSTR,
        PCWSTR,
        ICoreWebView2EnvironmentOptions*,
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);

    auto createEnvironment = reinterpret_cast<CreateEnvironmentFn>(
        GetProcAddress(m_loader, "CreateCoreWebView2EnvironmentWithOptions"));
    if (!createEnvironment) {
        LOG_ERROR("CreateCoreWebView2EnvironmentWithOptions not found");
        return;
    }

    HRESULT hr = createEnvironment(
        nullptr,
        m_userDataDir.empty() ? nullptr : m_userDataDir.c_str(),
        nullptr,
        new EnvironmentCompletedHandler(this));
    if (FAILED(hr)) {
        LOG_ERROR("CreateCoreWebView2EnvironmentWithOptions failed: 0x%08X", hr);
    }
}

void WebViewHostWindow::onEnvironmentCreated(HRESULT result,
                                             ICoreWebView2Environment* environment) {
    if (FAILED(result) || !environment) {
        LOG_ERROR("WebView2 environment creation failed: 0x%08X", result);
        return;
    }

    safeRelease(m_environment);
    m_environment = environment;
    m_environment->AddRef();

    HRESULT hr = m_environment->CreateCoreWebView2Controller(
        m_hWnd, new ControllerCompletedHandler(this));
    if (FAILED(hr)) {
        LOG_ERROR("CreateCoreWebView2Controller failed: 0x%08X", hr);
    }
}

void WebViewHostWindow::onControllerCreated(HRESULT result,
                                            ICoreWebView2Controller* controller) {
    if (FAILED(result) || !controller) {
        LOG_ERROR("WebView2 controller creation failed: 0x%08X", result);
        return;
    }

    safeRelease(m_controller);
    m_controller = controller;
    m_controller->AddRef();

    safeRelease(m_webView);
    HRESULT hr = m_controller->get_CoreWebView2(&m_webView);
    if (FAILED(hr) || !m_webView) {
        LOG_ERROR("get_CoreWebView2 failed: 0x%08X", hr);
        return;
    }

    EventRegistrationToken token{};
    m_webView->add_WebMessageReceived(new WebMessageReceivedHandler(this), &token);

    resizeWebView();
    navigateToWebPath();
    LOG_INFO("Embedded WebView window ready");
}

void WebViewHostWindow::resizeWebView() {
    if (!m_controller || !m_hWnd) return;
    RECT bounds{};
    GetClientRect(m_hWnd, &bounds);
    m_controller->put_Bounds(bounds);
}

void WebViewHostWindow::navigateToWebPath() {
    if (!m_webView || m_webPath.empty()) return;
    std::wstring uri = fileUriFromPath(m_webPath);
    HRESULT hr = m_webView->Navigate(uri.c_str());
    if (FAILED(hr)) {
        LOG_ERROR("WebView2 Navigate failed: 0x%08X", hr);
    }
}

void WebViewHostWindow::onWebMessage(ICoreWebView2WebMessageReceivedEventArgs* args) {
    if (!args) return;

    LPWSTR rawMessage = nullptr;
    HRESULT hr = args->TryGetWebMessageAsString(&rawMessage);
    if (FAILED(hr) || !rawMessage) {
        return;
    }

    std::string message = StringUtils::wideToUtf8(rawMessage);
    CoTaskMemFree(rawMessage);
    postJsonToPage(handleWebMessage(message));
}

void WebViewHostWindow::postJsonToPage(const std::string& json) {
    if (!m_webView || json.empty()) return;
    std::wstring wideJson = StringUtils::utf8ToWide(json);
    m_webView->PostWebMessageAsJson(wideJson.c_str());
}

std::string WebViewHostWindow::handleWebMessage(const std::string& message) {
    boost::json::object reply;
    reply["type"] = "pet-api-response";

    try {
        boost::json::object root = boost::json::parse(message).as_object();
        std::string id = jsonString(root, "id");
        std::string type = jsonString(root, "type");
        std::string method = jsonString(root, "method");
        std::string path = jsonString(root, "path");

        reply["id"] = id;

        if (type != "pet-api" || path.empty()) {
            reply["ok"] = false;
            reply["error"] = "bad bridge message";
            return boost::json::serialize(reply);
        }

        std::string body;
        if (auto* payload = root.if_contains("payload"); payload && !payload->is_null()) {
            body = boost::json::serialize(*payload);
        }

        if (method.empty()) {
            method = body.empty() ? "GET" : "POST";
        }

        std::string response = m_requestHandler
            ? m_requestHandler(method, path, body)
            : std::string(R"({"ok":false,"error":"native bridge unavailable"})");

        reply["ok"] = true;
        try {
            reply["data"] = boost::json::parse(response);
        } catch (...) {
            reply["data"] = response;
        }
    } catch (const std::exception& e) {
        reply["ok"] = false;
        reply["error"] = e.what();
    }

    return boost::json::serialize(reply);
}
