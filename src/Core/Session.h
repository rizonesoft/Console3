#pragma once
// Console3 - Session.h
// Terminal session model - ties together PTY, buffer, and emulation

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <optional>

#include "Core/PtySession.h"
#include "Core/TerminalBuffer.h"
#include "Core/RingBuffer.h"
#include "Emulation/VTermWrapper.h"


namespace Console3::Core {

/// Session state
enum class SessionState {
    Idle,
    Running,
    Exited
};

/// Session configuration
struct SessionConfig {
    std::wstring shell = L"cmd.exe";
    std::wstring args;
    std::wstring workingDir;
    std::wstring title = L"Console3";
    std::wstring profileName;  ///< Profile name for restore
    int rows = 25;
    int cols = 80;
    size_t scrollbackLines = 10000;
    int tabIndex = 0;          ///< Tab position for restore
};

/// Exit callback type
using SessionExitCallback = std::function<void(DWORD exitCode)>;

/// Title change callback
using TitleChangeCallback = std::function<void(const std::wstring& title)>;

/// Manages a complete terminal session
class Session {
public:
    Session();
    ~Session();

    // Non-copyable, non-movable
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    /// Start a new session
    [[nodiscard]] bool Start(const SessionConfig& config);

    /// Stop the session
    void Stop();

    /// Get session state
    [[nodiscard]] SessionState GetState() const noexcept { return m_state; }

    /// Check if session is running
    [[nodiscard]] bool IsRunning() const noexcept { return m_state == SessionState::Running; }

    /// Write data to the PTY (keyboard input)
    int Write(const char* data, size_t length);

    /// Resize the terminal
    bool Resize(int cols, int rows);

    /// Get current dimensions
    [[nodiscard]] int GetRows() const noexcept { return m_rows; }
    [[nodiscard]] int GetCols() const noexcept { return m_cols; }

    /// Get session title
    [[nodiscard]] const std::wstring& GetTitle() const noexcept { return m_title; }

    /// Get the terminal buffer
    [[nodiscard]] TerminalBuffer* GetBuffer() { return m_buffer.get(); }
    [[nodiscard]] const TerminalBuffer* GetBuffer() const { return m_buffer.get(); }

    /// Get the VTerm wrapper
    [[nodiscard]] Emulation::VTermWrapper* GetVTerm() { return m_vterm.get(); }
    [[nodiscard]] const Emulation::VTermWrapper* GetVTerm() const { return m_vterm.get(); }

    /// Get the PTY session
    [[nodiscard]] PtySession* GetPty() { return m_pty.get(); }

    /// Get exit code (valid after exit)
    [[nodiscard]] DWORD GetExitCode() const noexcept { return m_exitCode; }

    /// Set exit callback
    void SetExitCallback(SessionExitCallback callback);

    /// Set title change callback
    void SetTitleChangeCallback(TitleChangeCallback callback);

    /// Process pending output (call periodically from UI thread)
    void ProcessOutput();

    // ========================================================================
    // Serialization
    // ========================================================================

    /// Get current session configuration (for serialization)
    [[nodiscard]] SessionConfig GetConfig() const;

    /// Serialize session state to JSON string
    [[nodiscard]] std::string Serialize() const;

    /// Deserialize session config from JSON string
    [[nodiscard]] static std::optional<SessionConfig> Deserialize(const std::string& json);

    /// Save all sessions to file
    static bool SaveSessions(const std::vector<SessionConfig>& sessions, const std::wstring& path);

    /// Load sessions from file
    [[nodiscard]] static std::vector<SessionConfig> LoadSessions(const std::wstring& path);

private:
    /// Handle PTY output
    void OnPtyOutput(const char* data, size_t length);

    /// Handle PTY exit
    void OnPtyExit(DWORD exitCode);

    /// Handle VTerm damage
    void OnVTermDamage(int startRow, int endRow, int startCol, int endCol);

    /// Handle VTerm property change
    void OnVTermPropChange(const Emulation::TermProps& props);

private:
    // Components
    std::unique_ptr<PtySession> m_pty;
    std::unique_ptr<TerminalBuffer> m_buffer;
    std::unique_ptr<Emulation::VTermWrapper> m_vterm;
    std::unique_ptr<ByteRingBuffer> m_outputBuffer;

    // State
    SessionState m_state = SessionState::Idle;
    int m_rows = 25;
    int m_cols = 80;
    std::wstring m_title = L"Console3";
    DWORD m_exitCode = 0;

    // Callbacks
    SessionExitCallback m_exitCallback;
    TitleChangeCallback m_titleCallback;
};

} // namespace Console3::Core
