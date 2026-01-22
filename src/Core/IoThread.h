#pragma once
// Console3 - IoThread.h
// Background I/O thread for PTY output reading
//
// This class manages a dedicated thread that reads from the ConPTY output pipe
// and writes data to a ring buffer for consumption by the terminal emulator.

// Target Windows 10 RS5 (1809) or later for ConPTY APIs
#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS5
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

#include "Core/RingBuffer.h"

namespace Console3::Core {

/// Callback type for notifying when data is available
using DataAvailableCallback = std::function<void()>;

/// Callback type for notifying thread errors
using ErrorCallback = std::function<void(DWORD errorCode, const std::wstring& message)>;

/// Configuration for the I/O thread
struct IoThreadConfig {
    HANDLE readHandle = nullptr;           ///< Pipe handle to read from
    ByteRingBuffer* outputBuffer = nullptr; ///< Ring buffer to write to
    size_t chunkSize = 4096;               ///< Read chunk size in bytes
};

/// Background thread for reading PTY output
/// 
/// This thread continuously reads from the PTY output pipe in blocking mode
/// and writes the data to a lock-free ring buffer. The terminal emulator
/// consumes data from the ring buffer at its own pace.
class IoThread {
public:
    IoThread() = default;
    ~IoThread();

    // Non-copyable, non-movable
    IoThread(const IoThread&) = delete;
    IoThread& operator=(const IoThread&) = delete;
    IoThread(IoThread&&) = delete;
    IoThread& operator=(IoThread&&) = delete;

    /// Start the I/O thread
    /// @param config Thread configuration
    /// @return true on success
    [[nodiscard]] bool Start(const IoThreadConfig& config);

    /// Stop the I/O thread gracefully
    /// @param waitMs Maximum milliseconds to wait for thread to stop
    void Stop(DWORD waitMs = 5000);

    /// Check if the thread is running
    [[nodiscard]] bool IsRunning() const noexcept;

    /// Set callback for data available notification
    /// Called from the I/O thread when new data is written to the buffer
    void SetDataAvailableCallback(DataAvailableCallback callback);

    /// Set callback for error notification
    void SetErrorCallback(ErrorCallback callback);

    /// Get total bytes read since start
    [[nodiscard]] uint64_t GetBytesRead() const noexcept;

    /// Get the last error message
    [[nodiscard]] const std::wstring& GetLastError() const noexcept;

private:
    /// Main thread procedure
    void ThreadProc();

    /// Format a Win32 error code as a message
    static std::wstring FormatWin32Error(DWORD errorCode);

private:
    // Configuration
    HANDLE m_readHandle = nullptr;
    ByteRingBuffer* m_outputBuffer = nullptr;
    size_t m_chunkSize = 4096;

    // Thread management
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};

    // Callbacks
    DataAvailableCallback m_dataAvailableCallback;
    ErrorCallback m_errorCallback;

    // Statistics
    std::atomic<uint64_t> m_bytesRead{0};

    // Error state
    std::wstring m_lastError;
};

} // namespace Console3::Core
