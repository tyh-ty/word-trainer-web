#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

class ScreenMonitor {
public:
    ScreenMonitor(int titlePollMs, int screenshotIntervalS,
                  int downscaleWidth, bool screenshotEnabled);

    void update(float dt);

    using TitleCallback = std::function<void(const std::wstring& title)>;
    using ScreenshotCallback = std::function<void(const std::vector<uint8_t>& bgra,
                                                  int width, int height)>;

    void setTitleCallback(TitleCallback cb) { m_titleCb = std::move(cb); }
    void setScreenshotCallback(ScreenshotCallback cb) { m_screenshotCb = std::move(cb); }

    const std::wstring& lastTitle() const { return m_lastTitle; }

private:
    void pollTitle();
    void captureScreenshot();

    float m_titleTimer = 0;
    float m_screenshotTimer = 0;

    int m_titlePollMs;
    int m_screenshotIntervalS;
    int m_downscaleWidth;
    bool m_screenshotEnabled;

    std::wstring m_lastTitle;

    TitleCallback m_titleCb;
    ScreenshotCallback m_screenshotCb;
};
