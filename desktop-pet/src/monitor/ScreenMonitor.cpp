#include "ScreenMonitor.h"
#include "../util/Logger.h"
#include <Windows.h>

ScreenMonitor::ScreenMonitor(int titlePollMs, int screenshotIntervalS,
                             int downscaleWidth, bool screenshotEnabled)
    : m_titlePollMs(titlePollMs)
    , m_screenshotIntervalS(screenshotIntervalS)
    , m_downscaleWidth(downscaleWidth)
    , m_screenshotEnabled(screenshotEnabled) {}

void ScreenMonitor::update(float dt) {
    m_titleTimer += dt;
    if (m_titleTimer * 1000.0f >= (float)m_titlePollMs) {
        m_titleTimer = 0;
        pollTitle();
    }

    if (!m_screenshotEnabled) return;

    m_screenshotTimer += dt;
    if (m_screenshotTimer >= (float)m_screenshotIntervalS) {
        m_screenshotTimer = 0;
        captureScreenshot();
    }
}

void ScreenMonitor::pollTitle() {
    HWND fg = GetForegroundWindow();
    if (!fg) return;

    int len = GetWindowTextLengthW(fg);
    if (len <= 0) return;

    std::wstring title(len + 1, L'\0');
    GetWindowTextW(fg, &title[0], len + 1);
    title.resize(len);

    if (title != m_lastTitle) {
        m_lastTitle = title;
        LOG_INFO("Title changed: %ls", title.c_str());
        if (m_titleCb) m_titleCb(title);
    }
}

void ScreenMonitor::captureScreenshot() {
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    HDC screenDC = GetDC(nullptr);
    if (!screenDC) return;

    int dw = m_downscaleWidth;
    int dh = (int)((float)sh * (float)dw / (float)sw);

    HDC memDC = CreateCompatibleDC(screenDC);
    if (!memDC) {
        ReleaseDC(nullptr, screenDC);
        return;
    }

    HBITMAP bitmap = CreateCompatibleBitmap(screenDC, dw, dh);
    if (!bitmap) {
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return;
    }

    HGDIOBJ oldBmp = SelectObject(memDC, bitmap);
    if (!oldBmp || oldBmp == HGDI_ERROR) {
        DeleteObject(bitmap);
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return;
    }

    SetStretchBltMode(memDC, HALFTONE);
    StretchBlt(memDC, 0, 0, dw, dh,
               screenDC, 0, 0, sw, sh, SRCCOPY);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = dw;
    bmi.bmiHeader.biHeight = -dh;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint8_t> pixels(dw * dh * 4);
    GetDIBits(memDC, bitmap, 0, dh, pixels.data(), &bmi, DIB_RGB_COLORS);

    SelectObject(memDC, oldBmp);
    DeleteObject(bitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);

    LOG_INFO("Screenshot captured %dx%d (%zu bytes)", dw, dh, pixels.size());
    if (m_screenshotCb) m_screenshotCb(pixels, dw, dh);
}
