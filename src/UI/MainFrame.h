#pragma once
// Console3 - MainFrame.h
// Main application window using WTL

// Target Windows 10 RS5 (1809) or later
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
#include <atlcrack.h>

// Direct2D
#include <d2d1_1.h>
#include <dwrite_1.h>

#include <memory>
#include <string>

// Forward declarations
namespace Console3::Core {
    class PtySession;
    class TerminalBuffer;
}

namespace Console3::Emulation {
    class VTermWrapper;
}

namespace Console3::UI {

// Forward declarations
class TerminalView;

/// Main application window
class MainFrame : 
    public CFrameWindowImpl<MainFrame>,
    public CUpdateUI<MainFrame>,
    public CMessageFilter,
    public CIdleHandler
{
public:
    DECLARE_FRAME_WND_CLASS(L"Console3MainFrame", 0)

    MainFrame();
    ~MainFrame();

    // Factory setters (must be called before CreateEx)
    void SetD2DFactory(ID2D1Factory1* factory) { m_d2dFactory = factory; }
    void SetDWriteFactory(IDWriteFactory1* factory) { m_dwriteFactory = factory; }

    // CMessageFilter
    BOOL PreTranslateMessage(MSG* pMsg) override;

    // CIdleHandler
    BOOL OnIdle() override;

    // Message map
    BEGIN_MSG_MAP(MainFrame)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_SIZE(OnSize)
        MSG_WM_SETFOCUS(OnSetFocus)
        MSG_WM_CLOSE(OnClose)
        COMMAND_ID_HANDLER_EX(ID_FILE_NEW_TAB, OnFileNewTab)
        COMMAND_ID_HANDLER_EX(ID_FILE_CLOSE_TAB, OnFileCloseTab)
        COMMAND_ID_HANDLER_EX(ID_FILE_EXIT, OnFileExit)
        COMMAND_ID_HANDLER_EX(ID_EDIT_COPY, OnEditCopy)
        COMMAND_ID_HANDLER_EX(ID_EDIT_PASTE, OnEditPaste)
        COMMAND_ID_HANDLER_EX(ID_VIEW_SETTINGS, OnViewSettings)
        COMMAND_ID_HANDLER_EX(ID_HELP_ABOUT, OnHelpAbout)
        CHAIN_MSG_MAP(CUpdateUI<MainFrame>)
        CHAIN_MSG_MAP(CFrameWindowImpl<MainFrame>)
    END_MSG_MAP()

    // Update UI map
    BEGIN_UPDATE_UI_MAP(MainFrame)
        UPDATE_ELEMENT(ID_FILE_CLOSE_TAB, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_EDIT_COPY, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_EDIT_PASTE, UPDUI_MENUPOPUP)
    END_UPDATE_UI_MAP()

    // Command IDs
    enum {
        ID_FILE_NEW_TAB = 100,
        ID_FILE_CLOSE_TAB,
        ID_FILE_EXIT,
        ID_EDIT_COPY,
        ID_EDIT_PASTE,
        ID_EDIT_FIND,
        ID_VIEW_SETTINGS,
        ID_HELP_ABOUT,
    };

private:
    // Message handlers
    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnSize(UINT nType, CSize size);
    void OnSetFocus(CWindow wndOld);
    void OnClose();

    // Command handlers
    void OnFileNewTab(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFileCloseTab(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFileExit(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnEditCopy(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnEditPaste(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewSettings(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnHelpAbout(UINT uNotifyCode, int nID, CWindow wndCtl);

    // Initialization
    bool CreateMenuBar();
    bool CreateStatusBar();
    bool CreateTerminalView();

    // Start a new terminal session
    bool StartNewSession();

private:
    // Direct2D/DirectWrite factories (not owned)
    ID2D1Factory1* m_d2dFactory = nullptr;
    IDWriteFactory1* m_dwriteFactory = nullptr;

    // UI components
    CMenuHandle m_menu;
    CStatusBarCtrl m_statusBar;
    std::unique_ptr<TerminalView> m_terminalView;

    // Core components
    std::unique_ptr<Core::PtySession> m_ptySession;
    std::unique_ptr<Core::TerminalBuffer> m_terminalBuffer;
    std::unique_ptr<Emulation::VTermWrapper> m_vtermWrapper;

    // Window state
    bool m_isClosing = false;
};

} // namespace Console3::UI
