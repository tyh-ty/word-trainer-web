#pragma once
#include <string>
#include <d2d1.h>
#include <dwrite.h>

class D2DRenderer;

class SpeechBubble {
public:
    SpeechBubble();
    ~SpeechBubble();

    void show(const std::string& text, int durationMs);
    void update(float dt);
    void draw(D2DRenderer* renderer, float petCx, float petTopY);
    bool isActive() const { return m_active; }

private:
    bool initTextFormat();

    std::string m_text;
    float m_timer = 0;
    float m_duration = 0;
    bool m_active = false;

    IDWriteFactory* m_writeFactory = nullptr;
    IDWriteTextFormat* m_textFormat = nullptr;
};
