#pragma once
// Console3 - VTermWrapper.h
// C++ wrapper for libvterm terminal emulation library
//
// Provides a modern C++ interface to libvterm's screen layer for
// parsing VT sequences and managing terminal state.

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS5
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#include <Windows.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// libvterm C header
extern "C" {
#include <vterm.h>
}

namespace Console3::Emulation {

// Forward declarations
class VTermWrapper;

/// Color representation (24-bit RGB + type info)
struct TermColor {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    bool isDefault = true;        ///< Is this the default fg/bg color?
    bool isIndexed = false;       ///< Is this an indexed palette color?
    uint8_t paletteIndex = 0;     ///< Palette index if isIndexed is true

    /// Create from VTermColor
    static TermColor FromVTerm(const VTermColor& color);
};

/// Cell attributes (bold, italic, etc.)
struct CellAttrs {
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool blink = false;
    bool reverse = false;
    bool strikethrough = false;
    bool conceal = false;
    uint8_t underlineStyle = 0;   ///< 0=off, 1=single, 2=double, 3=curly

    /// Create from VTermScreenCellAttrs
    static CellAttrs FromVTerm(const VTermScreenCellAttrs& attrs);
};

/// A single terminal cell
struct TermCell {
    std::vector<uint32_t> chars;  ///< UTF-32 codepoints (may include combining chars)
    int width = 1;                ///< Cell width (1 or 2 for wide chars)
    CellAttrs attrs;              ///< Visual attributes
    TermColor fg;                 ///< Foreground color
    TermColor bg;                 ///< Background color
};

/// Cursor shape enumeration
enum class CursorShape {
    Block,
    Underline,
    Bar
};

/// Terminal properties that can change
struct TermProps {
    std::wstring title;           ///< Window title
    std::wstring iconName;        ///< Icon name
    bool cursorVisible = true;    ///< Is cursor visible?
    bool cursorBlink = true;      ///< Should cursor blink?
    CursorShape cursorShape = CursorShape::Block;
    bool altScreen = false;       ///< Is alternate screen buffer active?
    int mouseMode = 0;            ///< Mouse reporting mode
};

/// Callback types for terminal events
using DamageCallback = std::function<void(int startRow, int endRow, int startCol, int endCol)>;
using MoveCursorCallback = std::function<void(int row, int col, bool visible)>;
using SetTermPropCallback = std::function<void(const TermProps& props)>;
using BellCallback = std::function<void()>;
using ResizeCallback = std::function<void(int rows, int cols)>;
using OutputCallback = std::function<void(const char* data, size_t len)>;
using ScrollbackPushCallback = std::function<void(const std::vector<TermCell>& cells)>;

/// C++ wrapper for libvterm
class VTermWrapper {
public:
    /// Create a new terminal emulator
    /// @param rows Initial row count
    /// @param cols Initial column count
    explicit VTermWrapper(int rows = 25, int cols = 80);
    ~VTermWrapper();

    // Non-copyable, non-movable (VTerm* is unique)
    VTermWrapper(const VTermWrapper&) = delete;
    VTermWrapper& operator=(const VTermWrapper&) = delete;
    VTermWrapper(VTermWrapper&&) = delete;
    VTermWrapper& operator=(VTermWrapper&&) = delete;

    /// Input data from PTY (VT sequences to parse)
    /// @param data Raw bytes from PTY
    /// @param length Number of bytes
    /// @return Number of bytes consumed
    size_t InputWrite(const char* data, size_t length);

    /// Input a keyboard character
    /// @param codepoint Unicode codepoint
    /// @param modifiers Modifier keys (Ctrl, Alt, Shift)
    void KeyboardUnichar(uint32_t codepoint, int modifiers = 0);

    /// Input a special keyboard key
    /// @param key Key code (from VTermKey enum)
    /// @param modifiers Modifier keys
    void KeyboardKey(int key, int modifiers = 0);

    /// Resize the terminal
    /// @param rows New row count
    /// @param cols New column count
    void Resize(int rows, int cols);

    /// Get current terminal size
    void GetSize(int& rows, int& cols) const;

    /// Get a cell at the specified position
    /// @param row Row (0-indexed)
    /// @param col Column (0-indexed)
    /// @return Cell data
    [[nodiscard]] TermCell GetCell(int row, int col) const;

    /// Get the current cursor position
    void GetCursorPos(int& row, int& col) const;

    /// Get current terminal properties
    [[nodiscard]] const TermProps& GetProps() const noexcept;

    /// Reset the terminal (hard reset)
    void Reset();

    /// Flush pending damage notifications
    void FlushDamage();

    /// Read output data (responses to terminal queries)
    /// @param buffer Buffer to read into
    /// @param maxLen Maximum bytes to read
    /// @return Number of bytes read
    size_t OutputRead(char* buffer, size_t maxLen);

    // Callback setters
    void SetDamageCallback(DamageCallback callback);
    void SetMoveCursorCallback(MoveCursorCallback callback);
    void SetTermPropCallback(SetTermPropCallback callback);
    void SetBellCallback(BellCallback callback);
    void SetResizeCallback(ResizeCallback callback);
    void SetOutputCallback(OutputCallback callback);
    void SetScrollbackPushCallback(ScrollbackPushCallback callback);

    /// Convert a VTermColor to RGB using the current palette
    void ConvertColorToRgb(VTermColor& color) const;

    /// Get the underlying VTerm pointer (for advanced use)
    [[nodiscard]] VTerm* GetVTerm() const noexcept { return m_vterm; }

private:
    // libvterm callback handlers (static for C callback interface)
    static int OnDamage(VTermRect rect, void* user);
    static int OnMoveRect(VTermRect dest, VTermRect src, void* user);
    static int OnMoveCursor(VTermPos pos, VTermPos oldpos, int visible, void* user);
    static int OnSetTermProp(VTermProp prop, VTermValue* val, void* user);
    static int OnBell(void* user);
    static int OnResize(int rows, int cols, void* user);
    static int OnScrollbackPush(int cols, const VTermScreenCell* cells, void* user);
    static int OnScrollbackPop(int cols, VTermScreenCell* cells, void* user);

    // Output callback
    static void OnOutput(const char* s, size_t len, void* user);

private:
    VTerm* m_vterm = nullptr;
    VTermScreen* m_screen = nullptr;
    
    // Current state
    TermProps m_props;
    int m_cursorRow = 0;
    int m_cursorCol = 0;

    // Callbacks
    DamageCallback m_damageCallback;
    MoveCursorCallback m_moveCursorCallback;
    SetTermPropCallback m_termPropCallback;
    BellCallback m_bellCallback;
    ResizeCallback m_resizeCallback;
    OutputCallback m_outputCallback;
    ScrollbackPushCallback m_scrollbackPushCallback;

    // Screen callbacks structure (must persist for lifetime)
    VTermScreenCallbacks m_screenCallbacks{};
};

} // namespace Console3::Emulation
