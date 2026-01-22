#pragma once
// Console3 - SettingsDialog.h
// Settings dialog using WTL property sheet with multiple pages

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
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlcrack.h>

#include "Core/Settings.h"

#include <memory>
#include <functional>

namespace Console3::UI {

/// Callback for settings changes (for live preview)
using SettingsChangedCallback = std::function<void(const Core::Settings&)>;

// ============================================================================
// Property Pages
// ============================================================================

/// General settings page
class GeneralPage : public CPropertyPageImpl<GeneralPage> {
public:
    enum { IDD = 0 };  // We'll create controls programmatically

    GeneralPage(Core::Settings& settings);

    BEGIN_MSG_MAP(GeneralPage)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_DEFAULT_PROFILE, CBN_SELCHANGE, OnProfileChanged)
        COMMAND_HANDLER_EX(IDC_SCROLLBACK, EN_CHANGE, OnScrollbackChanged)
        COMMAND_HANDLER_EX(IDC_COPY_ON_SELECT, BN_CLICKED, OnCopyOnSelectChanged)
        CHAIN_MSG_MAP(CPropertyPageImpl<GeneralPage>)
    END_MSG_MAP()

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnProfileChanged(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnScrollbackChanged(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnCopyOnSelectChanged(UINT uNotifyCode, int nID, CWindow wndCtl);
    int OnApply();

    // Control IDs
    enum {
        IDC_DEFAULT_PROFILE = 1001,
        IDC_SCROLLBACK,
        IDC_COPY_ON_SELECT,
        IDC_WORD_WRAP
    };

private:
    Core::Settings& m_settings;
    CComboBox m_profileCombo;
    CEdit m_scrollbackEdit;
    CButton m_copyOnSelectCheck;
    CButton m_wordWrapCheck;
};

/// Appearance settings page
class AppearancePage : public CPropertyPageImpl<AppearancePage> {
public:
    enum { IDD = 0 };

    AppearancePage(Core::Settings& settings);

    BEGIN_MSG_MAP(AppearancePage)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_FONT_FAMILY, CBN_SELCHANGE, OnFontChanged)
        COMMAND_HANDLER_EX(IDC_FONT_SIZE, EN_CHANGE, OnFontChanged)
        COMMAND_HANDLER_EX(IDC_COLOR_SCHEME, CBN_SELCHANGE, OnColorSchemeChanged)
        CHAIN_MSG_MAP(CPropertyPageImpl<AppearancePage>)
    END_MSG_MAP()

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnFontChanged(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnColorSchemeChanged(UINT uNotifyCode, int nID, CWindow wndCtl);
    int OnApply();

    enum {
        IDC_FONT_FAMILY = 2001,
        IDC_FONT_SIZE,
        IDC_FONT_BOLD,
        IDC_COLOR_SCHEME,
        IDC_OPACITY_SLIDER,
        IDC_USE_ACRYLIC
    };

private:
    Core::Settings& m_settings;
    CComboBox m_fontFamilyCombo;
    CEdit m_fontSizeEdit;
    CButton m_fontBoldCheck;
    CComboBox m_colorSchemeCombo;
    CTrackBarCtrl m_opacitySlider;
    CButton m_useAcrylicCheck;
};

/// Cursor settings page
class CursorPage : public CPropertyPageImpl<CursorPage> {
public:
    enum { IDD = 0 };

    CursorPage(Core::Settings& settings);

    BEGIN_MSG_MAP(CursorPage)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_CURSOR_STYLE, CBN_SELCHANGE, OnCursorStyleChanged)
        COMMAND_HANDLER_EX(IDC_CURSOR_BLINK, BN_CLICKED, OnCursorBlinkChanged)
        CHAIN_MSG_MAP(CPropertyPageImpl<CursorPage>)
    END_MSG_MAP()

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnCursorStyleChanged(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnCursorBlinkChanged(UINT uNotifyCode, int nID, CWindow wndCtl);
    int OnApply();

    enum {
        IDC_CURSOR_STYLE = 3001,
        IDC_CURSOR_BLINK,
        IDC_BLINK_RATE
    };

private:
    Core::Settings& m_settings;
    CComboBox m_cursorStyleCombo;
    CButton m_cursorBlinkCheck;
    CEdit m_blinkRateEdit;
};

/// Tab settings page
class TabsPage : public CPropertyPageImpl<TabsPage> {
public:
    enum { IDD = 0 };

    TabsPage(Core::Settings& settings);

    BEGIN_MSG_MAP(TabsPage)
        MSG_WM_INITDIALOG(OnInitDialog)
        CHAIN_MSG_MAP(CPropertyPageImpl<TabsPage>)
    END_MSG_MAP()

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    int OnApply();

    enum {
        IDC_NEW_TAB_POS = 4001,
        IDC_CLOSE_LAST_ACTION,
        IDC_SHOW_CLOSE_BTN,
        IDC_CONFIRM_CLOSE,
        IDC_RESTORE_TABS
    };

private:
    Core::Settings& m_settings;
    CComboBox m_newTabPosCombo;
    CComboBox m_closeLastActionCombo;
    CButton m_showCloseBtn;
    CButton m_confirmClose;
    CButton m_restoreTabs;
};

// ============================================================================
// Main Settings Dialog
// ============================================================================

/// Settings dialog with property sheet
class SettingsDialog : public CPropertySheetImpl<SettingsDialog> {
public:
    SettingsDialog(Core::Settings& settings);
    ~SettingsDialog();

    /// Set callback for live preview
    void SetChangedCallback(SettingsChangedCallback callback);

    /// Show the dialog
    /// @return IDOK if user clicked OK, IDCANCEL otherwise
    INT_PTR DoModal(HWND hWndParent = nullptr);

    BEGIN_MSG_MAP(SettingsDialog)
        CHAIN_MSG_MAP(CPropertySheetImpl<SettingsDialog>)
    END_MSG_MAP()

private:
    Core::Settings& m_settings;
    Core::Settings m_originalSettings;  // For cancel/revert
    SettingsChangedCallback m_changedCallback;

    // Pages
    std::unique_ptr<GeneralPage> m_generalPage;
    std::unique_ptr<AppearancePage> m_appearancePage;
    std::unique_ptr<CursorPage> m_cursorPage;
    std::unique_ptr<TabsPage> m_tabsPage;
};

} // namespace Console3::UI
