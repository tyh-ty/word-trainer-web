#include "PetWindow.h"
#include "../util/Logger.h"
#include <utility>

PetWindow::PetWindow(HINSTANCE hInstance) : m_hInst(hInstance) {}

PetWindow::~PetWindow() { destroy(); }

bool PetWindow::create() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = PetWindow::WndProc;
    wc.hInstance = m_hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"DesktopPet";

    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        LOG_ERROR("RegisterClassExW failed: %lu", GetLastError());
        return false;
    }

    m_hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"DesktopPet", L"",
        WS_POPUP,
        0, 0, m_width, m_height,
        nullptr, nullptr, m_hInst, this
    );

    if (!m_hWnd) {
        LOG_ERROR("CreateWindowExW failed: %lu", GetLastError());
        return false;
    }

    if (!m_renderer.initialize(m_width, m_height)) {
        LOG_ERROR("D2DRenderer initialization failed");
        return false;
    }

    pinToBottomRight();
    LOG_INFO("PetWindow created %dx%d", m_width, m_height);
    return true;
}

void PetWindow::show() {
    ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);
}

void PetWindow::hide() {
    ShowWindow(m_hWnd, SW_HIDE);
}

void PetWindow::destroy() {
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

void PetWindow::pinToBottomRight(int margin) {
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = screenW - m_width - margin;
    int y = screenH - m_height - margin;
    moveTo(x, y);
}

void PetWindow::moveTo(int x, int y) {
    SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, m_width, m_height,
                 SWP_NOACTIVATE | SWP_NOZORDER);
}

void PetWindow::savePosition(const char* filePath) {
    RECT r;
    if (!GetWindowRect(m_hWnd, &r)) return;
    FILE* f = fopen(filePath, "w");
    if (f) {
        fprintf(f, "%ld %ld\n", r.left, r.top);
        fclose(f);
    }
}

void PetWindow::loadPosition(const char* filePath) {
    FILE* f = fopen(filePath, "r");
    if (!f) return;

    int x = 0;
    int y = 0;
    if (fscanf(f, "%d %d", &x, &y) == 2) {
        moveTo(x, y);
    }
    fclose(f);
}

void PetWindow::setClickHandler(std::function<void(POINT)> handler) {
    m_clickHandler = std::move(handler);
}

void PetWindow::invalidate() {
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

LRESULT CALLBACK PetWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    PetWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lp;
        self = (PetWindow*)cs->lpCreateParams;
        self->m_hWnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    } else {
        self = (PetWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (self) return self->handleMessage(msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT PetWindow::handleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_TIMER:
        if (wp == IDT_RENDER) {
            static int topmostCounter = 0;
            if (++topmostCounter >= 300) {
                topmostCounter = 0;
                SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                             SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
            }

            if (m_renderCb) {
                m_renderer.beginFrame();
                m_renderCb(&m_renderer);
                m_renderer.endFrame(m_hWnd);
            }
        }
        return 0;

    case WM_NCHITTEST: {
        POINTS pts = MAKEPOINTS(lp);
        POINT pt = {pts.x, pts.y};
        ScreenToClient(m_hWnd, &pt);
        float cx = (float)m_width / 2.0f;
        float cy = (float)m_height / 2.0f;
        float rx = 80.0f;
        float ry = 92.0f;
        float dx = ((float)pt.x - cx) / rx;
        float dy = ((float)pt.y - cy) / ry;
        if (dx * dx + dy * dy <= 1.0f) return HTCLIENT;
        return HTTRANSPARENT;
    }

    case WM_LBUTTONDOWN: {
        m_dragging = false;
        GetCursorPos(&m_dragStart);
        RECT r;
        GetWindowRect(m_hWnd, &r);
        m_dragWinPos = {r.left, r.top};
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
            m_dragging = false;
            return 0;
        }

        POINT cur;
        GetCursorPos(&cur);
        int dx = cur.x - m_dragStart.x;
        int dy = cur.y - m_dragStart.y;
        if (!m_dragging && dx * dx + dy * dy >= 16) {
            m_dragging = true;
        }

        if (m_dragging) {
            SetWindowPos(m_hWnd, HWND_TOPMOST,
                         m_dragWinPos.x + dx, m_dragWinPos.y + dy,
                         m_width, m_height,
                         SWP_NOACTIVATE | SWP_NOZORDER | SWP_ASYNCWINDOWPOS);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        POINT cur;
        GetCursorPos(&cur);
        int dx = cur.x - m_dragStart.x;
        int dy = cur.y - m_dragStart.y;
        if (!m_dragging && dx * dx + dy * dy <= 25) {
            POINTS pts = MAKEPOINTS(lp);
            POINT pt = {pts.x, pts.y};
            if (m_clickHandler) m_clickHandler(pt);
        }
        m_dragging = false;
        return 0;
    }

    case WM_API_RESPONSE: {
        if (lp && m_apiCb) {
            std::string* text = reinterpret_cast<std::string*>(lp);
            m_apiCb(*text);
            delete text;
        }
        return 0;
    }

    case WM_WORD_EVENT: {
        if (lp && m_wordEventCb) {
            std::string* text = reinterpret_cast<std::string*>(lp);
            m_wordEventCb(*text);
            delete text;
        }
        return 0;
    }

    case WM_HOTKEY:
        if (wp == IDH_TOGGLE) {
            ShowWindow(m_hWnd, IsWindowVisible(m_hWnd) ? SW_HIDE : SW_SHOWNOACTIVATE);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(m_hWnd, msg, wp, lp);
    }
}
