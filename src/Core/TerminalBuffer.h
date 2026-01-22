#pragma once
// Console3 - TerminalBuffer.h
// Terminal screen buffer with scrollback support
//
// Manages the terminal's cell grid, tracks dirty lines for efficient
// rendering, and maintains a scrollback history buffer.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <bitset>
#include <optional>

namespace Console3::Core {

/// Cell attributes packed into a single byte
struct CellAttributes {
    uint8_t bold : 1 = 0;
    uint8_t italic : 1 = 0;
    uint8_t underline : 2 = 0;      ///< 0=none, 1=single, 2=double, 3=curly
    uint8_t blink : 1 = 0;
    uint8_t reverse : 1 = 0;
    uint8_t strikethrough : 1 = 0;
    uint8_t conceal : 1 = 0;

    bool operator==(const CellAttributes& other) const noexcept = default;
};

/// 24-bit RGB color with type flags
struct CellColor {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t flags = 0;  ///< Bit 0: isDefault, Bit 1: isIndexed
    
    static constexpr uint8_t FLAG_DEFAULT = 0x01;
    static constexpr uint8_t FLAG_INDEXED = 0x02;

    [[nodiscard]] bool IsDefault() const noexcept { return (flags & FLAG_DEFAULT) != 0; }
    [[nodiscard]] bool IsIndexed() const noexcept { return (flags & FLAG_INDEXED) != 0; }

    /// Create a default color
    static CellColor Default() {
        CellColor c;
        c.flags = FLAG_DEFAULT;
        return c;
    }

    /// Create an RGB color
    static CellColor Rgb(uint8_t r, uint8_t g, uint8_t b) {
        CellColor c;
        c.r = r;
        c.g = g;
        c.b = b;
        c.flags = 0;
        return c;
    }

    /// Create an indexed palette color (index stored in r)
    static CellColor Indexed(uint8_t index) {
        CellColor c;
        c.r = index;
        c.flags = FLAG_INDEXED;
        return c;
    }

    bool operator==(const CellColor& other) const noexcept = default;
};

/// A single terminal cell
struct Cell {
    uint32_t charCode = U' ';       ///< Primary UTF-32 codepoint
    uint32_t combining[3] = {0};    ///< Up to 3 combining characters
    CellColor fg = CellColor::Default();
    CellColor bg = CellColor::Default();
    CellAttributes attrs{};
    uint8_t width = 1;              ///< Cell width (1 or 2 for wide chars)

    /// Reset to default empty cell
    void Clear() {
        charCode = U' ';
        combining[0] = combining[1] = combining[2] = 0;
        fg = CellColor::Default();
        bg = CellColor::Default();
        attrs = CellAttributes{};
        width = 1;
    }

    /// Check if cell has combining characters
    [[nodiscard]] bool HasCombining() const noexcept {
        return combining[0] != 0;
    }

    bool operator==(const Cell& other) const noexcept = default;
};

/// A row of cells
using Row = std::vector<Cell>;

/// Configuration for the terminal buffer
struct TerminalBufferConfig {
    int rows = 25;
    int cols = 80;
    size_t scrollbackLines = 10000;  ///< Maximum scrollback history lines
};

/// Terminal buffer with scrollback support and dirty tracking
class TerminalBuffer {
public:
    explicit TerminalBuffer(const TerminalBufferConfig& config = {});
    ~TerminalBuffer() = default;

    // Non-copyable but movable
    TerminalBuffer(const TerminalBuffer&) = delete;
    TerminalBuffer& operator=(const TerminalBuffer&) = delete;
    TerminalBuffer(TerminalBuffer&&) noexcept = default;
    TerminalBuffer& operator=(TerminalBuffer&&) noexcept = default;

    // ========================================================================
    // Size Management
    // ========================================================================

    /// Get current terminal dimensions
    [[nodiscard]] int GetRows() const noexcept { return m_rows; }
    [[nodiscard]] int GetCols() const noexcept { return m_cols; }

    /// Resize the terminal buffer
    /// @param rows New row count
    /// @param cols New column count
    void Resize(int rows, int cols);

    // ========================================================================
    // Cell Access
    // ========================================================================

    /// Get a cell at the specified position (0-indexed)
    /// @param row Row index
    /// @param col Column index
    /// @return Reference to the cell
    [[nodiscard]] Cell& GetCell(int row, int col);
    [[nodiscard]] const Cell& GetCell(int row, int col) const;

    /// Set a cell at the specified position
    void SetCell(int row, int col, const Cell& cell);

    /// Set just the character at a position
    void SetChar(int row, int col, uint32_t charCode, int width = 1);

    /// Clear a cell to default
    void ClearCell(int row, int col);

    /// Clear a range of cells in a row
    void ClearRange(int row, int startCol, int endCol);

    /// Clear entire row
    void ClearRow(int row);

    /// Clear entire screen
    void ClearScreen();

    /// Get a row by index
    [[nodiscard]] Row& GetRow(int row);
    [[nodiscard]] const Row& GetRow(int row) const;

    // ========================================================================
    // Scrolling
    // ========================================================================

    /// Scroll the screen content
    /// @param lines Number of lines to scroll (positive = up, negative = down)
    /// @param top Top of scroll region (inclusive)
    /// @param bottom Bottom of scroll region (exclusive)
    void Scroll(int lines, int top = 0, int bottom = -1);

    /// Push current top line to scrollback and scroll up
    void ScrollUp();

    /// Pop line from scrollback and scroll down
    void ScrollDown();

    // ========================================================================
    // Scrollback Buffer
    // ========================================================================

    /// Get number of lines in scrollback
    [[nodiscard]] size_t GetScrollbackSize() const noexcept;

    /// Get a line from scrollback (0 = most recent)
    [[nodiscard]] const Row* GetScrollbackLine(size_t index) const;

    /// Clear scrollback buffer
    void ClearScrollback();

    /// Get maximum scrollback size
    [[nodiscard]] size_t GetMaxScrollback() const noexcept { return m_maxScrollback; }

    /// Set maximum scrollback size
    void SetMaxScrollback(size_t lines);

    // ========================================================================
    // Dirty Tracking
    // ========================================================================

    /// Mark a row as dirty (needs redraw)
    void MarkDirty(int row);

    /// Mark a range of rows as dirty
    void MarkDirtyRange(int startRow, int endRow);

    /// Mark entire screen as dirty
    void MarkAllDirty();

    /// Check if a row is dirty
    [[nodiscard]] bool IsDirty(int row) const;

    /// Get list of dirty rows
    [[nodiscard]] std::vector<int> GetDirtyRows() const;

    /// Clear all dirty flags
    void ClearDirty();

    /// Check if any rows are dirty
    [[nodiscard]] bool HasDirty() const noexcept;

    // ========================================================================
    // Text Extraction
    // ========================================================================

    /// Get text content of a row as UTF-8
    [[nodiscard]] std::string GetRowText(int row) const;

    /// Get text content of a rectangular region
    [[nodiscard]] std::string GetRegionText(int startRow, int startCol, 
                                             int endRow, int endCol) const;

    /// Get all visible text
    [[nodiscard]] std::string GetAllText() const;

private:
    /// Ensure row index is valid
    void ValidateRow(int row) const;

    /// Ensure column index is valid
    void ValidateCol(int col) const;

    /// Create an empty row with default cells
    [[nodiscard]] Row CreateEmptyRow() const;

    /// Trim scrollback to max size
    void TrimScrollback();

private:
    int m_rows;
    int m_cols;
    size_t m_maxScrollback;

    // Main screen buffer
    std::vector<Row> m_screen;

    // Scrollback history (front = most recent)
    std::deque<Row> m_scrollback;

    // Dirty line tracking (one bit per row)
    std::vector<bool> m_dirty;

    // Static empty cell for out-of-bounds access
    static const Cell s_emptyCell;
};

} // namespace Console3::Core
