#pragma once
// Console3 - PtySession.h
// ConPTY (Windows Pseudo Console) wrapper class
//
// This class manages a single pseudo console session, including:
// - Creating pipes for input/output
// - Creating the pseudo console via CreatePseudoConsole
// - Launching the shell process
// - Resizing the terminal
// - Graceful shutdown

// Target Windows 10 RS5 (1809) or later for ConPTY APIs
#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS5
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#include <Windows.h>
#include <atomic>
#include <functional>
#include <string>
#include <thread>


// WIL for RAII handle management
#include <wil/resource.h>

namespace Console3::Core {

/// Callback type for receiving output data from the PTY
using OutputCallback = std::function<void(const char *data, size_t length)>;

/// Callback type for process exit notification
using ExitCallback = std::function<void(DWORD exitCode)>;

/// Configuration for creating a PTY session
struct PtyConfig {
  std::wstring shell = L"cmd.exe"; ///< Shell executable path
  std::wstring args;               ///< Command line arguments
  std::wstring workingDir;         ///< Initial working directory
  int cols = 80;                   ///< Initial column count
  int rows = 25;                   ///< Initial row count
};

/// RAII wrapper for HPCON (Pseudo Console handle)
struct HpconDeleter {
  void operator()(HPCON hpc) const noexcept {
    if (hpc) {
      ClosePseudoConsole(hpc);
    }
  }
};
using unique_hpcon =
    std::unique_ptr<std::remove_pointer_t<HPCON>, HpconDeleter>;

/// Manages a single ConPTY session
class PtySession {
public:
  PtySession() = default;
  ~PtySession();

  // Non-copyable, movable
  PtySession(const PtySession &) = delete;
  PtySession &operator=(const PtySession &) = delete;
  PtySession(PtySession &&) noexcept = default;
  PtySession &operator=(PtySession &&) noexcept = default;

  /// Start a new PTY session with the given configuration
  /// @param config Session configuration
  /// @return true on success, false on failure
  [[nodiscard]] bool Start(const PtyConfig &config);

  /// Stop the PTY session and terminate the process
  void Stop();

  /// Check if the session is currently running
  [[nodiscard]] bool IsRunning() const noexcept;

  /// Write data to the PTY input (sends to shell)
  /// @param data Data buffer to write
  /// @param length Length of data in bytes
  /// @return Number of bytes written, or -1 on error
  [[nodiscard]] int Write(const char *data, size_t length);

  /// Write a string to the PTY input
  /// @param text String to write
  /// @return Number of bytes written, or -1 on error
  [[nodiscard]] int Write(std::string_view text);

  /// Resize the pseudo console
  /// @param cols New column count
  /// @param rows New row count
  /// @return true on success
  [[nodiscard]] bool Resize(int cols, int rows);

  /// Set callback for receiving output data
  void SetOutputCallback(OutputCallback callback);

  /// Set callback for process exit notification
  void SetExitCallback(ExitCallback callback);

  /// Get the current terminal size
  [[nodiscard]] std::pair<int, int> GetSize() const noexcept;

  /// Get the process ID of the shell
  [[nodiscard]] DWORD GetProcessId() const noexcept;

  /// Get the last error message
  [[nodiscard]] const std::wstring &GetLastError() const noexcept;

private:
  /// Create the input/output pipes
  bool CreatePipes();

  /// Create the pseudo console
  bool CreatePseudoConsoleHandle(int cols, int rows);

  /// Launch the shell process
  bool LaunchProcess(const PtyConfig &config);

  /// IO thread function - reads from PTY output
  void IoThreadProc();

  /// Set last error from Windows error code
  void SetLastErrorFromWin32();

private:
  // Handles (WIL RAII wrappers)
  wil::unique_hfile m_pipeIn;  ///< Pipe for writing to PTY
  wil::unique_hfile m_pipeOut; ///< Pipe for reading from PTY
  wil::unique_hfile m_ptyIn;   ///< PTY input pipe (we write here)
  wil::unique_hfile m_ptyOut;  ///< PTY output pipe (we read here)
  unique_hpcon m_hPCon;        ///< Pseudo console handle
  wil::unique_process_information m_processInfo; ///< Shell process info

  // IO thread
  std::thread m_ioThread;
  std::atomic<bool> m_running{false};

  // Callbacks
  OutputCallback m_outputCallback;
  ExitCallback m_exitCallback;

  // State
  int m_cols = 80;
  int m_rows = 25;
  std::wstring m_lastError;
};

} // namespace Console3::Core
