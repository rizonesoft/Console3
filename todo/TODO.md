# Console3 - Modern Terminal Emulator TODO

> A lightweight, native Windows terminal emulator built on ConPTY API with Direct2D rendering.

---

## Phase 1: Project Setup & Infrastructure

### 1.1 Visual Studio 2026 Environment
- [x] Install Visual Studio 2026 Build Tools (MSVC v144 toolchain)
- [x] Install Windows 11 SDK (latest version with ConPTY support)
- [x] Install C++20 language components
- [x] CMake available (bundled with VS2026 - v4.1.1)

### 1.2 Repository Structure
- [x] Create folder structure:
  ```
  /Console3
  ├── /src
  │   ├── /Core
  │   ├── /UI
  │   └── /Emulation
  ├── /vendor
  ├── /docs
  ├── /tests
  └── /assets
  ```
- [x] Create `CMakeLists.txt` root configuration
- [x] Create `src/CMakeLists.txt` for source targets
- [x] Create `tests/CMakeLists.txt` for test targets

### 1.3 Dependency Acquisition
- [x] Download and integrate **WTL 10** (Windows Template Library) to `/vendor/wtl`
- [x] Download and integrate **WIL** (Windows Implementation Libraries) to `/vendor/wil`
- [x] Download and build **libvterm** to `/vendor/libvterm`
- [x] ~~Configure vcpkg manifest~~ Using FetchContent instead (simpler, no vcpkg dependency)
- [x] Add nlohmann/json for settings file parsing (via FetchContent)

---

## Phase 2: GitHub Repository Setup

### 2.1 Repository Initialization
- [x] Create GitHub repository `rizonesoft/Console3`
- [x] Initialize with `.gitignore` (Visual Studio, CMake, build artifacts)
- [ ] Create initial commit with folder structure

### 2.2 Repository Documentation
- [x] Create `README.md` with project overview, features, and build instructions
- [x] Create `LICENSE` file (choose license - recommend MIT or GPL v2 for Console2 compatibility)
- [x] Create `CONTRIBUTING.md` with contribution guidelines
- [x] Create `CODE_OF_CONDUCT.md`
- [x] Create `SECURITY.md` with vulnerability reporting instructions
- [x] Create `CHANGELOG.md` with Keep a Changelog format

### 2.3 Issue & PR Templates
- [x] Create `.github/ISSUE_TEMPLATE/bug_report.md`
- [x] Create `.github/ISSUE_TEMPLATE/feature_request.md`
- [x] Create `.github/PULL_REQUEST_TEMPLATE.md`

### 2.4 GitHub Actions CI/CD
- [x] Create `.github/workflows/build.yml` for CI builds (x64, ARM64)
- [x] Configure MSBuild with VS2026 toolchain in workflow
- [x] Add build matrix for Debug/Release configurations
- [x] Create `.github/workflows/release.yml` for tagged releases
- [x] Add artifact upload for build outputs

---

## Phase 3: Core Backend (ConPTY)

### 3.1 PtySession Implementation
- [ ] Create `src/Core/PtySession.h` - ConPTY wrapper class declaration
- [ ] Create `src/Core/PtySession.cpp` - Implementation:
  - [ ] Implement pipe creation (input/output)
  - [ ] Implement `CreatePseudoConsole()` wrapper
  - [ ] Implement process startup with `PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE`
  - [ ] Implement `ResizePseudoConsole()` for window resize
  - [ ] Implement graceful shutdown and cleanup
- [ ] Add WIL RAII wrappers for handles (`wil::unique_hfile`, `wil::unique_handle`)
- [ ] Implement thread-safe ring buffer for output data

### 3.2 IO Thread Management
- [ ] Create `src/Core/IoThread.h` - Background read thread
- [ ] Create `src/Core/IoThread.cpp`:
  - [ ] Implement 4KB chunk reading from ConPTY output pipe
  - [ ] Implement thread-safe queue push to ring buffer
  - [ ] Implement graceful thread termination
- [ ] Create `src/Core/RingBuffer.h` - Lock-free ring buffer implementation

### 3.3 Process Management
- [ ] Implement shell detection (cmd.exe, powershell.exe, pwsh.exe, wsl.exe)
- [ ] Implement environment variable passthrough
- [ ] Implement working directory setting
- [ ] Implement custom shell command support

---

## Phase 4: Terminal Emulation (libvterm)

### 4.1 VTerm Wrapper
- [ ] Create `src/Emulation/VTermWrapper.h` - libvterm C++ wrapper
- [ ] Create `src/Emulation/VTermWrapper.cpp`:
  - [ ] Initialize `vterm_new()` with screen dimensions
  - [ ] Implement `vterm_input_write()` for byte feeding
  - [ ] Set up screen callbacks structure

### 4.2 Screen Callbacks
- [ ] Implement `putglyph` callback - character placement
- [ ] Implement `movecursor` callback - cursor position tracking
- [ ] Implement `settermprop` callback - terminal properties (title, etc.)
- [ ] Implement `scrollrect` callback - scroll region handling
- [ ] Implement `moverect` callback - content moving
- [ ] Implement `erase` callback - screen clearing
- [ ] Implement `bell` callback - system bell handling

### 4.3 Terminal Buffer
- [ ] Create `src/Core/TerminalBuffer.h`:
  ```cpp
  struct Cell {
      uint32_t charCode;  // UTF-32
      uint32_t fgColor, bgColor;
      uint8_t attributes; // Bold, Italic, Underline
  };
  ```
- [ ] Create `src/Core/TerminalBuffer.cpp`:
  - [ ] Implement `std::vector<Cell>` grid storage
  - [ ] Implement dirty line tracking for efficient redraws
  - [ ] Implement scrollback buffer (configurable history)
- [ ] Implement cell attribute parsing (SGR sequences)

---

## Phase 5: UI Foundation (WTL + Direct2D)

### 5.1 Application Framework
- [ ] Create `src/main.cpp` - Application entry point
- [ ] Create `src/UI/MainFrame.h` - Main window class (CFrameWindowImpl)
- [ ] Create `src/UI/MainFrame.cpp`:
  - [ ] Implement window creation and message loop
  - [ ] Implement menu bar (File, Edit, View, Help)
  - [ ] Implement status bar
- [ ] Create application manifest (`Console3.exe.manifest`):
  - [ ] Set `<dpiAware>true</dpiAware>` for High-DPI support
  - [ ] Set `<dpiAwareness>PerMonitorV2</dpiAwareness>`
  - [ ] Require Windows 10 1809+ compatibility

### 5.2 Direct2D Initialization
- [ ] Create `src/UI/D2DRenderer.h` - Direct2D wrapper
- [ ] Create `src/UI/D2DRenderer.cpp`:
  - [ ] Initialize `ID2D1Factory`
  - [ ] Create `ID2D1HwndRenderTarget`
  - [ ] Implement render target recreation on resize
- [ ] Create `src/UI/DirectWriteFont.h` - Font management
- [ ] Create `src/UI/DirectWriteFont.cpp`:
  - [ ] Initialize `IDWriteFactory`
  - [ ] Implement font loading with fallback chain
  - [ ] Implement `IDWriteTextFormat` creation

### 5.3 Terminal View
- [ ] Create `src/UI/TerminalView.h` - Terminal rendering window
- [ ] Create `src/UI/TerminalView.cpp`:
  - [ ] Implement `WM_PAINT` handler with `BeginDraw()`/`EndDraw()`
  - [ ] Implement line-based text rendering with `IDWriteTextLayout`
  - [ ] Implement cursor rendering (block, underline, bar styles)
  - [ ] Implement selection highlighting
  - [ ] Implement only-dirty-lines rendering optimization

---

## Phase 6: Input Handling

### 6.1 Keyboard Input
- [ ] Implement `WM_KEYDOWN` / `WM_KEYUP` handling
- [ ] Create ANSI escape sequence mapping:
  - [ ] Arrow keys → `\x1b[A`, `\x1b[B`, `\x1b[C`, `\x1b[D`
  - [ ] Function keys → `\x1b[11~` through `\x1b[24~`
  - [ ] Home/End/Insert/Delete/PageUp/PageDown
  - [ ] Ctrl+Key combinations
  - [ ] Alt+Key combinations (Meta key)
- [ ] Implement `WM_CHAR` for printable character input
- [ ] Implement IME (Input Method Editor) support for CJK input

### 6.2 Mouse Input
- [ ] Implement mouse reporting modes (X10, Normal, SGR)
- [ ] Handle `WM_LBUTTONDOWN`, `WM_RBUTTONDOWN`, `WM_MBUTTONDOWN`
- [ ] Handle `WM_MOUSEMOVE` for selection and mouse tracking
- [ ] Handle `WM_MOUSEWHEEL` for scrolling

### 6.3 Clipboard
- [ ] Implement copy to clipboard (Ctrl+Shift+C or selection + right-click)
- [ ] Implement paste from clipboard (Ctrl+Shift+V or right-click)
- [ ] Implement bracketed paste mode support

---

## Phase 7: Window Management

### 7.1 Resize Handling
- [ ] Implement `WM_SIZE` handler
- [ ] Call `ResizePseudoConsole()` on resize
- [ ] Recalculate rows/columns based on font metrics
- [ ] Recreate Direct2D render target on resize

### 7.2 Tab System
- [ ] Create `src/UI/TabControl.h` - Tab management
- [ ] Create `src/UI/TabControl.cpp`:
  - [ ] Implement tab bar rendering (custom drawn or CTabCtrl)
  - [ ] Implement tab creation/closing
  - [ ] Implement tab switching (Ctrl+Tab, Ctrl+1-9)
  - [ ] Implement tab reordering (drag & drop)
  - [ ] Implement tab context menu (Close, Duplicate, etc.)

### 7.3 Session Management
- [ ] Create `src/Core/Session.h` - Tab session model
- [ ] Create `src/Core/Session.cpp`:
  - [ ] Associate PtySession + TerminalBuffer + TerminalView
  - [ ] Implement session state management
  - [ ] Implement session serialization for restore

---

## Phase 8: Configuration System

### 8.1 Settings File
- [ ] Create `src/Core/Settings.h` - Settings model
- [ ] Create `src/Core/Settings.cpp`:
  - [ ] Define settings schema (JSON format)
  - [ ] Implement settings loading from `%APPDATA%\Console3\settings.json`
  - [ ] Implement settings saving
  - [ ] Implement default settings generation

### 8.2 Configurable Options
- [ ] Font family and size
- [ ] Color scheme (16-color palette + ANSI 256 + TrueColor)
- [ ] Background opacity/transparency
- [ ] Cursor style and blink rate
- [ ] Scrollback buffer size
- [ ] Default shell and arguments
- [ ] Keyboard shortcuts
- [ ] Tab behavior settings

### 8.3 Settings UI
- [ ] Create settings dialog (WTL property sheet)
- [ ] Implement live preview for visual settings
- [ ] Implement import/export settings

---

## Phase 9: Advanced Features

### 9.1 Background Transparency
- [ ] Implement Windows Acrylic blur effect (DWM)
- [ ] Implement configurable opacity
- [ ] Handle composition changes (`WM_DWMCOMPOSITIONCHANGED`)

### 9.2 Font Rendering
- [ ] Implement ligature support (Fira Code, JetBrains Mono)
- [ ] Implement font fallback for Emoji and CJK characters
- [ ] Implement bold/italic/underline rendering
- [ ] Implement strikethrough and double-underline

### 9.3 URL Detection
- [ ] Implement URL regex detection in buffer
- [ ] Implement Ctrl+Click to open URLs
- [ ] Implement URL highlighting on hover

### 9.4 Search
- [ ] Implement Ctrl+Shift+F search dialog
- [ ] Implement regex search
- [ ] Implement search highlighting
- [ ] Implement Find Next/Previous navigation

---

## Phase 10: Administrator Support

### 10.1 Elevated Tabs
- [ ] Research approach: helper process vs. full elevation
- [ ] Implement helper process for admin tabs (Named Pipe IPC)
- [ ] Implement visual indicator for elevated tabs
- [ ] Implement UAC prompt handling

### 10.2 Context Menu Integration
- [ ] Create shell extension for "Open Console3 Here"
- [ ] Create shell extension for "Open Console3 Here as Administrator"

---

## Phase 11: Testing

### 11.1 Unit Tests
- [ ] Set up Google Test framework
- [ ] Write tests for `RingBuffer`
- [ ] Write tests for `VTermWrapper` callbacks
- [ ] Write tests for `TerminalBuffer` cell operations
- [ ] Write tests for ANSI escape sequence generation
- [ ] Write tests for Settings serialization

### 11.2 Integration Tests
- [ ] Test ConPTY session lifecycle
- [ ] Test shell spawning (cmd, powershell, wsl)
- [ ] Test resize behavior
- [ ] Test clipboard operations

### 11.3 Manual Testing Checklist
- [ ] Verify Unicode rendering (Emoji, CJK, RTL)
- [ ] Verify 256-color and TrueColor support
- [ ] Verify ncurses applications (vim, htop, tmux)
- [ ] Verify SSH sessions
- [ ] Verify WSL integration
- [ ] Test on Windows 10 and Windows 11
- [ ] Test on 4K displays (High-DPI)
- [ ] Test with multiple monitors (different DPI)

---

## Phase 12: Build & Release

### 12.1 CMake Configuration
- [ ] Configure Release build with optimizations (`/O2`, `/GL`)
- [ ] Configure PDB generation for crash debugging
- [ ] Configure static CRT linking (`/MT`)
- [ ] Verify executable size target (<5MB)

### 12.2 Code Signing
- [ ] Obtain code signing certificate
- [ ] Configure signtool in build process
- [ ] Sign executable and installer

### 12.3 Installer
- [ ] Create Inno Setup script
- [ ] Include shell extension registration
- [ ] Include default settings
- [ ] Create portable ZIP distribution
- [ ] Create WinGet manifest

### 12.4 Release Checklist
- [ ] Update version number
- [ ] Update CHANGELOG.md
- [ ] Create GitHub Release with notes
- [ ] Upload signed installer and portable ZIP
- [ ] Update documentation/website

---

## Phase 13: Documentation

### 13.1 User Documentation
- [ ] Create user guide (Markdown or HTML)
- [ ] Document keyboard shortcuts
- [ ] Document configuration options
- [ ] Create FAQ

### 13.2 Developer Documentation
- [ ] Document architecture overview
- [ ] Document build instructions
- [ ] Document coding style guide
- [ ] Create API documentation (Doxygen)

---

## Critical Gotchas Checklist

- [ ] **DPI Awareness**: Manifest includes `<dpiAware>true</dpiAware>` and `PerMonitorV2`
- [ ] **UTF-8 Handling**: ConPTY outputs UTF-8; use correct `MultiByteToWideChar` conversion to UTF-16 for DirectWrite
- [ ] **Admin Tabs**: Cannot elevate just a tab; use helper process or launch entire app as Admin
- [ ] **Thread Safety**: Ring buffer must be lock-free or properly synchronized
- [ ] **Font Fallback**: DirectWrite handles this automatically with proper font collection
- [ ] **Resize Race**: Ensure `ResizePseudoConsole` is called synchronously before shell continues

---

## Milestones

| Milestone | Description | Target |
|-----------|-------------|--------|
| **M1** | ConPTY PoC: Console app spawning cmd.exe, printing output to stdout | Week 2 |
| **M2** | WTL Window: Static "Hello World" with DirectWrite | Week 3 |
| **M3** | Integration: ConPTY output rendered in DirectWrite window | Week 5 |
| **M4** | Interaction: Keyboard input mapped to ANSI, full shell interaction | Week 7 |
| **M5** | Tabs: Multi-tab support with proper session management | Week 9 |
| **M6** | Polish: Settings, transparency, search, URL detection | Week 12 |
| **M7** | Release: v1.0.0 with installer and documentation | Week 14 |

---

*Console3 - The next generation Windows terminal emulator*
