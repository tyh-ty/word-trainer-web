#include "SpeechBubble.h"
#include "../win/D2DRenderer.h"
#include "../util/Logger.h"
#include "../util/StringUtils.h"
#include <algorithm>
#include <cmath>

SpeechBubble::SpeechBubble() {
    initTextFormat();
}

SpeechBubble::~SpeechBubble() {
    if (m_textFormat) m_textFormat->Release();
    if (m_writeFactory) m_writeFactory->Release();
}

bool SpeechBubble::initTextFormat() {
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_writeFactory));
    if (FAILED(hr)) {
        LOG_ERROR("DWriteCreateFactory failed: 0x%08X", hr);
        return false;
    }

    hr = m_writeFactory->CreateTextFormat(
        L"Microsoft YaHei UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"zh-cn", &m_textFormat);
    if (FAILED(hr)) {
        LOG_ERROR("CreateTextFormat failed: 0x%08X", hr);
        return false;
    }

    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    m_textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    return true;
}

void SpeechBubble::show(const std::string& text, int durationMs) {
    m_text = text;
    m_duration = (float)durationMs / 1000.0f;
    m_timer = 0;
    m_active = true;
}

void SpeechBubble::update(float dt) {
    if (!m_active) return;
    m_timer += dt;
    if (m_timer >= m_duration) {
        m_active = false;
        m_text.clear();
    }
}

void SpeechBubble::draw(D2DRenderer* renderer, float petCx, float petTopY) {
    if (!m_active || m_text.empty()) return;
    if (!m_textFormat) return;

    ID2D1RenderTarget* rt = renderer->getTarget();
    ID2D1Factory* factory = renderer->getFactory();
    if (!rt || !factory) return;

    float alpha = 1.0f;
    float fadeStart = m_duration * 0.85f;
    if (m_timer > fadeStart) {
        alpha = 1.0f - (m_timer - fadeStart) / (m_duration - fadeStart);
        if (alpha < 0) alpha = 0;
    }

    std::wstring wideText = StringUtils::utf8ToWide(m_text);

    const float padX = 22.0f;
    const float padY = 14.0f;

    IDWriteTextLayout* layout = nullptr;
    const float textMaxW = 200.0f;
    m_writeFactory->CreateTextLayout(wideText.c_str(), (UINT32)wideText.size(),
        m_textFormat, textMaxW, 200.0f, &layout);
    if (!layout) return;

    DWRITE_TEXT_METRICS metrics;
    layout->GetMetrics(&metrics);

    float textW = metrics.width;
    float textH = metrics.height;
    float bw = textW + padX * 2;
    float bh = textH + padY * 2;

    if (bw < 120.0f) bw = 120.0f;

    float left = petCx - bw / 2.0f;
    float top = petTopY - bh - 30.0f;
    float winWf = (float)renderer->getWidth();
    if (left < 8.0f) left = 8.0f;
    if (left + bw > winWf - 8.0f) left = winWf - bw - 8.0f;
    if (top < 8.0f) top = 8.0f;
    float right = left + bw;
    float bottom = top + bh;

    ID2D1SolidColorBrush* bubbleBrush = renderer->getBrush(1, 1, 1, alpha * 0.92f);
    ID2D1SolidColorBrush* textBrush  = renderer->getBrush(0.15f, 0.15f, 0.18f, alpha);

    D2D1_ROUNDED_RECT rr = {{left, top, right, bottom}, 14.0f, 14.0f};
    rt->FillRoundedRectangle(&rr, bubbleBrush);

    // Adjust layout max width to match the actual bubble content text area,
    // so center alignment works exactly inside the bubble text boundaries.
    layout->SetMaxWidth(bw - padX * 2.0f);

    rt->DrawTextLayout(
        D2D1::Point2F(left + padX, top + padY),
        layout, textBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);

    layout->Release();
}
