#include "D2DRenderer.h"
#include "../util/Logger.h"

D2DRenderer::D2DRenderer() {}
D2DRenderer::~D2DRenderer() { cleanup(); }

uint32_t D2DRenderer::colorKey(float r, float g, float b, float a) {
    return (uint32_t)(a * 255) << 24
         | (uint32_t)(b * 255) << 16
         | (uint32_t)(g * 255) << 8
         | (uint32_t)(r * 255);
}

bool D2DRenderer::initialize(int width, int height) {
    m_width = width;
    m_height = height;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_wicFactory));
    if (FAILED(hr)) return false;

    m_screenDC = GetDC(nullptr);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* dibPixels = nullptr;
    m_hBitmap = CreateDIBSection(m_screenDC, &bmi, DIB_RGB_COLORS, &dibPixels, nullptr, 0);
    if (!m_hBitmap) {
        LOG_ERROR("CreateDIBSection failed");
        return false;
    }

    m_memDC = CreateCompatibleDC(m_screenDC);
    SelectObject(m_memDC, m_hBitmap);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_factory);
    if (FAILED(hr)) {
        LOG_ERROR("D2D1CreateFactory failed: 0x%08X", hr);
        return false;
    }

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    hr = m_factory->CreateDCRenderTarget(&rtProps, &m_renderTarget);
    if (FAILED(hr) || !m_renderTarget) {
        LOG_ERROR("CreateDCRenderTarget failed: 0x%08X", hr);
        return false;
    }

    m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    LOG_INFO("D2DRenderer initialized %dx%d", width, height);
    return true;
}

void D2DRenderer::beginFrame() {
    if (!m_renderTarget) return;
    RECT rc = {0, 0, m_width, m_height};
    m_renderTarget->BindDC(m_memDC, &rc);
    m_renderTarget->BeginDraw();
    m_renderTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
}

void D2DRenderer::endFrame(HWND hwnd) {
    if (!m_renderTarget || !hwnd) return;

    HRESULT hr = m_renderTarget->EndDraw();
    if (FAILED(hr)) {
        LOG_ERROR("EndDraw failed: 0x%08X", hr);
    }

    RECT wndRect = {};
    GetWindowRect(hwnd, &wndRect);
    POINT ptDst = {wndRect.left, wndRect.top};
    POINT ptSrc = {0, 0};
    SIZE size = {m_width, m_height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    HDC hdcScreen = GetDC(nullptr);
    UpdateLayeredWindow(hwnd, hdcScreen, &ptDst, &size,
                        m_memDC, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, hdcScreen);
}

ID2D1SolidColorBrush* D2DRenderer::getBrush(float r, float g, float b, float a) {
    uint32_t key = colorKey(r, g, b, a);
    auto it = m_brushCache.find(key);
    if (it != m_brushCache.end()) return it->second;

    ID2D1SolidColorBrush* brush = nullptr;
    D2D1_COLOR_F color = {r, g, b, a};
    HRESULT hr = m_renderTarget->CreateSolidColorBrush(color, &brush);
    if (FAILED(hr)) return nullptr;

    m_brushCache[key] = brush;
    return brush;
}

void D2DRenderer::cleanup() {
    for (auto& [_, brush] : m_brushCache) {
        if (brush) brush->Release();
    }
    m_brushCache.clear();

    if (m_renderTarget) { m_renderTarget->Release(); m_renderTarget = nullptr; }
    if (m_factory) { m_factory->Release(); m_factory = nullptr; }
    if (m_memDC && m_hBitmap) { SelectObject(m_memDC, nullptr); }
    if (m_hBitmap) { DeleteObject(m_hBitmap); m_hBitmap = nullptr; }
    if (m_memDC) { DeleteDC(m_memDC); m_memDC = nullptr; }
    if (m_screenDC) { ReleaseDC(nullptr, m_screenDC); m_screenDC = nullptr; }
    if (m_wicFactory) { m_wicFactory->Release(); m_wicFactory = nullptr; }
}

ID2D1Bitmap* D2DRenderer::loadBitmapFromFile(const std::wstring& path) {
    if (!m_wicFactory || !m_renderTarget) return nullptr;

    IWICBitmapDecoder* decoder = nullptr;
    HRESULT hr = m_wicFactory->CreateDecoderFromFilename(
        path.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) return nullptr;

    IWICBitmapFrameDecode* frame = nullptr;
    decoder->GetFrame(0, &frame);
    decoder->Release();

    IWICFormatConverter* converter = nullptr;
    m_wicFactory->CreateFormatConverter(&converter);
    converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    frame->Release();

    ID2D1Bitmap* bitmap = nullptr;
    hr = m_renderTarget->CreateBitmapFromWicBitmap(converter, nullptr, &bitmap);
    converter->Release();

    return SUCCEEDED(hr) ? bitmap : nullptr;
}

void D2DRenderer::releaseBitmap(ID2D1Bitmap*& bmp) {
    if (bmp) { bmp->Release(); bmp = nullptr; }
}
