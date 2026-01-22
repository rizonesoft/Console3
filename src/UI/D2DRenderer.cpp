// Console3 - D2DRenderer.cpp
// Direct2D rendering wrapper implementation

#include "UI/D2DRenderer.h"
#include <algorithm>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace Console3::UI {

D2DRenderer::D2DRenderer() = default;

D2DRenderer::~D2DRenderer() {
    Shutdown();
}

bool D2DRenderer::Initialize(const RendererConfig& config) {
    if (!config.hwnd || !config.d2dFactory || !config.dwriteFactory) {
        return false;
    }

    m_hwnd = config.hwnd;
    m_d2dFactory = config.d2dFactory;
    m_dwriteFactory = config.dwriteFactory;
    m_backgroundColor = config.backgroundColor;
    m_dpiScaleX = config.dpiScaleX;
    m_dpiScaleY = config.dpiScaleY;

    // Create device resources
    if (!CreateDeviceResources()) {
        return false;
    }

    // Set default font
    if (!SetFont(L"Consolas", 12.0f)) {
        return false;
    }

    return true;
}

void D2DRenderer::Shutdown() {
    DiscardDeviceResources();
    m_d2dFactory = nullptr;
    m_dwriteFactory = nullptr;
    m_hwnd = nullptr;
}

bool D2DRenderer::Resize(UINT width, UINT height) {
    if (!m_renderTarget) {
        return false;
    }

    D2D1_SIZE_U size = D2D1::SizeU(width, height);
    HRESULT hr = m_renderTarget->Resize(size);
    return SUCCEEDED(hr);
}

void D2DRenderer::SetDpi(float dpiX, float dpiY) {
    m_dpiScaleX = dpiX / 96.0f;
    m_dpiScaleY = dpiY / 96.0f;

    if (m_renderTarget) {
        m_renderTarget->SetDpi(dpiX, dpiY);
    }

    // Recalculate font metrics with new DPI
    UpdateCellMetrics();
}

// ============================================================================
// Rendering Commands
// ============================================================================

bool D2DRenderer::BeginDraw() {
    if (!m_renderTarget || m_isDrawing) {
        return false;
    }

    m_renderTarget->BeginDraw();
    m_isDrawing = true;
    return true;
}

bool D2DRenderer::EndDraw() {
    if (!m_renderTarget || !m_isDrawing) {
        return false;
    }

    m_isDrawing = false;
    HRESULT hr = m_renderTarget->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET) {
        // Device lost - need to recreate resources
        DiscardDeviceResources();
        return CreateDeviceResources();
    }

    return SUCCEEDED(hr);
}

void D2DRenderer::Clear() {
    Clear(m_backgroundColor);
}

void D2DRenderer::Clear(const Color& color) {
    if (m_renderTarget && m_isDrawing) {
        m_renderTarget->Clear(color.ToD2D());
    }
}

void D2DRenderer::FillRect(float x, float y, float width, float height, const Color& color) {
    if (!m_renderTarget || !m_isDrawing) return;

    ID2D1SolidColorBrush* brush = GetBrush(color);
    if (brush) {
        D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
        m_renderTarget->FillRectangle(rect, brush);
    }
}

void D2DRenderer::DrawRect(float x, float y, float width, float height, 
                           const Color& color, float strokeWidth) {
    if (!m_renderTarget || !m_isDrawing) return;

    ID2D1SolidColorBrush* brush = GetBrush(color);
    if (brush) {
        D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
        m_renderTarget->DrawRectangle(rect, brush, strokeWidth);
    }
}

void D2DRenderer::DrawLine(float x1, float y1, float x2, float y2, 
                           const Color& color, float strokeWidth) {
    if (!m_renderTarget || !m_isDrawing) return;

    ID2D1SolidColorBrush* brush = GetBrush(color);
    if (brush) {
        m_renderTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), 
                                  brush, strokeWidth);
    }
}

void D2DRenderer::DrawText(const std::wstring& text, float x, float y, 
                           const Color& color, const std::wstring& /*fontName*/) {
    if (!m_renderTarget || !m_isDrawing || !m_textFormat || text.empty()) return;

    ID2D1SolidColorBrush* brush = GetBrush(color);
    if (!brush) return;

    D2D1_SIZE_F size = m_renderTarget->GetSize();
    D2D1_RECT_F layoutRect = D2D1::RectF(x, y, size.width, size.height);

    m_renderTarget->DrawTextW(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        m_textFormat.Get(),
        layoutRect,
        brush,
        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
    );
}

void D2DRenderer::DrawChar(uint32_t codepoint, float x, float y, const Color& color) {
    if (!m_renderTarget || !m_isDrawing) return;

    // Convert UTF-32 to UTF-16
    wchar_t buffer[3] = {0};
    if (codepoint <= 0xFFFF) {
        buffer[0] = static_cast<wchar_t>(codepoint);
    } else {
        // Surrogate pair
        codepoint -= 0x10000;
        buffer[0] = static_cast<wchar_t>(0xD800 | (codepoint >> 10));
        buffer[1] = static_cast<wchar_t>(0xDC00 | (codepoint & 0x3FF));
    }

    DrawText(buffer, x, y, color);
}

void D2DRenderer::DrawTextLayout(IDWriteTextLayout* layout, float x, float y, 
                                  const Color& color) {
    if (!m_renderTarget || !m_isDrawing || !layout) return;

    ID2D1SolidColorBrush* brush = GetBrush(color);
    if (brush) {
        m_renderTarget->DrawTextLayout(D2D1::Point2F(x, y), layout, brush,
                                        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    }
}

// ============================================================================
// Font Management
// ============================================================================

bool D2DRenderer::SetFont(const std::wstring& fontName, float fontSize) {
    if (!m_dwriteFactory) {
        return false;
    }

    m_fontName = fontName;
    m_fontSize = fontSize;

    // Create text format
    HRESULT hr = m_dwriteFactory->CreateTextFormat(
        fontName.c_str(),
        nullptr,  // Font collection (nullptr = system fonts)
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize * m_dpiScaleY,  // Scale font size by DPI
        L"en-us",
        m_textFormat.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr)) {
        return false;
    }

    // Set text alignment
    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    // Update cell metrics
    UpdateCellMetrics();

    return true;
}

ComPtr<IDWriteTextLayout> D2DRenderer::CreateTextLayout(
    const std::wstring& text, float maxWidth, float maxHeight) {
    
    if (!m_dwriteFactory || !m_textFormat) {
        return nullptr;
    }

    ComPtr<IDWriteTextLayout> layout;
    HRESULT hr = m_dwriteFactory->CreateTextLayout(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        m_textFormat.Get(),
        maxWidth,
        maxHeight,
        layout.GetAddressOf()
    );

    return SUCCEEDED(hr) ? layout : nullptr;
}

// ============================================================================
// Brush Management
// ============================================================================

ID2D1SolidColorBrush* D2DRenderer::GetBrush(const Color& color) {
    if (!m_renderTarget) {
        return nullptr;
    }

    uint32_t hash = ColorHash(color);
    
    auto it = m_brushCache.find(hash);
    if (it != m_brushCache.end()) {
        return it->second.Get();
    }

    // Create new brush
    ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = m_renderTarget->CreateSolidColorBrush(color.ToD2D(), brush.GetAddressOf());
    
    if (FAILED(hr)) {
        return nullptr;
    }

    m_brushCache[hash] = brush;
    return brush.Get();
}

// ============================================================================
// Private Implementation
// ============================================================================

bool D2DRenderer::CreateDeviceResources() {
    if (!m_d2dFactory || !m_hwnd) {
        return false;
    }

    // Get window size
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    // Create render target
    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
        m_hwnd,
        size,
        D2D1_PRESENT_OPTIONS_NONE
    );

    HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
        rtProps,
        hwndProps,
        m_renderTarget.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr)) {
        return false;
    }

    // Set DPI
    m_renderTarget->SetDpi(96.0f * m_dpiScaleX, 96.0f * m_dpiScaleY);

    return true;
}

void D2DRenderer::DiscardDeviceResources() {
    m_brushCache.clear();
    m_renderTarget.Reset();
}

void D2DRenderer::UpdateCellMetrics() {
    if (!m_dwriteFactory || !m_textFormat) {
        return;
    }

    // Create a text layout for measuring
    ComPtr<IDWriteTextLayout> layout;
    HRESULT hr = m_dwriteFactory->CreateTextLayout(
        L"M",  // Use 'M' for em-width measurement
        1,
        m_textFormat.Get(),
        1000.0f,
        1000.0f,
        layout.GetAddressOf()
    );

    if (FAILED(hr)) {
        return;
    }

    // Get metrics
    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);

    m_cellWidth = metrics.width;
    m_cellHeight = metrics.height;

    // Get font metrics for baseline
    ComPtr<IDWriteFontCollection> fontCollection;
    m_dwriteFactory->GetSystemFontCollection(fontCollection.GetAddressOf());

    if (fontCollection) {
        UINT32 index;
        BOOL exists;
        fontCollection->FindFamilyName(m_fontName.c_str(), &index, &exists);
        
        if (exists) {
            ComPtr<IDWriteFontFamily> fontFamily;
            fontCollection->GetFontFamily(index, fontFamily.GetAddressOf());
            
            if (fontFamily) {
                ComPtr<IDWriteFont> font;
                fontFamily->GetFirstMatchingFont(
                    DWRITE_FONT_WEIGHT_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    DWRITE_FONT_STYLE_NORMAL,
                    font.GetAddressOf()
                );

                if (font) {
                    DWRITE_FONT_METRICS fontMetrics;
                    font->GetMetrics(&fontMetrics);
                    
                    float designUnitsPerEm = static_cast<float>(fontMetrics.designUnitsPerEm);
                    float scaledFontSize = m_fontSize * m_dpiScaleY;
                    m_baseline = (fontMetrics.ascent / designUnitsPerEm) * scaledFontSize;
                }
            }
        }
    }
}

uint32_t D2DRenderer::ColorHash(const Color& color) {
    // Pack RGBA into 32-bit hash
    uint8_t r = static_cast<uint8_t>(color.r * 255);
    uint8_t g = static_cast<uint8_t>(color.g * 255);
    uint8_t b = static_cast<uint8_t>(color.b * 255);
    uint8_t a = static_cast<uint8_t>(color.a * 255);
    return (r << 24) | (g << 16) | (b << 8) | a;
}

} // namespace Console3::UI
