// Console3 - TerminalBuffer.cpp
// Terminal screen buffer implementation

#include "Core/TerminalBuffer.h"
#include <algorithm>
#include <stdexcept>
#include <codecvt>
#include <locale>

namespace Console3::Core {

// Static empty cell instance
const Cell TerminalBuffer::s_emptyCell{};

TerminalBuffer::TerminalBuffer(const TerminalBufferConfig& config)
    : m_rows(config.rows)
    , m_cols(config.cols)
    , m_maxScrollback(config.scrollbackLines) {
    
    if (m_rows <= 0 || m_cols <= 0) {
        throw std::invalid_argument("Terminal dimensions must be positive");
    }

    // Initialize screen buffer
    m_screen.resize(m_rows);
    for (auto& row : m_screen) {
        row = CreateEmptyRow();
    }

    // Initialize dirty tracking
    m_dirty.resize(m_rows, true);  // All rows dirty initially
}

// ============================================================================
// Size Management
// ============================================================================

void TerminalBuffer::Resize(int rows, int cols) {
    if (rows <= 0 || cols <= 0) {
        return;
    }

    // Handle row changes
    if (rows != m_rows) {
        if (rows > m_rows) {
            // Add new rows at the bottom
            for (int i = m_rows; i < rows; ++i) {
                m_screen.push_back(CreateEmptyRow());
            }
        } else {
            // Push excess rows to scrollback, then remove
            for (int i = 0; i < m_rows - rows; ++i) {
                if (!m_screen.empty()) {
                    m_scrollback.push_front(std::move(m_screen.front()));
                    m_screen.erase(m_screen.begin());
                }
            }
            TrimScrollback();
        }
        m_rows = rows;
    }

    // Handle column changes
    if (cols != m_cols) {
        for (auto& row : m_screen) {
            if (static_cast<int>(row.size()) < cols) {
                // Expand row
                row.resize(cols);
            } else if (static_cast<int>(row.size()) > cols) {
                // Shrink row
                row.resize(cols);
            }
        }
        m_cols = cols;
    }

    // Resize dirty tracking
    m_dirty.resize(m_rows, true);
    MarkAllDirty();
}

// ============================================================================
// Cell Access
// ============================================================================

Cell& TerminalBuffer::GetCell(int row, int col) {
    ValidateRow(row);
    ValidateCol(col);
    return m_screen[row][col];
}

const Cell& TerminalBuffer::GetCell(int row, int col) const {
    if (row < 0 || row >= m_rows || col < 0 || col >= m_cols) {
        return s_emptyCell;
    }
    return m_screen[row][col];
}

void TerminalBuffer::SetCell(int row, int col, const Cell& cell) {
    if (row >= 0 && row < m_rows && col >= 0 && col < m_cols) {
        m_screen[row][col] = cell;
        MarkDirty(row);
    }
}

void TerminalBuffer::SetChar(int row, int col, uint32_t charCode, int width) {
    if (row >= 0 && row < m_rows && col >= 0 && col < m_cols) {
        auto& cell = m_screen[row][col];
        cell.charCode = charCode;
        cell.width = static_cast<uint8_t>(width);
        cell.combining[0] = cell.combining[1] = cell.combining[2] = 0;
        MarkDirty(row);
    }
}

void TerminalBuffer::ClearCell(int row, int col) {
    if (row >= 0 && row < m_rows && col >= 0 && col < m_cols) {
        m_screen[row][col].Clear();
        MarkDirty(row);
    }
}

void TerminalBuffer::ClearRange(int row, int startCol, int endCol) {
    if (row < 0 || row >= m_rows) return;
    
    startCol = std::max(0, startCol);
    endCol = std::min(m_cols, endCol);
    
    for (int col = startCol; col < endCol; ++col) {
        m_screen[row][col].Clear();
    }
    MarkDirty(row);
}

void TerminalBuffer::ClearRow(int row) {
    ClearRange(row, 0, m_cols);
}

void TerminalBuffer::ClearScreen() {
    for (int row = 0; row < m_rows; ++row) {
        ClearRow(row);
    }
}

Row& TerminalBuffer::GetRow(int row) {
    ValidateRow(row);
    return m_screen[row];
}

const Row& TerminalBuffer::GetRow(int row) const {
    ValidateRow(row);
    return m_screen[row];
}

// ============================================================================
// Scrolling
// ============================================================================

void TerminalBuffer::Scroll(int lines, int top, int bottom) {
    if (bottom < 0) {
        bottom = m_rows;
    }
    
    top = std::clamp(top, 0, m_rows - 1);
    bottom = std::clamp(bottom, top + 1, m_rows);
    
    if (lines == 0 || top >= bottom - 1) {
        return;
    }

    if (lines > 0) {
        // Scroll up: move content up, blank lines appear at bottom
        for (int i = 0; i < lines && top < bottom - 1; ++i) {
            // Push top line to scrollback if at screen top
            if (top == 0) {
                m_scrollback.push_front(std::move(m_screen[top]));
            }
            
            // Shift lines up within region
            for (int row = top; row < bottom - 1; ++row) {
                m_screen[row] = std::move(m_screen[row + 1]);
            }
            
            // Clear the bottom line
            m_screen[bottom - 1] = CreateEmptyRow();
        }
        TrimScrollback();
    } else {
        // Scroll down: move content down, blank lines appear at top
        lines = -lines;
        for (int i = 0; i < lines && top < bottom - 1; ++i) {
            // Shift lines down within region
            for (int row = bottom - 1; row > top; --row) {
                m_screen[row] = std::move(m_screen[row - 1]);
            }
            
            // Clear the top line (or restore from scrollback)
            if (top == 0 && !m_scrollback.empty()) {
                m_screen[top] = std::move(m_scrollback.front());
                m_scrollback.pop_front();
            } else {
                m_screen[top] = CreateEmptyRow();
            }
        }
    }

    // Mark affected region as dirty
    MarkDirtyRange(top, bottom);
}

void TerminalBuffer::ScrollUp() {
    Scroll(1, 0, m_rows);
}

void TerminalBuffer::ScrollDown() {
    Scroll(-1, 0, m_rows);
}

// ============================================================================
// Scrollback Buffer
// ============================================================================

size_t TerminalBuffer::GetScrollbackSize() const noexcept {
    return m_scrollback.size();
}

const Row* TerminalBuffer::GetScrollbackLine(size_t index) const {
    if (index < m_scrollback.size()) {
        return &m_scrollback[index];
    }
    return nullptr;
}

void TerminalBuffer::ClearScrollback() {
    m_scrollback.clear();
}

void TerminalBuffer::SetMaxScrollback(size_t lines) {
    m_maxScrollback = lines;
    TrimScrollback();
}

// ============================================================================
// Dirty Tracking
// ============================================================================

void TerminalBuffer::MarkDirty(int row) {
    if (row >= 0 && row < m_rows) {
        m_dirty[row] = true;
    }
}

void TerminalBuffer::MarkDirtyRange(int startRow, int endRow) {
    startRow = std::max(0, startRow);
    endRow = std::min(m_rows, endRow);
    for (int row = startRow; row < endRow; ++row) {
        m_dirty[row] = true;
    }
}

void TerminalBuffer::MarkAllDirty() {
    std::fill(m_dirty.begin(), m_dirty.end(), true);
}

bool TerminalBuffer::IsDirty(int row) const {
    if (row >= 0 && row < m_rows) {
        return m_dirty[row];
    }
    return false;
}

std::vector<int> TerminalBuffer::GetDirtyRows() const {
    std::vector<int> result;
    result.reserve(m_rows);
    for (int row = 0; row < m_rows; ++row) {
        if (m_dirty[row]) {
            result.push_back(row);
        }
    }
    return result;
}

void TerminalBuffer::ClearDirty() {
    std::fill(m_dirty.begin(), m_dirty.end(), false);
}

bool TerminalBuffer::HasDirty() const noexcept {
    return std::any_of(m_dirty.begin(), m_dirty.end(), [](bool d) { return d; });
}

// ============================================================================
// Text Extraction
// ============================================================================

std::string TerminalBuffer::GetRowText(int row) const {
    if (row < 0 || row >= m_rows) {
        return {};
    }

    std::string result;
    result.reserve(m_cols * 4);  // UTF-8 can be up to 4 bytes per char

    for (const auto& cell : m_screen[row]) {
        // Skip continuation cells (part of wide character)
        if (cell.width == 0) {
            continue;
        }

        // Convert UTF-32 to UTF-8
        uint32_t cp = cell.charCode;
        if (cp < 0x80) {
            result += static_cast<char>(cp);
        } else if (cp < 0x800) {
            result += static_cast<char>(0xC0 | (cp >> 6));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            result += static_cast<char>(0xE0 | (cp >> 12));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            result += static_cast<char>(0xF0 | (cp >> 18));
            result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        }

        // Add combining characters
        for (uint32_t comb : cell.combining) {
            if (comb == 0) break;
            // Same UTF-8 encoding for combining chars
            if (comb < 0x80) {
                result += static_cast<char>(comb);
            } else if (comb < 0x800) {
                result += static_cast<char>(0xC0 | (comb >> 6));
                result += static_cast<char>(0x80 | (comb & 0x3F));
            } else if (comb < 0x10000) {
                result += static_cast<char>(0xE0 | (comb >> 12));
                result += static_cast<char>(0x80 | ((comb >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (comb & 0x3F));
            } else {
                result += static_cast<char>(0xF0 | (comb >> 18));
                result += static_cast<char>(0x80 | ((comb >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((comb >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (comb & 0x3F));
            }
        }
    }

    // Trim trailing spaces
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return result;
}

std::string TerminalBuffer::GetRegionText(int startRow, int startCol,
                                          int endRow, int endCol) const {
    std::string result;
    
    startRow = std::clamp(startRow, 0, m_rows - 1);
    endRow = std::clamp(endRow, 0, m_rows - 1);
    
    for (int row = startRow; row <= endRow; ++row) {
        std::string rowText = GetRowText(row);
        
        int colStart = (row == startRow) ? startCol : 0;
        int colEnd = (row == endRow) ? endCol : m_cols;
        
        // Note: This is a simplified extraction; proper UTF-8 column handling
        // would require more complex logic
        if (!rowText.empty()) {
            result += rowText;
        }
        
        if (row < endRow) {
            result += '\n';
        }
    }
    
    return result;
}

std::string TerminalBuffer::GetAllText() const {
    return GetRegionText(0, 0, m_rows - 1, m_cols);
}

// ============================================================================
// Private Helpers
// ============================================================================

void TerminalBuffer::ValidateRow(int row) const {
    if (row < 0 || row >= m_rows) {
        throw std::out_of_range("Row index out of range: " + std::to_string(row));
    }
}

void TerminalBuffer::ValidateCol(int col) const {
    if (col < 0 || col >= m_cols) {
        throw std::out_of_range("Column index out of range: " + std::to_string(col));
    }
}

Row TerminalBuffer::CreateEmptyRow() const {
    Row row(m_cols);
    for (auto& cell : row) {
        cell.Clear();
    }
    return row;
}

void TerminalBuffer::TrimScrollback() {
    while (m_scrollback.size() > m_maxScrollback) {
        m_scrollback.pop_back();
    }
}

} // namespace Console3::Core
