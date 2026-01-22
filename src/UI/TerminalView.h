#pragma once
// Console3 - TerminalView.h
// Terminal rendering window using Direct2D
//
// Child window that renders the terminal content, handles keyboard/mouse
// input, and manages cursor blinking.

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS5
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

// ATL/WTL headers
#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <atlcrack.h>

#include "UI/D2DRenderer.h"
#include "Core/TerminalBuffer.h"
#include "Emulation/VTermWrapper.h"

#include <functional>
#include <memory>
#include <string>

namespace Console3::UI {

/// Cursor style
enum class CursorStyle {
    Block,
    Underline,
    Bar
};

/// Mouse reporting mode
enum class MouseMode {
    None,       ///< No mouse reporting
    X10,        ///< X10 basic mouse (button press only)
    Normal,     ///< Normal tracking (button press and release)
    SGR         ///< SGR extended (supports large coordinates)
};

/// Selection state
struct Selection {
    int startRow = 0;
    int startCol = 0;
    int endRow = 0;
    int endCol = 0;
    bool active = false;

    /// Check if a cell is within the selection
    [[nodiscard]] bool Contains(int row, int col) const;

    /// Normalize selection (ensure start <= end)
    void Normalize();

    /// Get selected text from buffer
    [[nodiscard]] std::wstring GetText(const Core::TerminalBuffer& buffer) const;
};

/// Callback for keyboard input
using KeyboardInputCallback = std::function<void(const char* data, size_t len)>;

/// Terminal rendering view
class TerminalView : public CWindowImpl<TerminalView> {
public:
    DECLARE_WND_CLASS(L"Console3TerminalView")

    TerminalView();
    ~TerminalView();

    /// Initialize the terminal view
    /// @param d2dFactory Direct2D factory
    /// @param dwriteFactory DirectWrite factory
    /// @param buffer Terminal buffer to display
    /// @return true on success
    [[nodiscard]] bool Initialize(
        ID2D1Factory1* d2dFactory,
        IDWriteFactory1* dwriteFactory,
        Core::TerminalBuffer* buffer
    );

    /// Set the terminal buffer to display
    void SetBuffer(Core::TerminalBuffer* buffer);

    /// Set the VTerm wrapper for input handling
    void SetVTerm(Emulation::VTermWrapper* vterm);

    /// Set callback for keyboard input
    void SetKeyboardInputCallback(KeyboardInputCallback callback);

    /// Set the font
    /// @param fontName Font family name
    /// @param fontSize Font size in points
    /// @return true on success
    [[nodiscard]] bool SetFont(const std::wstring& fontName, float fontSize);

    /// Set cursor style
    void SetCursorStyle(CursorStyle style);

    /// Set cursor blink rate (0 to disable)
    void SetCursorBlinkRate(UINT milliseconds);

    /// Set whether cursor is visible
    void SetCursorVisible(bool visible);

    /// Get current terminal dimensions in cells
    [[nodiscard]] int GetTerminalRows() const;
    [[nodiscard]] int GetTerminalCols() const;

    /// Request a redraw
    void Invalidate();

    /// Copy selection to clipboard
    void CopyToClipboard();

    /// Paste from clipboard
    void PasteFromClipboard();

    /// Clear selection
    void ClearSelection();

    /// Set mouse reporting mode
    void SetMouseMode(MouseMode mode);

    /// Enable/disable bracketed paste mode
    void SetBracketedPasteMode(bool enabled);

    // Message map
    BEGIN_MSG_MAP(TerminalView)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_SIZE(OnSize)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_ERASEBKGND(OnEraseBkgnd)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_SETFOCUS(OnSetFocus)
        MSG_WM_KILLFOCUS(OnKillFocus)
        MSG_WM_KEYDOWN(OnKeyDown)
        MSG_WM_KEYUP(OnKeyUp)
        MSG_WM_CHAR(OnChar)
        MSG_WM_LBUTTONDOWN(OnLButtonDown)
        MSG_WM_LBUTTONUP(OnLButtonUp)
        MSG_WM_MOUSEMOVE(OnMouseMove)
        MSG_WM_MOUSEWHEEL(OnMouseWheel)
        // IME messages for CJK input
        MESSAGE_HANDLER(WM_IME_STARTCOMPOSITION, OnImeStartComposition)
        MESSAGE_HANDLER(WM_IME_COMPOSITION, OnImeComposition)
        MESSAGE_HANDLER(WM_IME_ENDCOMPOSITION, OnImeEndComposition)
        MESSAGE_HANDLER(WM_IME_CHAR, OnImeChar)
    END_MSG_MAP()

private:
    // Message handlers
    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnSize(UINT nType, CSize size);
    void OnPaint(CDCHandle dc);
    LRESULT OnEraseBkgnd(CDCHandle dc);
    void OnTimer(UINT_PTR nIDEvent);
    void OnSetFocus(CWindow wndOld);
    void OnKillFocus(CWindow wndNew);
    void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnLButtonDown(UINT nFlags, CPoint point);
    void OnLButtonUp(UINT nFlags, CPoint point);
    void OnMouseMove(UINT nFlags, CPoint point);
    BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

    // IME handlers for CJK input
    LRESULT OnImeStartComposition(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnImeComposition(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnImeEndComposition(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnImeChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    // Rendering
    void Render();
    void RenderRow(int row);
    void RenderCursor();
    void RenderSelection();

    // Input handling
    void SendKeyToTerminal(UINT vkey, UINT scanCode, bool keyDown);
    void SendCharToTerminal(wchar_t ch);
    void SendMouseReport(int button, int row, int col, bool pressed);

    // Coordinate conversion
    int PixelToRow(int y) const;
    int PixelToCol(int x) const;
    float RowToPixel(int row) const;
    float ColToPixel(int col) const;

    // Timer IDs
    static constexpr UINT_PTR TIMER_CURSOR_BLINK = 1;

private:
    // Renderer
    std::unique_ptr<D2DRenderer> m_renderer;

    // Terminal state (not owned)
    Core::TerminalBuffer* m_buffer = nullptr;
    Emulation::VTermWrapper* m_vterm = nullptr;

    // Callbacks
    KeyboardInputCallback m_keyboardCallback;

    // Cursor state
    CursorStyle m_cursorStyle = CursorStyle::Block;
    bool m_cursorVisible = true;
    bool m_cursorBlinkState = true;  // true = visible in blink cycle
    UINT m_cursorBlinkRate = 530;    // milliseconds
    bool m_hasFocus = false;

    // Selection state
    Selection m_selection;
    bool m_isSelecting = false;

    // Scroll offset (for scrollback viewing)
    int m_scrollOffset = 0;

    // Colors
    Color m_defaultFg = Color::FromRgb(204, 204, 204);
    Color m_defaultBg = Color::FromRgb(12, 12, 12);
    Color m_cursorColor = Color::FromRgb(255, 255, 255);
    Color m_selectionColor = Color::FromRgb(38, 79, 120);

    // Mouse and paste modes
    MouseMode m_mouseMode = MouseMode::None;
    bool m_bracketedPasteMode = false;
};

} // namespace Console3::UI
