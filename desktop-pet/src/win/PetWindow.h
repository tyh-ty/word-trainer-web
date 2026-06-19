#pragma once
#include "D2DRenderer.h"
#include <Windows.h>
#include <functional>
#include <string>

class PetWindow {
public:
    PetWindow(HINSTANCE hInstance);
    ~PetWindow();

    bool create();
    void show();
    void hide();
    void destroy();
    void invalidate();

    HWND getHwnd() const { return m_hWnd; }
    const D2DRenderer* getRenderer() const { return &m_renderer; }
    D2DRenderer* getRenderer() { return &m_renderer; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    void pinToBottomRight(int margin = 16);
    void moveTo(int x, int y);
    void savePosition(const char* filePath);
    void loadPosition(const char* filePath);
    void setClickHandler(std::function<void(POINT)> handler);

    using RenderCallback = std::function<void(D2DRenderer*)>;
    void setRenderCallback(RenderCallback cb) { m_renderCb = cb; }

    enum MenuCmd {
        IDM_MOOD_IDLE = 1,
        IDM_MOOD_HAPPY,
        IDM_MOOD_CURIOUS,
        IDM_MOOD_SLEEPY,
        IDM_MOOD_SURPRISED,
        IDM_MOOD_THINKING,
        IDM_PIN_BOTTOM,
        IDM_SNAP_WINDOW,
        IDM_SKIN_PINK, IDM_SKIN_BLUE, IDM_SKIN_GREEN,
        IDM_SKIN_ORANGE, IDM_SKIN_PURPLE,
        IDM_STYLE_DEFAULT, IDM_STYLE_NAILONG, IDM_STYLE_AIMEE,
        IDM_OPEN_WORD_TRAINER,
        IDM_CLEAN_MEMORY,
        IDM_EXIT
    };

    static constexpr UINT WM_PET_CLICKED = WM_USER + 100;
    static constexpr UINT WM_TITLE_CHANGED = WM_USER + 101;
    static constexpr UINT WM_API_RESPONSE = WM_USER + 102;
    static constexpr UINT WM_WORD_EVENT = WM_USER + 103;

    static constexpr UINT IDT_RENDER = 1;
    static constexpr UINT IDT_SCREENSHOT = 2;
    static constexpr UINT IDH_TOGGLE = 100;

    using MenuCallback = std::function<void(MenuCmd)>;
    void setMenuCallback(MenuCallback cb) { m_menuCb = std::move(cb); }

    using ApiResponseCallback = std::function<void(const std::string&)>;
    void setApiResponseCallback(ApiResponseCallback cb) { m_apiCb = std::move(cb); }

    using WordEventCallback = std::function<void(const std::string&)>;
    void setWordEventCallback(WordEventCallback cb) { m_wordEventCb = std::move(cb); }

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handleMessage(UINT msg, WPARAM wp, LPARAM lp);

    HINSTANCE m_hInst = nullptr;
    HWND m_hWnd = nullptr;
    D2DRenderer m_renderer;

    int m_width = 200;
    int m_height = 360;

    bool m_dragging = false;
    POINT m_dragStart = {0, 0};
    POINT m_dragWinPos = {0, 0};

    std::function<void(POINT)> m_clickHandler;
    RenderCallback m_renderCb;
    MenuCallback m_menuCb;
    ApiResponseCallback m_apiCb;
    WordEventCallback m_wordEventCb;
};
