// Console3 - SettingsDialog.cpp
// Settings dialog implementation

#include "UI/SettingsDialog.h"
#include <atlstr.h>

namespace Console3::UI {

// ============================================================================
// GeneralPage
// ============================================================================

GeneralPage::GeneralPage(Core::Settings& settings)
    : m_settings(settings) {
    SetTitle(L"General");
}

BOOL GeneralPage::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/) {
    // Create controls programmatically
    CRect clientRect;
    GetClientRect(&clientRect);
    
    int y = 20;
    int labelWidth = 120;
    int controlWidth = 200;
    int height = 24;
    int spacing = 35;
    
    // Default Profile label and combo
    CStatic::Create(m_hWnd, CRect(20, y + 3, 20 + labelWidth, y + height),
        L"Default Profile:", WS_CHILD | WS_VISIBLE);
    m_profileCombo.Create(m_hWnd, CRect(20 + labelWidth, y, 20 + labelWidth + controlWidth, y + height * 5),
        nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 0, IDC_DEFAULT_PROFILE);
    
    // Populate profiles
    for (const auto& profile : m_settings.profiles) {
        m_profileCombo.AddString(profile.name.c_str());
    }
    m_profileCombo.SelectString(-1, m_settings.defaultProfile.c_str());
    
    y += spacing;
    
    // Scrollback lines
    CStatic::Create(m_hWnd, CRect(20, y + 3, 20 + labelWidth, y + height),
        L"Scrollback Lines:", WS_CHILD | WS_VISIBLE);
    m_scrollbackEdit.Create(m_hWnd, CRect(20 + labelWidth, y, 20 + labelWidth + 100, y + height),
        nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER, 0, IDC_SCROLLBACK);
    m_scrollbackEdit.SetWindowTextW(std::to_wstring(m_settings.scrollbackLines).c_str());
    
    y += spacing;
    
    // Copy on select checkbox
    m_copyOnSelectCheck.Create(m_hWnd, CRect(20, y, 20 + 200, y + height),
        L"Copy text on selection", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, IDC_COPY_ON_SELECT);
    m_copyOnSelectCheck.SetCheck(m_settings.copyOnSelect ? BST_CHECKED : BST_UNCHECKED);
    
    y += spacing;
    
    // Word wrap checkbox
    m_wordWrapCheck.Create(m_hWnd, CRect(20, y, 20 + 200, y + height),
        L"Enable word wrap", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, IDC_WORD_WRAP);
    m_wordWrapCheck.SetCheck(m_settings.wordWrap ? BST_CHECKED : BST_UNCHECKED);
    
    return TRUE;
}

void GeneralPage::OnProfileChanged(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    SetModified(TRUE);
}

void GeneralPage::OnScrollbackChanged(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    SetModified(TRUE);
}

void GeneralPage::OnCopyOnSelectChanged(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    SetModified(TRUE);
}

int GeneralPage::OnApply() {
    // Save profile
    CString text;
    int sel = m_profileCombo.GetCurSel();
    if (sel >= 0) {
        m_profileCombo.GetLBText(sel, text);
        m_settings.defaultProfile = text.GetString();
    }
    
    // Save scrollback
    m_scrollbackEdit.GetWindowTextW(text);
    m_settings.scrollbackLines = _wtoi(text);
    
    // Save checkboxes
    m_settings.copyOnSelect = (m_copyOnSelectCheck.GetCheck() == BST_CHECKED);
    m_settings.wordWrap = (m_wordWrapCheck.GetCheck() == BST_CHECKED);
    
    return PSNRET_NOERROR;
}

// ============================================================================
// AppearancePage
// ============================================================================

AppearancePage::AppearancePage(Core::Settings& settings)
    : m_settings(settings) {
    SetTitle(L"Appearance");
}

BOOL AppearancePage::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/) {
    int y = 20;
    int labelWidth = 100;
    int controlWidth = 180;
    int height = 24;
    int spacing = 35;
    
    // Font family
    CStatic::Create(m_hWnd, CRect(20, y + 3, 20 + labelWidth, y + height),
        L"Font Family:", WS_CHILD | WS_VISIBLE);
    m_fontFamilyCombo.Create(m_hWnd, CRect(20 + labelWidth, y, 20 + labelWidth + controlWidth, y + height * 8),
        nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN, 0, IDC_FONT_FAMILY);
    
    // Add common monospace fonts
    const wchar_t* fonts[] = { L"Consolas", L"Cascadia Code", L"Cascadia Mono", 
        L"Fira Code", L"JetBrains Mono", L"Source Code Pro", L"Courier New" };
    for (const auto* font : fonts) {
        m_fontFamilyCombo.AddString(font);
    }
    m_fontFamilyCombo.SetWindowTextW(m_settings.font.family.c_str());
    
    y += spacing;
    
    // Font size
    CStatic::Create(m_hWnd, CRect(20, y + 3, 20 + labelWidth, y + height),
        L"Font Size:", WS_CHILD | WS_VISIBLE);
    m_fontSizeEdit.Create(m_hWnd, CRect(20 + labelWidth, y, 20 + labelWidth + 60, y + height),
        nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, 0, IDC_FONT_SIZE);
    m_fontSizeEdit.SetWindowTextW(std::to_wstring(static_cast<int>(m_settings.font.size)).c_str());
    
    y += spacing;
    
    // Font bold
    m_fontBoldCheck.Create(m_hWnd, CRect(20, y, 20 + 150, y + height),
        L"Bold font", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, IDC_FONT_BOLD);
    m_fontBoldCheck.SetCheck(m_settings.font.bold ? BST_CHECKED : BST_UNCHECKED);
    
    y += spacing;
    
    // Color scheme
    CStatic::Create(m_hWnd, CRect(20, y + 3, 20 + labelWidth, y + height),
        L"Color Scheme:", WS_CHILD | WS_VISIBLE);
    m_colorSchemeCombo.Create(m_hWnd, CRect(20 + labelWidth, y, 20 + labelWidth + controlWidth, y + height * 6),
        nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 0, IDC_COLOR_SCHEME);
    m_colorSchemeCombo.AddString(L"Default");
    m_colorSchemeCombo.AddString(L"One Dark");
    m_colorSchemeCombo.AddString(L"Solarized Dark");
    m_colorSchemeCombo.AddString(L"Solarized Light");
    m_colorSchemeCombo.SelectString(-1, m_settings.colorScheme.name.c_str());
    
    y += spacing;
    
    // Opacity
    CStatic::Create(m_hWnd, CRect(20, y + 3, 20 + labelWidth, y + height),
        L"Opacity:", WS_CHILD | WS_VISIBLE);
    m_opacitySlider.Create(m_hWnd, CRect(20 + labelWidth, y, 20 + labelWidth + controlWidth, y + height),
        nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ, 0, IDC_OPACITY_SLIDER);
    m_opacitySlider.SetRange(20, 100);
    m_opacitySlider.SetPos(static_cast<int>(m_settings.window.opacity * 100));
    
    y += spacing;
    
    // Use acrylic
    m_useAcrylicCheck.Create(m_hWnd, CRect(20, y, 20 + 200, y + height),
        L"Use acrylic background", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, IDC_USE_ACRYLIC);
    m_useAcrylicCheck.SetCheck(m_settings.window.useAcrylic ? BST_CHECKED : BST_UNCHECKED);
    
    return TRUE;
}

void AppearancePage::OnFontChanged(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    SetModified(TRUE);
}

void AppearancePage::OnColorSchemeChanged(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    SetModified(TRUE);
}

int AppearancePage::OnApply() {
    CString text;
    
    // Font family
    m_fontFamilyCombo.GetWindowTextW(text);
    m_settings.font.family = text.GetString();
    
    // Font size
    m_fontSizeEdit.GetWindowTextW(text);
    m_settings.font.size = static_cast<float>(_wtof(text));
    
    // Font bold
    m_settings.font.bold = (m_fontBoldCheck.GetCheck() == BST_CHECKED);
    
    // Color scheme
    int sel = m_colorSchemeCombo.GetCurSel();
    if (sel >= 0) {
        m_colorSchemeCombo.GetLBText(sel, text);
        m_settings.colorScheme.name = text.GetString();
    }
    
    // Opacity
    m_settings.window.opacity = m_opacitySlider.GetPos() / 100.0f;
    
    // Acrylic
    m_settings.window.useAcrylic = (m_useAcrylicCheck.GetCheck() == BST_CHECKED);
    
    return PSNRET_NOERROR;
}

// ============================================================================
// CursorPage
// ============================================================================

CursorPage::CursorPage(Core::Settings& settings)
    : m_settings(settings) {
    SetTitle(L"Cursor");
}

BOOL CursorPage::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/) {
    int y = 20;
    int labelWidth = 100;
    int controlWidth = 150;
    int height = 24;
    int spacing = 35;
    
    // Cursor style
    CStatic::Create(m_hWnd, CRect(20, y + 3, 20 + labelWidth, y + height),
        L"Cursor Style:", WS_CHILD | WS_VISIBLE);
    m_cursorStyleCombo.Create(m_hWnd, CRect(20 + labelWidth, y, 20 + labelWidth + controlWidth, y + height * 4),
        nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 0, IDC_CURSOR_STYLE);
    m_cursorStyleCombo.AddString(L"Block");
    m_cursorStyleCombo.AddString(L"Underline");
    m_cursorStyleCombo.AddString(L"Bar");
    
    // Select current style
    if (m_settings.cursor.style == L"underline") {
        m_cursorStyleCombo.SetCurSel(1);
    } else if (m_settings.cursor.style == L"bar") {
        m_cursorStyleCombo.SetCurSel(2);
    } else {
        m_cursorStyleCombo.SetCurSel(0);
    }
    
    y += spacing;
    
    // Cursor blink
    m_cursorBlinkCheck.Create(m_hWnd, CRect(20, y, 20 + 150, y + height),
        L"Enable cursor blink", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, IDC_CURSOR_BLINK);
    m_cursorBlinkCheck.SetCheck(m_settings.cursor.blink ? BST_CHECKED : BST_UNCHECKED);
    
    y += spacing;
    
    // Blink rate
    CStatic::Create(m_hWnd, CRect(20, y + 3, 20 + labelWidth, y + height),
        L"Blink Rate (ms):", WS_CHILD | WS_VISIBLE);
    m_blinkRateEdit.Create(m_hWnd, CRect(20 + labelWidth, y, 20 + labelWidth + 80, y + height),
        nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER, 0, IDC_BLINK_RATE);
    m_blinkRateEdit.SetWindowTextW(std::to_wstring(m_settings.cursor.blinkRate).c_str());
    
    return TRUE;
}

void CursorPage::OnCursorStyleChanged(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    SetModified(TRUE);
}

void CursorPage::OnCursorBlinkChanged(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    SetModified(TRUE);
}

int CursorPage::OnApply() {
    // Cursor style
    int sel = m_cursorStyleCombo.GetCurSel();
    switch (sel) {
        case 1: m_settings.cursor.style = L"underline"; break;
        case 2: m_settings.cursor.style = L"bar"; break;
        default: m_settings.cursor.style = L"block"; break;
    }
    
    // Blink
    m_settings.cursor.blink = (m_cursorBlinkCheck.GetCheck() == BST_CHECKED);
    
    // Blink rate
    CString text;
    m_blinkRateEdit.GetWindowTextW(text);
    m_settings.cursor.blinkRate = _wtoi(text);
    
    return PSNRET_NOERROR;
}

// ============================================================================
// TabsPage
// ============================================================================

TabsPage::TabsPage(Core::Settings& settings)
    : m_settings(settings) {
    SetTitle(L"Tabs");
}

BOOL TabsPage::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/) {
    int y = 20;
    int labelWidth = 140;
    int controlWidth = 160;
    int height = 24;
    int spacing = 35;
    
    // New tab position
    CStatic::Create(m_hWnd, CRect(20, y + 3, 20 + labelWidth, y + height),
        L"New Tab Position:", WS_CHILD | WS_VISIBLE);
    m_newTabPosCombo.Create(m_hWnd, CRect(20 + labelWidth, y, 20 + labelWidth + controlWidth, y + height * 3),
        nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 0, IDC_NEW_TAB_POS);
    m_newTabPosCombo.AddString(L"After Current Tab");
    m_newTabPosCombo.AddString(L"At End");
    m_newTabPosCombo.SetCurSel(m_settings.tabs.newTabPosition == L"atEnd" ? 1 : 0);
    
    y += spacing;
    
    // Close last tab action
    CStatic::Create(m_hWnd, CRect(20, y + 3, 20 + labelWidth, y + height),
        L"When Last Tab Closes:", WS_CHILD | WS_VISIBLE);
    m_closeLastActionCombo.Create(m_hWnd, CRect(20 + labelWidth, y, 20 + labelWidth + controlWidth, y + height * 3),
        nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 0, IDC_CLOSE_LAST_ACTION);
    m_closeLastActionCombo.AddString(L"Close Window");
    m_closeLastActionCombo.AddString(L"Open New Tab");
    m_closeLastActionCombo.SetCurSel(m_settings.tabs.closeLastTabAction == L"newTab" ? 1 : 0);
    
    y += spacing;
    
    // Show close button
    m_showCloseBtn.Create(m_hWnd, CRect(20, y, 20 + 200, y + height),
        L"Show close button on tabs", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, IDC_SHOW_CLOSE_BTN);
    m_showCloseBtn.SetCheck(m_settings.tabs.showCloseButton ? BST_CHECKED : BST_UNCHECKED);
    
    y += spacing;
    
    // Confirm close
    m_confirmClose.Create(m_hWnd, CRect(20, y, 20 + 200, y + height),
        L"Confirm before closing tab", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, IDC_CONFIRM_CLOSE);
    m_confirmClose.SetCheck(m_settings.tabs.confirmTabClose ? BST_CHECKED : BST_UNCHECKED);
    
    y += spacing;
    
    // Restore tabs
    m_restoreTabs.Create(m_hWnd, CRect(20, y, 20 + 200, y + height),
        L"Restore tabs on startup", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, IDC_RESTORE_TABS);
    m_restoreTabs.SetCheck(m_settings.tabs.restoreTabsOnStartup ? BST_CHECKED : BST_UNCHECKED);
    
    return TRUE;
}

int TabsPage::OnApply() {
    // New tab position
    m_settings.tabs.newTabPosition = (m_newTabPosCombo.GetCurSel() == 1) ? L"atEnd" : L"afterCurrent";
    
    // Close last action
    m_settings.tabs.closeLastTabAction = (m_closeLastActionCombo.GetCurSel() == 1) ? L"newTab" : L"closeWindow";
    
    // Checkboxes
    m_settings.tabs.showCloseButton = (m_showCloseBtn.GetCheck() == BST_CHECKED);
    m_settings.tabs.confirmTabClose = (m_confirmClose.GetCheck() == BST_CHECKED);
    m_settings.tabs.restoreTabsOnStartup = (m_restoreTabs.GetCheck() == BST_CHECKED);
    
    return PSNRET_NOERROR;
}

// ============================================================================
// SettingsDialog
// ============================================================================

SettingsDialog::SettingsDialog(Core::Settings& settings)
    : m_settings(settings)
    , m_originalSettings(settings) {
    
    SetTitle(L"Console3 Settings");
    
    // Create pages
    m_generalPage = std::make_unique<GeneralPage>(m_settings);
    m_appearancePage = std::make_unique<AppearancePage>(m_settings);
    m_cursorPage = std::make_unique<CursorPage>(m_settings);
    m_tabsPage = std::make_unique<TabsPage>(m_settings);
    
    // Add pages
    AddPage(*m_generalPage);
    AddPage(*m_appearancePage);
    AddPage(*m_cursorPage);
    AddPage(*m_tabsPage);
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::SetChangedCallback(SettingsChangedCallback callback) {
    m_changedCallback = std::move(callback);
}

INT_PTR SettingsDialog::DoModal(HWND hWndParent) {
    INT_PTR result = CPropertySheetImpl<SettingsDialog>::DoModal(hWndParent);
    
    if (result == IDCANCEL) {
        // Restore original settings
        m_settings = m_originalSettings;
    }
    
    return result;
}

} // namespace Console3::UI
