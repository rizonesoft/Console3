// Console3 - VTermWrapper.cpp
// libvterm C++ wrapper implementation

#include "Emulation/VTermWrapper.h"
#include <algorithm>
#include <cstring>

namespace Console3::Emulation {

// ============================================================================
// Helper Conversion Functions
// ============================================================================

TermColor TermColor::FromVTerm(const VTermColor& color) {
    TermColor result;
    
    if (VTERM_COLOR_IS_DEFAULT_FG(&color) || VTERM_COLOR_IS_DEFAULT_BG(&color)) {
        result.isDefault = true;
    } else if (VTERM_COLOR_IS_INDEXED(&color)) {
        result.isIndexed = true;
        result.isDefault = false;
        result.paletteIndex = color.indexed.idx;
    } else if (VTERM_COLOR_IS_RGB(&color)) {
        result.isDefault = false;
        result.isIndexed = false;
        result.r = color.rgb.red;
        result.g = color.rgb.green;
        result.b = color.rgb.blue;
    }
    
    return result;
}

CellAttrs CellAttrs::FromVTerm(const VTermScreenCellAttrs& attrs) {
    CellAttrs result;
    result.bold = attrs.bold != 0;
    result.italic = attrs.italic != 0;
    result.underline = attrs.underline != 0;
    result.underlineStyle = static_cast<uint8_t>(attrs.underline);
    result.blink = attrs.blink != 0;
    result.reverse = attrs.reverse != 0;
    result.strikethrough = attrs.strike != 0;
    result.conceal = attrs.conceal != 0;
    return result;
}

// ============================================================================
// VTermWrapper Implementation
// ============================================================================

VTermWrapper::VTermWrapper(int rows, int cols) {
    // Verify version compatibility
    VTERM_CHECK_VERSION;
    
    // Create the vterm instance
    m_vterm = vterm_new(rows, cols);
    if (!m_vterm) {
        throw std::runtime_error("Failed to create VTerm instance");
    }

    // Enable UTF-8 mode
    vterm_set_utf8(m_vterm, 1);

    // Set up output callback
    vterm_output_set_callback(m_vterm, &VTermWrapper::OnOutput, this);

    // Get the screen layer
    m_screen = vterm_obtain_screen(m_vterm);
    
    // Set up screen callbacks
    std::memset(&m_screenCallbacks, 0, sizeof(m_screenCallbacks));
    m_screenCallbacks.damage = &VTermWrapper::OnDamage;
    m_screenCallbacks.moverect = &VTermWrapper::OnMoveRect;
    m_screenCallbacks.movecursor = &VTermWrapper::OnMoveCursor;
    m_screenCallbacks.settermprop = &VTermWrapper::OnSetTermProp;
    m_screenCallbacks.bell = &VTermWrapper::OnBell;
    m_screenCallbacks.resize = &VTermWrapper::OnResize;
    m_screenCallbacks.sb_pushline = &VTermWrapper::OnScrollbackPush;
    m_screenCallbacks.sb_popline = &VTermWrapper::OnScrollbackPop;

    vterm_screen_set_callbacks(m_screen, &m_screenCallbacks, this);
    
    // Enable alternate screen buffer support
    vterm_screen_enable_altscreen(m_screen, 1);
    
    // Reset to initialize state
    vterm_screen_reset(m_screen, 1);
}

VTermWrapper::~VTermWrapper() {
    if (m_vterm) {
        vterm_free(m_vterm);
        m_vterm = nullptr;
    }
}

size_t VTermWrapper::InputWrite(const char* data, size_t length) {
    if (!m_vterm || !data || length == 0) {
        return 0;
    }
    return vterm_input_write(m_vterm, data, length);
}

void VTermWrapper::KeyboardUnichar(uint32_t codepoint, int modifiers) {
    if (m_vterm) {
        vterm_keyboard_unichar(m_vterm, codepoint, static_cast<VTermModifier>(modifiers));
    }
}

void VTermWrapper::KeyboardKey(int key, int modifiers) {
    if (m_vterm) {
        vterm_keyboard_key(m_vterm, static_cast<VTermKey>(key), static_cast<VTermModifier>(modifiers));
    }
}

void VTermWrapper::Resize(int rows, int cols) {
    if (m_vterm && rows > 0 && cols > 0) {
        vterm_set_size(m_vterm, rows, cols);
    }
}

void VTermWrapper::GetSize(int& rows, int& cols) const {
    if (m_vterm) {
        vterm_get_size(m_vterm, &rows, &cols);
    } else {
        rows = cols = 0;
    }
}

TermCell VTermWrapper::GetCell(int row, int col) const {
    TermCell result;
    
    if (!m_screen) {
        return result;
    }

    VTermPos pos{row, col};
    VTermScreenCell cell{};
    
    if (vterm_screen_get_cell(m_screen, pos, &cell)) {
        // Copy characters
        for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i] != 0; ++i) {
            result.chars.push_back(cell.chars[i]);
        }
        
        // Ensure at least a space if empty
        if (result.chars.empty()) {
            result.chars.push_back(U' ');
        }
        
        result.width = cell.width > 0 ? cell.width : 1;
        result.attrs = CellAttrs::FromVTerm(cell.attrs);
        result.fg = TermColor::FromVTerm(cell.fg);
        result.bg = TermColor::FromVTerm(cell.bg);
    }
    
    return result;
}

void VTermWrapper::GetCursorPos(int& row, int& col) const {
    row = m_cursorRow;
    col = m_cursorCol;
}

const TermProps& VTermWrapper::GetProps() const noexcept {
    return m_props;
}

void VTermWrapper::Reset() {
    if (m_screen) {
        vterm_screen_reset(m_screen, 1);
    }
}

void VTermWrapper::FlushDamage() {
    if (m_screen) {
        vterm_screen_flush_damage(m_screen);
    }
}

size_t VTermWrapper::OutputRead(char* buffer, size_t maxLen) {
    if (!m_vterm || !buffer || maxLen == 0) {
        return 0;
    }
    return vterm_output_read(m_vterm, buffer, maxLen);
}

void VTermWrapper::SetDamageCallback(DamageCallback callback) {
    m_damageCallback = std::move(callback);
}

void VTermWrapper::SetMoveCursorCallback(MoveCursorCallback callback) {
    m_moveCursorCallback = std::move(callback);
}

void VTermWrapper::SetTermPropCallback(SetTermPropCallback callback) {
    m_termPropCallback = std::move(callback);
}

void VTermWrapper::SetBellCallback(BellCallback callback) {
    m_bellCallback = std::move(callback);
}

void VTermWrapper::SetResizeCallback(ResizeCallback callback) {
    m_resizeCallback = std::move(callback);
}

void VTermWrapper::SetOutputCallback(OutputCallback callback) {
    m_outputCallback = std::move(callback);
}

void VTermWrapper::SetScrollbackPushCallback(ScrollbackPushCallback callback) {
    m_scrollbackPushCallback = std::move(callback);
}

void VTermWrapper::ConvertColorToRgb(VTermColor& color) const {
    if (m_screen) {
        vterm_screen_convert_color_to_rgb(m_screen, &color);
    }
}

// ============================================================================
// Static Callback Handlers
// ============================================================================

int VTermWrapper::OnDamage(VTermRect rect, void* user) {
    auto* self = static_cast<VTermWrapper*>(user);
    if (self && self->m_damageCallback) {
        self->m_damageCallback(rect.start_row, rect.end_row, rect.start_col, rect.end_col);
    }
    return 1;
}

int VTermWrapper::OnMoveRect(VTermRect dest, VTermRect src, void* user) {
    // Convert moverect to damage for simplicity
    auto* self = static_cast<VTermWrapper*>(user);
    if (self && self->m_damageCallback) {
        // Damage both source and destination regions
        self->m_damageCallback(src.start_row, src.end_row, src.start_col, src.end_col);
        self->m_damageCallback(dest.start_row, dest.end_row, dest.start_col, dest.end_col);
    }
    return 1;
}

int VTermWrapper::OnMoveCursor(VTermPos pos, VTermPos oldpos, int visible, void* user) {
    auto* self = static_cast<VTermWrapper*>(user);
    if (self) {
        self->m_cursorRow = pos.row;
        self->m_cursorCol = pos.col;
        
        if (self->m_moveCursorCallback) {
            self->m_moveCursorCallback(pos.row, pos.col, visible != 0);
        }
    }
    return 1;
}

int VTermWrapper::OnSetTermProp(VTermProp prop, VTermValue* val, void* user) {
    auto* self = static_cast<VTermWrapper*>(user);
    if (!self) return 0;

    bool propsChanged = false;

    switch (prop) {
        case VTERM_PROP_CURSORVISIBLE:
            self->m_props.cursorVisible = val->boolean != 0;
            propsChanged = true;
            break;
            
        case VTERM_PROP_CURSORBLINK:
            self->m_props.cursorBlink = val->boolean != 0;
            propsChanged = true;
            break;
            
        case VTERM_PROP_ALTSCREEN:
            self->m_props.altScreen = val->boolean != 0;
            propsChanged = true;
            break;
            
        case VTERM_PROP_TITLE:
            if (val->string.str && val->string.len > 0) {
                // Convert UTF-8 to wide string
                int wideLen = MultiByteToWideChar(CP_UTF8, 0, 
                    val->string.str, static_cast<int>(val->string.len), 
                    nullptr, 0);
                if (wideLen > 0) {
                    self->m_props.title.resize(wideLen);
                    MultiByteToWideChar(CP_UTF8, 0, 
                        val->string.str, static_cast<int>(val->string.len),
                        self->m_props.title.data(), wideLen);
                    propsChanged = true;
                }
            }
            break;
            
        case VTERM_PROP_ICONNAME:
            if (val->string.str && val->string.len > 0) {
                int wideLen = MultiByteToWideChar(CP_UTF8, 0,
                    val->string.str, static_cast<int>(val->string.len),
                    nullptr, 0);
                if (wideLen > 0) {
                    self->m_props.iconName.resize(wideLen);
                    MultiByteToWideChar(CP_UTF8, 0,
                        val->string.str, static_cast<int>(val->string.len),
                        self->m_props.iconName.data(), wideLen);
                    propsChanged = true;
                }
            }
            break;
            
        case VTERM_PROP_CURSORSHAPE:
            switch (val->number) {
                case VTERM_PROP_CURSORSHAPE_BLOCK:
                    self->m_props.cursorShape = CursorShape::Block;
                    break;
                case VTERM_PROP_CURSORSHAPE_UNDERLINE:
                    self->m_props.cursorShape = CursorShape::Underline;
                    break;
                case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
                    self->m_props.cursorShape = CursorShape::Bar;
                    break;
            }
            propsChanged = true;
            break;
            
        case VTERM_PROP_MOUSE:
            self->m_props.mouseMode = val->number;
            propsChanged = true;
            break;
            
        default:
            break;
    }

    if (propsChanged && self->m_termPropCallback) {
        self->m_termPropCallback(self->m_props);
    }

    return 1;
}

int VTermWrapper::OnBell(void* user) {
    auto* self = static_cast<VTermWrapper*>(user);
    if (self && self->m_bellCallback) {
        self->m_bellCallback();
    }
    return 1;
}

int VTermWrapper::OnResize(int rows, int cols, void* user) {
    auto* self = static_cast<VTermWrapper*>(user);
    if (self && self->m_resizeCallback) {
        self->m_resizeCallback(rows, cols);
    }
    return 1;
}

int VTermWrapper::OnScrollbackPush(int cols, const VTermScreenCell* cells, void* user) {
    auto* self = static_cast<VTermWrapper*>(user);
    if (self && self->m_scrollbackPushCallback && cells) {
        std::vector<TermCell> row;
        row.reserve(cols);
        
        for (int i = 0; i < cols; ++i) {
            TermCell cell;
            for (int j = 0; j < VTERM_MAX_CHARS_PER_CELL && cells[i].chars[j] != 0; ++j) {
                cell.chars.push_back(cells[i].chars[j]);
            }
            if (cell.chars.empty()) {
                cell.chars.push_back(U' ');
            }
            cell.width = cells[i].width > 0 ? cells[i].width : 1;
            cell.attrs = CellAttrs::FromVTerm(cells[i].attrs);
            cell.fg = TermColor::FromVTerm(cells[i].fg);
            cell.bg = TermColor::FromVTerm(cells[i].bg);
            row.push_back(std::move(cell));
        }
        
        self->m_scrollbackPushCallback(row);
    }
    return 1;
}

int VTermWrapper::OnScrollbackPop(int cols, VTermScreenCell* cells, void* user) {
    // Scrollback pop is used when scrolling down - we'd need to maintain
    // a scrollback buffer to support this. For now, just clear the cells.
    if (cells) {
        std::memset(cells, 0, cols * sizeof(VTermScreenCell));
    }
    return 0;  // Return 0 to indicate no scrollback available
}

void VTermWrapper::OnOutput(const char* s, size_t len, void* user) {
    auto* self = static_cast<VTermWrapper*>(user);
    if (self && self->m_outputCallback && s && len > 0) {
        self->m_outputCallback(s, len);
    }
}

} // namespace Console3::Emulation
