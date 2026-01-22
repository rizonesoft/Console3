// Console3 - PtySession.cpp
// ConPTY (Windows Pseudo Console) wrapper implementation
//
// Implements pipe creation, pseudo console lifecycle, and process management

#include "Core/PtySession.h"
#include <array>
#include <cassert>
#include <vector>

namespace Console3::Core {

// Buffer size for PTY I/O operations
constexpr DWORD kPtyBufferSize = 4096;

PtySession::~PtySession() { Stop(); }

bool PtySession::Start(const PtyConfig &config) {
  // Ensure we're not already running
  if (m_running.load()) {
    m_lastError = L"Session already running";
    return false;
  }

  // Store initial size
  m_cols = config.cols;
  m_rows = config.rows;

  // Step 1: Create the pipes for PTY communication
  if (!CreatePipes()) {
    return false;
  }

  // Step 2: Create the pseudo console with initial size
  if (!CreatePseudoConsoleHandle(config.cols, config.rows)) {
    return false;
  }

  // Step 3: Launch the shell process
  if (!LaunchProcess(config)) {
    return false;
  }

  // Step 4: Start the I/O reader thread
  m_running.store(true);
  m_ioThread = std::thread(&PtySession::IoThreadProc, this);

  return true;
}

void PtySession::Stop() {
  // Signal thread to stop
  m_running.store(false);

  // Close the pseudo console first - this will cause the shell to exit
  // and unblock any pending ReadFile calls
  m_hPCon.reset();

  // Wait for IO thread to finish
  if (m_ioThread.joinable()) {
    m_ioThread.join();
  }

  // Terminate process if still running
  if (m_processInfo.hProcess) {
    TerminateProcess(m_processInfo.hProcess, 0);
    m_processInfo.reset();
  }

  // Close all pipes
  m_pipeIn.reset();
  m_pipeOut.reset();
  m_ptyIn.reset();
  m_ptyOut.reset();
}

bool PtySession::IsRunning() const noexcept { return m_running.load(); }

int PtySession::Write(const char *data, size_t length) {
  if (!m_running.load() || !m_ptyIn) {
    return -1;
  }

  DWORD bytesWritten = 0;
  if (!WriteFile(m_ptyIn.get(), data, static_cast<DWORD>(length), &bytesWritten,
                 nullptr)) {
    SetLastErrorFromWin32();
    return -1;
  }

  return static_cast<int>(bytesWritten);
}

int PtySession::Write(std::string_view text) {
  return Write(text.data(), text.size());
}

bool PtySession::Resize(int cols, int rows) {
  if (!m_hPCon) {
    m_lastError = L"Pseudo console not initialized";
    return false;
  }

  COORD size{};
  size.X = static_cast<SHORT>(cols);
  size.Y = static_cast<SHORT>(rows);

  HRESULT hr = ResizePseudoConsole(m_hPCon.get(), size);
  if (FAILED(hr)) {
    m_lastError =
        L"ResizePseudoConsole failed with HRESULT: " + std::to_wstring(hr);
    return false;
  }

  m_cols = cols;
  m_rows = rows;
  return true;
}

void PtySession::SetOutputCallback(OutputCallback callback) {
  m_outputCallback = std::move(callback);
}

void PtySession::SetExitCallback(ExitCallback callback) {
  m_exitCallback = std::move(callback);
}

std::pair<int, int> PtySession::GetSize() const noexcept {
  return {m_cols, m_rows};
}

DWORD PtySession::GetProcessId() const noexcept {
  return m_processInfo.dwProcessId;
}

const std::wstring &PtySession::GetLastError() const noexcept {
  return m_lastError;
}

// ============================================================================
// Private Implementation
// ============================================================================

bool PtySession::CreatePipes() {
  // We need two pairs of pipes:
  // 1. pipeIn/ptyIn: We write to ptyIn, PTY reads from pipeIn
  // 2. ptyOut/pipeOut: PTY writes to pipeOut, we read from ptyOut
  //
  // Pipe configuration:
  //   [Our App] --(ptyIn)--> [PTY] --(pipeOut)--> (internal)
  //   [Our App] <--(ptyOut)-- [PTY] <--(pipeIn)-- (internal)

  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE; // Child process needs to inherit these handles
  sa.lpSecurityDescriptor = nullptr;

  // Create the INPUT pipe pair (our writes go to the shell's stdin)
  // We write to m_ptyIn, the PTY reads from m_pipeIn
  HANDLE hPipeInRead = nullptr;
  HANDLE hPipeInWrite = nullptr;
  if (!CreatePipe(&hPipeInRead, &hPipeInWrite, &sa, 0)) {
    SetLastErrorFromWin32();
    return false;
  }
  m_pipeIn.reset(hPipeInRead);
  m_ptyIn.reset(hPipeInWrite);

  // Ensure our write end is not inherited by child process
  if (!SetHandleInformation(m_ptyIn.get(), HANDLE_FLAG_INHERIT, 0)) {
    SetLastErrorFromWin32();
    return false;
  }

  // Create the OUTPUT pipe pair (shell's stdout comes to us)
  // PTY writes to m_pipeOut, we read from m_ptyOut
  HANDLE hPipeOutRead = nullptr;
  HANDLE hPipeOutWrite = nullptr;
  if (!CreatePipe(&hPipeOutRead, &hPipeOutWrite, &sa, 0)) {
    SetLastErrorFromWin32();
    return false;
  }
  m_ptyOut.reset(hPipeOutRead);
  m_pipeOut.reset(hPipeOutWrite);

  // Ensure our read end is not inherited by child process
  if (!SetHandleInformation(m_ptyOut.get(), HANDLE_FLAG_INHERIT, 0)) {
    SetLastErrorFromWin32();
    return false;
  }

  return true;
}

bool PtySession::CreatePseudoConsoleHandle(int cols, int rows) {
  COORD size{};
  size.X = static_cast<SHORT>(cols);
  size.Y = static_cast<SHORT>(rows);

  // CreatePseudoConsole requires:
  // - Input handle: where PTY reads from (our m_pipeIn read end)
  // - Output handle: where PTY writes to (our m_pipeOut write end)
  HPCON hPCon = nullptr;
  HRESULT hr = CreatePseudoConsole(size,
                                   m_pipeIn.get(),  // PTY reads input from here
                                   m_pipeOut.get(), // PTY writes output here
                                   0, // dwFlags (0 for default behavior)
                                   &hPCon);

  if (FAILED(hr)) {
    m_lastError =
        L"CreatePseudoConsole failed with HRESULT: " + std::to_wstring(hr);
    return false;
  }

  // Store in RAII wrapper
  m_hPCon.reset(hPCon);
  return true;
}

bool PtySession::LaunchProcess(const PtyConfig &config) {
  // Initialize the startup info with the pseudo console attribute
  STARTUPINFOEXW siEx{};
  siEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);

  // Calculate the required attribute list size
  SIZE_T attrListSize = 0;
  InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize);
  if (attrListSize == 0) {
    SetLastErrorFromWin32();
    return false;
  }

  // Allocate and initialize the attribute list
  std::vector<BYTE> attrListBuffer(attrListSize);
  siEx.lpAttributeList =
      reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrListBuffer.data());

  if (!InitializeProcThreadAttributeList(siEx.lpAttributeList, 1, 0,
                                         &attrListSize)) {
    SetLastErrorFromWin32();
    return false;
  }

  // Add the pseudo console attribute
  if (!UpdateProcThreadAttribute(
          siEx.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
          m_hPCon.get(), sizeof(HPCON), nullptr, nullptr)) {
    DeleteProcThreadAttributeList(siEx.lpAttributeList);
    SetLastErrorFromWin32();
    return false;
  }

  // Build the command line
  std::wstring cmdLine = config.shell;
  if (!config.args.empty()) {
    cmdLine += L" " + config.args;
  }

  // Launch the process
  PROCESS_INFORMATION procInfo{};
  BOOL success =
      CreateProcessW(nullptr,        // lpApplicationName
                     cmdLine.data(), // lpCommandLine (mutable)
                     nullptr,        // lpProcessAttributes
                     nullptr,        // lpThreadAttributes
                     FALSE,          // bInheritHandles
                     EXTENDED_STARTUPINFO_PRESENT |
                         CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
                     nullptr,                        // lpEnvironment
                     config.workingDir.empty()
                         ? nullptr
                         : config.workingDir.c_str(), // lpCurrentDirectory
                     &siEx.StartupInfo,               // lpStartupInfo
                     &procInfo                        // lpProcessInformation
      );

  // Clean up attribute list regardless of success
  DeleteProcThreadAttributeList(siEx.lpAttributeList);

  if (!success) {
    SetLastErrorFromWin32();
    return false;
  }

  // Store the process information using WIL RAII wrapper
  m_processInfo.hProcess = procInfo.hProcess;
  m_processInfo.hThread = procInfo.hThread;
  m_processInfo.dwProcessId = procInfo.dwProcessId;
  m_processInfo.dwThreadId = procInfo.dwThreadId;

  return true;
}

void PtySession::IoThreadProc() {
  std::array<char, kPtyBufferSize> buffer{};

  while (m_running.load()) {
    DWORD bytesRead = 0;

    // Read from the PTY output pipe (blocking call)
    BOOL success =
        ReadFile(m_ptyOut.get(), buffer.data(),
                 static_cast<DWORD>(buffer.size()), &bytesRead, nullptr);

    if (!success || bytesRead == 0) {
      // Pipe closed or error - shell probably exited
      break;
    }

    // Dispatch output to callback
    if (m_outputCallback && bytesRead > 0) {
      m_outputCallback(buffer.data(), static_cast<size_t>(bytesRead));
    }
  }

  // Check for process exit
  if (m_processInfo.hProcess) {
    DWORD exitCode = 0;
    if (GetExitCodeProcess(m_processInfo.hProcess, &exitCode)) {
      if (exitCode != STILL_ACTIVE && m_exitCallback) {
        m_exitCallback(exitCode);
      }
    }
  }

  m_running.store(false);
}

void PtySession::SetLastErrorFromWin32() {
  DWORD errorCode = ::GetLastError();

  LPWSTR messageBuffer = nullptr;
  DWORD size = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);

  if (size > 0 && messageBuffer) {
    // Remove trailing newlines
    while (size > 0 && (messageBuffer[size - 1] == L'\r' ||
                        messageBuffer[size - 1] == L'\n')) {
      messageBuffer[--size] = L'\0';
    }
    m_lastError = messageBuffer;
    LocalFree(messageBuffer);
  } else {
    m_lastError = L"Unknown error (code: " + std::to_wstring(errorCode) + L")";
  }
}

} // namespace Console3::Core
