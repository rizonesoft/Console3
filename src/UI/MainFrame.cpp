// Console3 - MainFrame.cpp
// Main application window implementation

#include "UI/MainFrame.h"
#include "Core/PtySession.h"
#include "Core/TerminalBuffer.h"
#include "Emulation/VTermWrapper.h"

namespace Console3::UI {

// Default window dimensions
constexpr int kDefaultWidth = 800;
constexpr int kDefaultHeight = 600;

// Status bar parts
constexpr int kStatusBarParts = 3;

MainFrame::MainFrame() = default;

MainFrame::~MainFrame() {
    // Stop PTY session before destroying
    if (m_ptySession) {
        m_ptySession->Stop();
    }
}

BOOL MainFrame::PreTranslateMessage(MSG* pMsg) {
    return CFrameWindowImpl<MainFrame>::PreTranslateMessage(pMsg);
}

BOOL MainFrame::OnIdle() {
    UIUpdateToolBar();
    return FALSE;
}

// ============================================================================
// Message Handlers
// ============================================================================

int MainFrame::OnCreate(LPCREATESTRUCT /*lpCreateStruct*/) {
    // Add to message loop
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != nullptr);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);

    // Create UI components
    if (!CreateMenuBar()) {
        return -1;
    }

    if (!CreateStatusBar()) {
        return -1;
    }

    // Set initial window size
    SetWindowPos(nullptr, 0, 0, kDefaultWidth, kDefaultHeight, 
                 SWP_NOMOVE | SWP_NOZORDER);

    // Center window on screen
    CenterWindow();

    // Start with a new terminal session
    if (!StartNewSession()) {
        // Non-fatal: show window anyway, user can open new tab
        m_statusBar.SetText(0, L"Failed to start terminal session");
    }

    // Update status bar
    m_statusBar.SetText(0, L"Ready");

    return 0;
}

void MainFrame::OnDestroy() {
    // Stop PTY session
    if (m_ptySession) {
        m_ptySession->Stop();
    }

    // Remove from message loop
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    if (pLoop) {
        pLoop->RemoveMessageFilter(this);
        pLoop->RemoveIdleHandler(this);
    }

    // Quit the application
    PostQuitMessage(0);
}

void MainFrame::OnSize(UINT nType, CSize size) {
    if (nType == SIZE_MINIMIZED) {
        return;
    }

    // Resize status bar
    if (m_statusBar.IsWindow()) {
        m_statusBar.SendMessage(WM_SIZE);
    }

    // Calculate client area excluding status bar
    CRect clientRect;
    GetClientRect(&clientRect);
    
    CRect statusRect;
    if (m_statusBar.IsWindow()) {
        m_statusBar.GetWindowRect(&statusRect);
        clientRect.bottom -= statusRect.Height();
    }

    // Resize terminal view
    if (m_terminalView) {
        // TODO: Resize terminal view window
    }

    // Resize PTY to match new dimensions
    if (m_ptySession && m_ptySession->IsRunning()) {
        // TODO: Calculate rows/cols from pixel dimensions and font metrics
        // m_ptySession->Resize(cols, rows);
    }
}

void MainFrame::OnSetFocus(CWindow /*wndOld*/) {
    // Forward focus to terminal view
    if (m_terminalView) {
        // TODO: Set focus to terminal view
    }
}

void MainFrame::OnClose() {
    if (m_isClosing) {
        return;
    }
    m_isClosing = true;

    // Check if session is running
    if (m_ptySession && m_ptySession->IsRunning()) {
        int result = MessageBoxW(
            L"A terminal session is still running.\nClose anyway?",
            L"Console3",
            MB_YESNO | MB_ICONQUESTION
        );
        if (result != IDYES) {
            m_isClosing = false;
            return;
        }
    }

    DestroyWindow();
}

// ============================================================================
// Command Handlers
// ============================================================================

void MainFrame::OnFileNewTab(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    // TODO: Implement multi-tab support
    StartNewSession();
}

void MainFrame::OnFileCloseTab(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    // TODO: Implement multi-tab support
    if (m_ptySession) {
        m_ptySession->Stop();
    }
}

void MainFrame::OnFileExit(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    PostMessage(WM_CLOSE);
}

void MainFrame::OnEditCopy(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    // TODO: Copy selection to clipboard
}

void MainFrame::OnEditPaste(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    // TODO: Paste from clipboard
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        if (OpenClipboard()) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(hData));
                if (text && m_ptySession) {
                    // Convert to UTF-8 and send to PTY
                    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
                    if (utf8Len > 0) {
                        std::string utf8(utf8Len, '\0');
                        WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8.data(), utf8Len, nullptr, nullptr);
                        m_ptySession->Write(utf8.c_str(), utf8.length() - 1);  // Exclude null terminator
                    }
                }
                GlobalUnlock(hData);
            }
            CloseClipboard();
        }
    }
}

void MainFrame::OnViewSettings(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    // TODO: Show settings dialog
    MessageBoxW(L"Settings dialog not yet implemented.", L"Console3", MB_ICONINFORMATION);
}

void MainFrame::OnHelpAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/) {
    MessageBoxW(
        L"Console3 Terminal Emulator\n"
        L"Version 0.1.0\n\n"
        L"A modern Windows terminal emulator built with\n"
        L"ConPTY, libvterm, WTL, and Direct2D.\n\n"
        L"Copyright (c) 2026 Rizonesoft",
        L"About Console3",
        MB_ICONINFORMATION
    );
}

// ============================================================================
// Initialization
// ============================================================================

bool MainFrame::CreateMenuBar() {
    // Create menu programmatically
    CMenuHandle mainMenu;
    mainMenu.CreateMenu();

    // File menu
    CMenuHandle fileMenu;
    fileMenu.CreatePopupMenu();
    fileMenu.AppendMenuW(MF_STRING, ID_FILE_NEW_TAB, L"New &Tab\tCtrl+T");
    fileMenu.AppendMenuW(MF_STRING, ID_FILE_CLOSE_TAB, L"&Close Tab\tCtrl+W");
    fileMenu.AppendMenuW(MF_SEPARATOR, 0, nullptr);
    fileMenu.AppendMenuW(MF_STRING, ID_FILE_EXIT, L"E&xit\tAlt+F4");
    mainMenu.AppendMenuW(MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu.m_hMenu), L"&File");

    // Edit menu
    CMenuHandle editMenu;
    editMenu.CreatePopupMenu();
    editMenu.AppendMenuW(MF_STRING, ID_EDIT_COPY, L"&Copy\tCtrl+Shift+C");
    editMenu.AppendMenuW(MF_STRING, ID_EDIT_PASTE, L"&Paste\tCtrl+Shift+V");
    editMenu.AppendMenuW(MF_SEPARATOR, 0, nullptr);
    editMenu.AppendMenuW(MF_STRING, ID_EDIT_FIND, L"&Find...\tCtrl+Shift+F");
    mainMenu.AppendMenuW(MF_POPUP, reinterpret_cast<UINT_PTR>(editMenu.m_hMenu), L"&Edit");

    // View menu
    CMenuHandle viewMenu;
    viewMenu.CreatePopupMenu();
    viewMenu.AppendMenuW(MF_STRING, ID_VIEW_SETTINGS, L"&Settings...");
    mainMenu.AppendMenuW(MF_POPUP, reinterpret_cast<UINT_PTR>(viewMenu.m_hMenu), L"&View");

    // Help menu
    CMenuHandle helpMenu;
    helpMenu.CreatePopupMenu();
    helpMenu.AppendMenuW(MF_STRING, ID_HELP_ABOUT, L"&About Console3...");
    mainMenu.AppendMenuW(MF_POPUP, reinterpret_cast<UINT_PTR>(helpMenu.m_hMenu), L"&Help");

    // Set the menu
    SetMenu(mainMenu);
    m_menu = mainMenu;

    return true;
}

bool MainFrame::CreateStatusBar() {
    // Create status bar
    HWND hWndStatusBar = m_statusBar.Create(
        m_hWnd,
        nullptr,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP
    );

    if (!hWndStatusBar) {
        return false;
    }

    // Set up status bar parts
    int widths[kStatusBarParts] = { 200, 350, -1 };
    m_statusBar.SetParts(kStatusBarParts, widths);

    return true;
}

bool MainFrame::CreateTerminalView() {
    // TODO: Create the terminal view window
    // m_terminalView = std::make_unique<TerminalView>();
    // return m_terminalView->Create(...);
    return true;
}

bool MainFrame::StartNewSession() {
    // Create terminal buffer
    Core::TerminalBufferConfig bufferConfig;
    bufferConfig.rows = 25;
    bufferConfig.cols = 80;
    bufferConfig.scrollbackLines = 10000;
    
    try {
        m_terminalBuffer = std::make_unique<Core::TerminalBuffer>(bufferConfig);
    } catch (const std::exception& e) {
        // Handle error
        return false;
    }

    // Create VTerm wrapper
    try {
        m_vtermWrapper = std::make_unique<Emulation::VTermWrapper>(
            bufferConfig.rows, bufferConfig.cols);
    } catch (const std::exception& e) {
        return false;
    }

    // Create PTY session
    m_ptySession = std::make_unique<Core::PtySession>();

    // Configure and start PTY
    Core::PtyConfig ptyConfig;
    ptyConfig.shell = L"cmd.exe";  // Default to cmd.exe
    ptyConfig.cols = bufferConfig.cols;
    ptyConfig.rows = bufferConfig.rows;

    // Set up output callback to feed VTerm
    m_ptySession->SetOutputCallback([this](const char* data, size_t len) {
        if (m_vtermWrapper) {
            m_vtermWrapper->InputWrite(data, len);
            // TODO: Update terminal view
        }
    });

    // Set up exit callback
    m_ptySession->SetExitCallback([this](DWORD exitCode) {
        // Update status bar
        if (m_statusBar.IsWindow()) {
            std::wstring msg = L"Process exited with code: " + std::to_wstring(exitCode);
            m_statusBar.SetText(0, msg.c_str());
        }
    });

    // Start the session
    if (!m_ptySession->Start(ptyConfig)) {
        std::wstring error = L"Failed to start PTY: " + m_ptySession->GetLastError();
        MessageBoxW(error.c_str(), L"Console3", MB_ICONERROR);
        return false;
    }

    return true;
}

} // namespace Console3::UI
