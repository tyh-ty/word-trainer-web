#pragma once
#include <d2d1.h>
#include <wincodec.h>
#include <windows.h>
#include <map>
#include <string>
#include <cstdint>

class D2DRenderer {
public:
    D2DRenderer();
    ~D2DRenderer();

    bool initialize(int width, int height);
    void cleanup();

    ID2D1RenderTarget* getTarget() const { return m_renderTarget; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    HBITMAP getHBitmap() const { return m_hBitmap; }
    HDC getMemoryDC() const { return m_memDC; }

    void beginFrame();
    void endFrame(HWND hwnd);

    ID2D1SolidColorBrush* getBrush(float r, float g, float b, float a = 1.0f);
    ID2D1Factory* getFactory() const { return m_factory; }

    ID2D1Bitmap* loadBitmapFromFile(const std::wstring& path);
    void releaseBitmap(ID2D1Bitmap*& bmp);

private:
    uint32_t colorKey(float r, float g, float b, float a);
    ID2D1Factory* m_factory = nullptr;
    ID2D1DCRenderTarget* m_renderTarget = nullptr;
    IWICImagingFactory* m_wicFactory = nullptr;

    int m_width = 0;
    int m_height = 0;

    HBITMAP m_hBitmap = nullptr;
    HDC m_memDC = nullptr;
    HDC m_screenDC = nullptr;

    std::map<uint32_t, ID2D1SolidColorBrush*> m_brushCache;
};
