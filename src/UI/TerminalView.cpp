// Console3 - TerminalView.cpp
// Terminal rendering window implementation

#include "UI/TerminalView.h"
#include <algorithm>
#include <imm.h>

#pragma comment(lib, "imm32.lib")

namespace Console3::UI {

// ============================================================================
// Selection Implementation
// ============================================================================

bool Selection::Contains(int row, int col) const {
    if (!active) return false;
    
    int sRow = startRow, sCol = startCol;
    int eRow = endRow, eCol = endCol;
    
    // Normalize
    if (sRow > eRow || (sRow == eRow && sCol > eCol)) {
        std::swap(sRow, eRow);
        std::swap(sCol, eCol);
    }
    
    if (row < sRow || row > eRow) return false;
    if (row == sRow && row == eRow) return col >= sCol && col < eCol;
    if (row == sRow) return col >= sCol;
    if (row == eRow) return col < eCol;
    return true;
}

void Selection::Normalize() {
    if (startRow > endRow || (startRow == endRow && startCol > endCol)) {
        std::swap(startRow, endRow);
        std::swap(startCol, endCol);
    }
}

std::wstring Selection::GetText(const Core::TerminalBuffer& buffer) const {
    if (!active) return L"";
    
    std::wstring result;
    int sRow = startRow, sCol = startCol;
    int eRow = endRow, eCol = endCol;
    
    // Normalize
    if (sRow > eRow || (sRow == eRow && sCol > eCol)) {
        std::swap(sRow, eRow);
        std::swap(sCol, eCol);
    }
    
    for (int row = sRow; row <= eRow; ++row) {
        int colStart = (row == sRow) ? sCol : 0;
        int colEnd = (row == eRow) ? eCol : buffer.GetCols();
        
        for (int col = colStart; col < colEnd; ++col) {
            const auto& cell = buffer.GetCell(row, col);
            if (cell.width == 0) continue;  // Skip continuation cells
            
            uint32_t cp = cell.charCode;
            if (cp <= 0xFFFF) {
                result += static_cast<wchar_t>(cp);
            } else {
                // Surrogate pair
                cp -= 0x10000;
                result += static_cast<wchar_t>(0xD800 | (cp >> 10));
                result += static_cast<wchar_t>(0xDC00 | (cp & 0x3FF));
            }
        }
        
        if (row < eRow) {
            result += L'\n';
        }
    }
    
    return result;
}

// ============================================================================
// TerminalView Implementation
// ============================================================================

TerminalView::TerminalView() = default;

TerminalView::~TerminalView() = default;

bool TerminalView::Initialize(
    ID2D1Factory1* d2dFactory,
    IDWriteFactory1* dwriteFactory,
    Core::TerminalBuffer* buffer
) {
    if (!m_hWnd || !d2dFactory || !dwriteFactory) {
        return false;
    }

    m_buffer = buffer;

    // Create renderer
    m_renderer = std::make_unique<D2DRenderer>();
    
    RendererConfig config;
    config.hwnd = m_hWnd;
    config.d2dFactory = d2dFactory;
    config.dwriteFactory = dwriteFactory;
    config.backgroundColor = m_defaultBg;
    
    if (!m_renderer->Initialize(config)) {
        return false;
    }

    // Set default font
    if (!m_renderer->SetFont(L"Consolas", 12.0f)) {
        return false;
    }

    return true;
}

void TerminalView::SetBuffer(Core::TerminalBuffer* buffer) {
    m_buffer = buffer;
    Invalidate();
}

void TerminalView::SetVTerm(Emulation::VTermWrapper* vterm) {
    m_vterm = vterm;
}

void TerminalView::SetKeyboardInputCallback(KeyboardInputCallback callback) {
    m_keyboardCallback = std::move(callback);
}

bool TerminalView::SetFont(const std::wstring& fontName, float fontSize) {
    if (!m_renderer) return false;
    
    bool result = m_renderer->SetFont(fontName, fontSize);
    if (result) {
        Invalidate();
    }
    return result;
}

void TerminalView::SetCursorStyle(CursorStyle style) {
    m_cursorStyle = style;
    Invalidate();
}

void TerminalView::SetCursorBlinkRate(UINT milliseconds) {
    m_cursorBlinkRate = milliseconds;
    
    if (m_hWnd && m_hasFocus) {
        KillTimer(TIMER_CURSOR_BLINK);
        if (milliseconds > 0) {
            SetTimer(TIMER_CURSOR_BLINK, milliseconds);
        }
    }
}

void TerminalView::SetCursorVisible(bool visible) {
    m_cursorVisible = visible;
    Invalidate();
}

int TerminalView::GetTerminalRows() const {
    if (!m_renderer) return 25;
    
    CRect rect;
    GetClientRect(&rect);
    float cellHeight = m_renderer->GetCellHeight();
    return cellHeight > 0 ? static_cast<int>(rect.Height() / cellHeight) : 25;
}

int TerminalView::GetTerminalCols() const {
    if (!m_renderer) return 80;
    
    CRect rect;
    GetClientRect(&rect);
    float cellWidth = m_renderer->GetCellWidth();
    return cellWidth > 0 ? static_cast<int>(rect.Width() / cellWidth) : 80;
}

void TerminalView::Invalidate() {
    if (m_hWnd) {
        ::InvalidateRect(m_hWnd, nullptr, FALSE);
    }
}

void TerminalView::CopyToClipboard() {
    if (!m_selection.active || !m_buffer) return;
    
    std::wstring text = m_selection.GetText(*m_buffer);
    if (text.empty()) return;
    
    if (OpenClipboard()) {
        EmptyClipboard();
        
        size_t size = (text.length() + 1) * sizeof(wchar_t);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hGlobal) {
            wchar_t* dest = static_cast<wchar_t*>(GlobalLock(hGlobal));
            if (dest) {
                wcscpy_s(dest, text.length() + 1, text.c_str());
                GlobalUnlock(hGlobal);
                SetClipboardData(CF_UNICODETEXT, hGlobal);
            }
        }
        CloseClipboard();
    }
}

void TerminalView::PasteFromClipboard() {
    if (!m_keyboardCallback) return;
    
    if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard()) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(hData));
            if (text) {
                // Convert to UTF-8
                int utf8Len = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
                if (utf8Len > 0) {
                    std::string utf8(utf8Len, '\0');
                    WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8.data(), utf8Len, nullptr, nullptr);
                    
                    // Bracketed paste mode
                    if (m_bracketedPasteMode) {
                        m_keyboardCallback("\x1b[200~", 6);  // Start bracket
                        m_keyboardCallback(utf8.c_str(), utf8.length() - 1);
                        m_keyboardCallback("\x1b[201~", 6);  // End bracket
                    } else {
                        m_keyboardCallback(utf8.c_str(), utf8.length() - 1);  // Exclude null
                    }
                }
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }
}

void TerminalView::ClearSelection() {
    m_selection.active = false;
    Invalidate();
}

// ============================================================================
// Message Handlers
// ============================================================================

int TerminalView::OnCreate(LPCREATESTRUCT /*lpCreateStruct*/) {
    return 0;
}

void TerminalView::OnDestroy() {
    KillTimer(TIMER_CURSOR_BLINK);
    m_renderer.reset();
}

void TerminalView::OnSize(UINT nType, CSize size) {
    if (nType == SIZE_MINIMIZED) return;
    
    if (m_renderer && m_renderer->IsInitialized()) {
        m_renderer->Resize(size.cx, size.cy);
        Invalidate();
    }
}

void TerminalView::OnPaint(CDCHandle /*dc*/) {
    Render();
    ValidateRect(nullptr);
}

LRESULT TerminalView::OnEraseBkgnd(CDCHandle /*dc*/) {
    return 1;  // We handle background in Render()
}

void TerminalView::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == TIMER_CURSOR_BLINK) {
        m_cursorBlinkState = !m_cursorBlinkState;
        Invalidate();
    }
}

void TerminalView::OnSetFocus(CWindow /*wndOld*/) {
    m_hasFocus = true;
    m_cursorBlinkState = true;
    
    if (m_cursorBlinkRate > 0) {
        SetTimer(TIMER_CURSOR_BLINK, m_cursorBlinkRate);
    }
    
    Invalidate();
}

void TerminalView::OnKillFocus(CWindow /*wndNew*/) {
    m_hasFocus = false;
    KillTimer(TIMER_CURSOR_BLINK);
    Invalidate();
}

void TerminalView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
    (void)nRepCnt;
    SendKeyToTerminal(nChar, nFlags & 0xFF, true);
}

void TerminalView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) {
    (void)nRepCnt;
    (void)nChar;
    (void)nFlags;
    // Most terminals don't need key up events
}

void TerminalView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
    (void)nRepCnt;
    (void)nFlags;
    
    // Skip control characters handled by OnKeyDown
    if (nChar < 32 && nChar != '\r' && nChar != '\t') {
        return;
    }
    
    SendCharToTerminal(static_cast<wchar_t>(nChar));
}

void TerminalView::OnLButtonDown(UINT nFlags, CPoint point) {
    SetCapture();
    
    m_selection.startRow = PixelToRow(point.y);
    m_selection.startCol = PixelToCol(point.x);
    m_selection.endRow = m_selection.startRow;
    m_selection.endCol = m_selection.startCol;
    m_selection.active = false;
    m_isSelecting = true;
    
    SetFocus();
}

void TerminalView::OnLButtonUp(UINT nFlags, CPoint point) {
    (void)nFlags;
    
    ReleaseCapture();
    m_isSelecting = false;
    
    if (m_selection.active) {
        m_selection.Normalize();
    }
}

void TerminalView::OnMouseMove(UINT nFlags, CPoint point) {
    if (m_isSelecting && (nFlags & MK_LBUTTON)) {
        m_selection.endRow = PixelToRow(point.y);
        m_selection.endCol = PixelToCol(point.x);
        m_selection.active = true;
        Invalidate();
    }
}

BOOL TerminalView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
    (void)nFlags;
    (void)pt;
    
    // Mouse wheel reporting in SGR mode
    if (m_mouseMode == MouseMode::SGR || m_mouseMode == MouseMode::Normal) {
        int row = PixelToRow(pt.y);
        int col = PixelToCol(pt.x);
        int button = (zDelta > 0) ? 64 : 65;  // 64 = scroll up, 65 = scroll down
        SendMouseReport(button, row, col, true);
        return TRUE;
    }
    
    // Scroll 3 lines per notch
    int lines = (zDelta > 0) ? 3 : -3;
    m_scrollOffset = std::clamp(
        m_scrollOffset + lines, 
        0, 
        static_cast<int>(m_buffer ? m_buffer->GetScrollbackSize() : 0)
    );
    
    Invalidate();
    return TRUE;
}

// ============================================================================
// Rendering
// ============================================================================

void TerminalView::Render() {
    if (!m_renderer || !m_renderer->IsInitialized() || !m_buffer) {
        return;
    }

    if (!m_renderer->BeginDraw()) {
        return;
    }

    m_renderer->Clear();

    // Render each row
    int rows = m_buffer->GetRows();
    for (int row = 0; row < rows; ++row) {
        RenderRow(row);
    }

    // Render selection
    if (m_selection.active) {
        RenderSelection();
    }

    // Render cursor
    if (m_cursorVisible && m_hasFocus && (m_cursorBlinkState || m_cursorBlinkRate == 0)) {
        RenderCursor();
    }

    m_renderer->EndDraw();
}

void TerminalView::RenderRow(int row) {
    if (!m_buffer || !m_renderer) return;
    
    float cellWidth = m_renderer->GetCellWidth();
    float cellHeight = m_renderer->GetCellHeight();
    float y = RowToPixel(row);
    
    int cols = m_buffer->GetCols();
    for (int col = 0; col < cols; ++col) {
        const auto& cell = m_buffer->GetCell(row, col);
        
        // Skip continuation cells
        if (cell.width == 0) continue;
        
        float x = ColToPixel(col);
        
        // Draw background
        Color bgColor = m_defaultBg;
        if (!cell.bg.IsDefault()) {
            bgColor = Color::FromRgb(cell.bg.r, cell.bg.g, cell.bg.b);
        }
        
        // Check if in selection
        if (m_selection.Contains(row, col)) {
            bgColor = m_selectionColor;
        }
        
        if (!cell.bg.IsDefault() || m_selection.Contains(row, col)) {
            m_renderer->FillRect(x, y, cellWidth * cell.width, cellHeight, bgColor);
        }
        
        // Draw character
        if (cell.charCode != U' ') {
            Color fgColor = m_defaultFg;
            if (!cell.fg.IsDefault()) {
                fgColor = Color::FromRgb(cell.fg.r, cell.fg.g, cell.fg.b);
            }
            
            m_renderer->DrawChar(cell.charCode, x, y, fgColor);
        }
    }
}

void TerminalView::RenderCursor() {
    if (!m_buffer || !m_renderer || !m_vterm) return;
    
    int cursorRow, cursorCol;
    m_vterm->GetCursorPos(cursorRow, cursorCol);
    
    float x = ColToPixel(cursorCol);
    float y = RowToPixel(cursorRow);
    float cellWidth = m_renderer->GetCellWidth();
    float cellHeight = m_renderer->GetCellHeight();
    
    switch (m_cursorStyle) {
        case CursorStyle::Block:
            m_renderer->FillRect(x, y, cellWidth, cellHeight, m_cursorColor);
            break;
        case CursorStyle::Underline:
            m_renderer->FillRect(x, y + cellHeight - 2, cellWidth, 2, m_cursorColor);
            break;
        case CursorStyle::Bar:
            m_renderer->FillRect(x, y, 2, cellHeight, m_cursorColor);
            break;
    }
}

void TerminalView::RenderSelection() {
    // Selection is rendered per-cell in RenderRow
}

// ============================================================================
// Input Handling
// ============================================================================

void TerminalView::SendKeyToTerminal(UINT vkey, UINT /*scanCode*/, bool keyDown) {
    if (!keyDown || !m_keyboardCallback) return;
    
    // Get modifier state
    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    
    // Handle Ctrl+Key combinations (send control characters)
    if (ctrl && !alt) {
        // Ctrl+C = 0x03, Ctrl+D = 0x04, etc.
        if (vkey >= 'A' && vkey <= 'Z') {
            char ctrlChar = static_cast<char>(vkey - 'A' + 1);
            m_keyboardCallback(&ctrlChar, 1);
            return;
        }
        // Ctrl+[ = ESC (0x1B), Ctrl+\ = 0x1C, Ctrl+] = 0x1D
        if (vkey == VK_OEM_4) { // [
            char esc = 0x1B;
            m_keyboardCallback(&esc, 1);
            return;
        }
    }
    
    // Alt+Key combinations (send ESC + key)
    if (alt && !ctrl) {
        if (vkey >= 'A' && vkey <= 'Z') {
            char seq[2] = { 0x1B, static_cast<char>(shift ? vkey : (vkey + 32)) };
            m_keyboardCallback(seq, 2);
            return;
        }
    }
    
    // Map virtual keys to escape sequences
    const char* seq = nullptr;
    char modSeq[16];
    
    // Calculate modifier parameter for CSI sequences
    // 1 = none, 2 = shift, 3 = alt, 4 = shift+alt, 5 = ctrl, 6 = shift+ctrl, 7 = alt+ctrl, 8 = shift+alt+ctrl
    int modParam = 1;
    if (shift) modParam += 1;
    if (alt) modParam += 2;
    if (ctrl) modParam += 4;
    
    bool hasModifier = (modParam > 1);
    
    switch (vkey) {
        case VK_UP:
            if (hasModifier) {
                snprintf(modSeq, sizeof(modSeq), "\x1b[1;%dA", modParam);
                seq = modSeq;
            } else {
                seq = "\x1b[A";
            }
            break;
        case VK_DOWN:
            if (hasModifier) {
                snprintf(modSeq, sizeof(modSeq), "\x1b[1;%dB", modParam);
                seq = modSeq;
            } else {
                seq = "\x1b[B";
            }
            break;
        case VK_RIGHT:
            if (hasModifier) {
                snprintf(modSeq, sizeof(modSeq), "\x1b[1;%dC", modParam);
                seq = modSeq;
            } else {
                seq = "\x1b[C";
            }
            break;
        case VK_LEFT:
            if (hasModifier) {
                snprintf(modSeq, sizeof(modSeq), "\x1b[1;%dD", modParam);
                seq = modSeq;
            } else {
                seq = "\x1b[D";
            }
            break;
        case VK_HOME:   seq = hasModifier ? (snprintf(modSeq, sizeof(modSeq), "\x1b[1;%dH", modParam), modSeq) : "\x1b[H"; break;
        case VK_END:    seq = hasModifier ? (snprintf(modSeq, sizeof(modSeq), "\x1b[1;%dF", modParam), modSeq) : "\x1b[F"; break;
        case VK_INSERT: seq = "\x1b[2~"; break;
        case VK_DELETE: seq = "\x1b[3~"; break;
        case VK_PRIOR:  seq = "\x1b[5~"; break;  // Page Up
        case VK_NEXT:   seq = "\x1b[6~"; break;  // Page Down
        case VK_F1:     seq = "\x1bOP"; break;
        case VK_F2:     seq = "\x1bOQ"; break;
        case VK_F3:     seq = "\x1bOR"; break;
        case VK_F4:     seq = "\x1bOS"; break;
        case VK_F5:     seq = "\x1b[15~"; break;
        case VK_F6:     seq = "\x1b[17~"; break;
        case VK_F7:     seq = "\x1b[18~"; break;
        case VK_F8:     seq = "\x1b[19~"; break;
        case VK_F9:     seq = "\x1b[20~"; break;
        case VK_F10:    seq = "\x1b[21~"; break;
        case VK_F11:    seq = "\x1b[23~"; break;
        case VK_F12:    seq = "\x1b[24~"; break;
        case VK_ESCAPE: seq = "\x1b"; break;
        case VK_TAB:
            if (shift) {
                seq = "\x1b[Z";  // Shift+Tab = reverse tab
            }
            break;
        default: return;  // Handled by OnChar for regular keys
    }
    
    if (seq) {
        m_keyboardCallback(seq, strlen(seq));
    }
}

void TerminalView::SendCharToTerminal(wchar_t ch) {
    if (!m_keyboardCallback) return;
    
    // Convert to UTF-8
    char utf8[4];
    int len = 0;
    
    if (ch == '\r') {
        utf8[0] = '\r';
        len = 1;
    } else if (ch < 0x80) {
        utf8[0] = static_cast<char>(ch);
        len = 1;
    } else if (ch < 0x800) {
        utf8[0] = static_cast<char>(0xC0 | (ch >> 6));
        utf8[1] = static_cast<char>(0x80 | (ch & 0x3F));
        len = 2;
    } else {
        utf8[0] = static_cast<char>(0xE0 | (ch >> 12));
        utf8[1] = static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
        utf8[2] = static_cast<char>(0x80 | (ch & 0x3F));
        len = 3;
    }
    
    m_keyboardCallback(utf8, len);
}

// ============================================================================
// Coordinate Conversion
// ============================================================================

int TerminalView::PixelToRow(int y) const {
    if (!m_renderer) return 0;
    float cellHeight = m_renderer->GetCellHeight();
    return cellHeight > 0 ? static_cast<int>(y / cellHeight) : 0;
}

int TerminalView::PixelToCol(int x) const {
    if (!m_renderer) return 0;
    float cellWidth = m_renderer->GetCellWidth();
    return cellWidth > 0 ? static_cast<int>(x / cellWidth) : 0;
}

float TerminalView::RowToPixel(int row) const {
    if (!m_renderer) return 0;
    return row * m_renderer->GetCellHeight();
}

float TerminalView::ColToPixel(int col) const {
    if (!m_renderer) return 0;
    return col * m_renderer->GetCellWidth();
}

// ============================================================================
// Mouse Reporting
// ============================================================================

void TerminalView::SendMouseReport(int button, int row, int col, bool pressed) {
    if (!m_keyboardCallback) return;
    
    char buf[32];
    
    switch (m_mouseMode) {
        case MouseMode::X10:
            // X10: Only button press, no modifiers
            if (pressed) {
                snprintf(buf, sizeof(buf), "\x1b[M%c%c%c",
                    static_cast<char>(32 + button),
                    static_cast<char>(32 + col + 1),
                    static_cast<char>(32 + row + 1));
                m_keyboardCallback(buf, strlen(buf));
            }
            break;
            
        case MouseMode::Normal:
            // Normal: Button press and release
            snprintf(buf, sizeof(buf), "\x1b[M%c%c%c",
                static_cast<char>(32 + (pressed ? button : 3)),  // 3 = release
                static_cast<char>(32 + col + 1),
                static_cast<char>(32 + row + 1));
            m_keyboardCallback(buf, strlen(buf));
            break;
            
        case MouseMode::SGR:
            // SGR: Extended format, supports large coordinates
            snprintf(buf, sizeof(buf), "\x1b[<%d;%d;%d%c",
                button,
                col + 1,
                row + 1,
                pressed ? 'M' : 'm');
            m_keyboardCallback(buf, strlen(buf));
            break;
            
        default:
            break;
    }
}

void TerminalView::SetMouseMode(MouseMode mode) {
    m_mouseMode = mode;
}

void TerminalView::SetBracketedPasteMode(bool enabled) {
    m_bracketedPasteMode = enabled;
}

// ============================================================================
// IME Support for CJK Input
// ============================================================================

LRESULT TerminalView::OnImeStartComposition(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    // Position the IME composition window near the cursor
    if (!m_renderer || !m_vterm) {
        bHandled = FALSE;
        return 0;
    }

    int cursorRow, cursorCol;
    m_vterm->GetCursorPos(cursorRow, cursorCol);
    
    float x = ColToPixel(cursorCol);
    float y = RowToPixel(cursorRow);
    
    HIMC hImc = ImmGetContext(m_hWnd);
    if (hImc) {
        COMPOSITIONFORM cf{};
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = static_cast<LONG>(x);
        cf.ptCurrentPos.y = static_cast<LONG>(y);
        ImmSetCompositionWindow(hImc, &cf);
        
        // Set the font for the composition window
        LOGFONTW lf{};
        lf.lfHeight = static_cast<LONG>(m_renderer->GetCellHeight());
        wcscpy_s(lf.lfFaceName, L"Consolas");
        ImmSetCompositionFontW(hImc, &lf);
        
        ImmReleaseContext(m_hWnd, hImc);
    }
    
    bHandled = FALSE;  // Let default processing continue
    return 0;
}

LRESULT TerminalView::OnImeComposition(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    // Handle IME composition result
    if (lParam & GCS_RESULTSTR) {
        HIMC hImc = ImmGetContext(m_hWnd);
        if (hImc) {
            // Get the result string length
            LONG len = ImmGetCompositionStringW(hImc, GCS_RESULTSTR, nullptr, 0);
            if (len > 0) {
                std::wstring result(len / sizeof(wchar_t), L'\0');
                ImmGetCompositionStringW(hImc, GCS_RESULTSTR, result.data(), len);
                
                // Send each character to the terminal
                for (wchar_t ch : result) {
                    SendCharToTerminal(ch);
                }
            }
            ImmReleaseContext(m_hWnd, hImc);
        }
        bHandled = TRUE;
        return 0;
    }
    
    bHandled = FALSE;
    return 0;
}

LRESULT TerminalView::OnImeEndComposition(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    // Composition ended, redraw to clear any inline preview
    Invalidate();
    bHandled = FALSE;
    return 0;
}

LRESULT TerminalView::OnImeChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
    // Handle IME characters (for systems that send WM_IME_CHAR instead of WM_CHAR)
    wchar_t ch = static_cast<wchar_t>(wParam);
    if (ch >= 32) {
        SendCharToTerminal(ch);
    }
    bHandled = TRUE;
    return 0;
}

} // namespace Console3::UI

