#pragma once
// Console3 - D2DRenderer.h
// Direct2D rendering wrapper for terminal display
//
// Manages Direct2D render targets, brushes, and provides
// drawing primitives for terminal rendering.

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS5
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#include <Windows.h>
#include <d2d1_1.h>
#include <dwrite_1.h>
#include <wrl/client.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

using Microsoft::WRL::ComPtr;

namespace Console3::UI {

/// RGBA color structure
struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    /// Create from RGB bytes (0-255)
    static Color FromRgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return {
            r / 255.0f,
            g / 255.0f,
            b / 255.0f,
            a / 255.0f
        };
    }

    /// Convert to D2D1_COLOR_F
    [[nodiscard]] D2D1_COLOR_F ToD2D() const {
        return D2D1::ColorF(r, g, b, a);
    }
};

/// Configuration for the renderer
struct RendererConfig {
    HWND hwnd = nullptr;
    ID2D1Factory1* d2dFactory = nullptr;
    IDWriteFactory1* dwriteFactory = nullptr;
    Color backgroundColor = Color::FromRgb(12, 12, 12);  // Dark background
    float dpiScaleX = 1.0f;
    float dpiScaleY = 1.0f;
};

/// Direct2D renderer for terminal display
class D2DRenderer {
public:
    D2DRenderer();
    ~D2DRenderer();

    // Non-copyable
    D2DRenderer(const D2DRenderer&) = delete;
    D2DRenderer& operator=(const D2DRenderer&) = delete;

    /// Initialize the renderer with the given configuration
    /// @param config Renderer configuration
    /// @return true on success
    [[nodiscard]] bool Initialize(const RendererConfig& config);

    /// Shutdown and release all resources
    void Shutdown();

    /// Check if the renderer is initialized
    [[nodiscard]] bool IsInitialized() const noexcept { return m_renderTarget != nullptr; }

    /// Handle window resize
    /// @param width New width in pixels
    /// @param height New height in pixels
    /// @return true on success
    [[nodiscard]] bool Resize(UINT width, UINT height);

    /// Handle DPI change
    /// @param dpiX New horizontal DPI
    /// @param dpiY New vertical DPI
    void SetDpi(float dpiX, float dpiY);

    /// Get current DPI scale
    [[nodiscard]] float GetDpiScaleX() const noexcept { return m_dpiScaleX; }
    [[nodiscard]] float GetDpiScaleY() const noexcept { return m_dpiScaleY; }

    // ========================================================================
    // Rendering Commands
    // ========================================================================

    /// Begin a frame (must be called before any drawing)
    /// @return true if rendering can proceed
    [[nodiscard]] bool BeginDraw();

    /// End the frame and present
    /// @return true on success, false if render target needs recreation
    [[nodiscard]] bool EndDraw();

    /// Clear the render target with the background color
    void Clear();

    /// Clear with a specific color
    void Clear(const Color& color);

    /// Draw a filled rectangle
    void FillRect(float x, float y, float width, float height, const Color& color);

    /// Draw a rectangle outline
    void DrawRect(float x, float y, float width, float height, const Color& color, float strokeWidth = 1.0f);

    /// Draw a line
    void DrawLine(float x1, float y1, float x2, float y2, const Color& color, float strokeWidth = 1.0f);

    /// Draw text at the specified position
    /// @param text Text to draw (UTF-16)
    /// @param x X position
    /// @param y Y position
    /// @param color Text color
    /// @param fontName Font family name (optional, uses default if empty)
    void DrawText(const std::wstring& text, float x, float y, const Color& color,
                  const std::wstring& fontName = L"");

    /// Draw a single character (optimized for terminal cells)
    /// @param codepoint UTF-32 codepoint
    /// @param x X position
    /// @param y Y position
    /// @param color Text color
    void DrawChar(uint32_t codepoint, float x, float y, const Color& color);

    /// Draw text using a text layout
    void DrawTextLayout(IDWriteTextLayout* layout, float x, float y, const Color& color);

    // ========================================================================
    // Font Management
    // ========================================================================

    /// Set the default font
    /// @param fontName Font family name
    /// @param fontSize Font size in points
    /// @return true on success
    [[nodiscard]] bool SetFont(const std::wstring& fontName, float fontSize);

    /// Get the current cell dimensions based on font metrics
    [[nodiscard]] float GetCellWidth() const noexcept { return m_cellWidth; }
    [[nodiscard]] float GetCellHeight() const noexcept { return m_cellHeight; }

    /// Get the baseline offset for text rendering
    [[nodiscard]] float GetBaseline() const noexcept { return m_baseline; }

    /// Create a text layout for a string
    [[nodiscard]] ComPtr<IDWriteTextLayout> CreateTextLayout(
        const std::wstring& text, float maxWidth, float maxHeight);

    // ========================================================================
    // Brush Management
    // ========================================================================

    /// Get or create a solid color brush for the given color
    [[nodiscard]] ID2D1SolidColorBrush* GetBrush(const Color& color);

    /// Get the render target (for advanced use)
    [[nodiscard]] ID2D1HwndRenderTarget* GetRenderTarget() const { return m_renderTarget.Get(); }

    /// Get the DirectWrite factory
    [[nodiscard]] IDWriteFactory1* GetDWriteFactory() const { return m_dwriteFactory; }

private:
    /// Create device-dependent resources
    [[nodiscard]] bool CreateDeviceResources();

    /// Release device-dependent resources
    void DiscardDeviceResources();

    /// Update cell metrics based on current font
    void UpdateCellMetrics();

    /// Generate a hash key for a color (for brush caching)
    [[nodiscard]] static uint32_t ColorHash(const Color& color);

private:
    // Factories (not owned)
    ID2D1Factory1* m_d2dFactory = nullptr;
    IDWriteFactory1* m_dwriteFactory = nullptr;

    // Window handle
    HWND m_hwnd = nullptr;

    // Render target
    ComPtr<ID2D1HwndRenderTarget> m_renderTarget;

    // Text format (default font)
    ComPtr<IDWriteTextFormat> m_textFormat;

    // Brush cache (color hash -> brush)
    std::unordered_map<uint32_t, ComPtr<ID2D1SolidColorBrush>> m_brushCache;

    // Font metrics
    std::wstring m_fontName = L"Consolas";
    float m_fontSize = 12.0f;
    float m_cellWidth = 8.0f;
    float m_cellHeight = 16.0f;
    float m_baseline = 12.0f;

    // DPI scaling
    float m_dpiScaleX = 1.0f;
    float m_dpiScaleY = 1.0f;

    // Background color
    Color m_backgroundColor;

    // State
    bool m_isDrawing = false;
};

} // namespace Console3::UI
