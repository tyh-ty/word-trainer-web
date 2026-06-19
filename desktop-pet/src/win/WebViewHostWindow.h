#pragma once

#include <Windows.h>
#include <functional>
#include <string>

struct ICoreWebView2;
struct ICoreWebView2Controller;
struct ICoreWebView2Environment;
struct ICoreWebView2WebMessageReceivedEventArgs;

class WebViewHostWindow {
public:
    using RequestHandler = std::function<std::string(
        const std::string& method,
        const std::string& path,
        const std::string& body)>;

    explicit WebViewHostWindow(HINSTANCE hInstance);
    ~WebViewHostWindow();

    bool create(const std::wstring& title,
                const std::string& webPath,
                const std::string& userDataDir);
    void show();
    void destroy();
    HWND getHwnd() const { return m_hWnd; }

    void setRequestHandler(RequestHandler handler) { m_requestHandler = std::move(handler); }

private:
    friend class EnvironmentCompletedHandler;
    friend class ControllerCompletedHandler;
    friend class WebMessageReceivedHandler;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT handleMessage(UINT msg, WPARAM wp, LPARAM lp);

    void initializeWebView();
    void resizeWebView();
    void navigateToWebPath();
    void postJsonToPage(const std::string& json);
    std::string handleWebMessage(const std::string& message);

public:
    void onEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment);
    void onControllerCreated(HRESULT result, ICoreWebView2Controller* controller);
    void onWebMessage(ICoreWebView2WebMessageReceivedEventArgs* args);

private:
    HINSTANCE m_hInstance = nullptr;
    HWND m_hWnd = nullptr;
    HMODULE m_loader = nullptr;
    ICoreWebView2Environment* m_environment = nullptr;
    ICoreWebView2Controller* m_controller = nullptr;
    ICoreWebView2* m_webView = nullptr;
    std::wstring m_title;
    std::wstring m_webPath;
    std::wstring m_userDataDir;
    RequestHandler m_requestHandler;
};
