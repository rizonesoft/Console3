// Console3 - IoThread.cpp
// Background I/O thread implementation

#include "Core/IoThread.h"
#include <vector>

namespace Console3::Core {

IoThread::~IoThread() {
    Stop();
}

bool IoThread::Start(const IoThreadConfig& config) {
    // Validate configuration
    if (!config.readHandle || config.readHandle == INVALID_HANDLE_VALUE) {
        m_lastError = L"Invalid read handle";
        return false;
    }

    if (!config.outputBuffer) {
        m_lastError = L"Output buffer is null";
        return false;
    }

    if (m_running.load()) {
        m_lastError = L"Thread already running";
        return false;
    }

    // Store configuration
    m_readHandle = config.readHandle;
    m_outputBuffer = config.outputBuffer;
    m_chunkSize = config.chunkSize > 0 ? config.chunkSize : 4096;

    // Reset state
    m_stopRequested.store(false);
    m_bytesRead.store(0);
    m_lastError.clear();

    // Start the thread
    m_running.store(true);
    m_thread = std::thread(&IoThread::ThreadProc, this);

    return true;
}

void IoThread::Stop(DWORD waitMs) {
    if (!m_running.load()) {
        return;
    }

    // Signal stop request
    m_stopRequested.store(true);

    // Cancel any pending I/O on the handle
    // This will cause ReadFile to return with an error
    if (m_readHandle && m_readHandle != INVALID_HANDLE_VALUE) {
        CancelIoEx(m_readHandle, nullptr);
    }

    // Wait for thread to finish
    if (m_thread.joinable()) {
        // Use a timed join approach
        auto future = std::async(std::launch::async, [this]() {
            m_thread.join();
        });

        if (future.wait_for(std::chrono::milliseconds(waitMs)) == std::future_status::timeout) {
            // Thread didn't stop in time - this is a serious issue
            // but we can't forcibly terminate std::thread safely
            m_lastError = L"Thread did not stop within timeout";
        }
    }

    m_running.store(false);
}

bool IoThread::IsRunning() const noexcept {
    return m_running.load();
}

void IoThread::SetDataAvailableCallback(DataAvailableCallback callback) {
    m_dataAvailableCallback = std::move(callback);
}

void IoThread::SetErrorCallback(ErrorCallback callback) {
    m_errorCallback = std::move(callback);
}

uint64_t IoThread::GetBytesRead() const noexcept {
    return m_bytesRead.load();
}

const std::wstring& IoThread::GetLastError() const noexcept {
    return m_lastError;
}

void IoThread::ThreadProc() {
    std::vector<char> buffer(m_chunkSize);

    while (!m_stopRequested.load()) {
        DWORD bytesRead = 0;

        // Blocking read from the pipe
        BOOL success = ReadFile(
            m_readHandle,
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            &bytesRead,
            nullptr  // Synchronous I/O
        );

        if (!success) {
            DWORD error = ::GetLastError();

            // ERROR_OPERATION_ABORTED is expected when we cancel I/O during shutdown
            if (error == ERROR_OPERATION_ABORTED) {
                break;
            }

            // ERROR_BROKEN_PIPE means the process exited
            if (error == ERROR_BROKEN_PIPE) {
                break;
            }

            // Report other errors
            if (m_errorCallback) {
                m_errorCallback(error, FormatWin32Error(error));
            }
            break;
        }

        if (bytesRead == 0) {
            // EOF - pipe closed
            break;
        }

        // Write to the ring buffer
        size_t written = 0;
        size_t remaining = bytesRead;
        const char* dataPtr = buffer.data();

        while (remaining > 0 && !m_stopRequested.load()) {
            size_t wrote = m_outputBuffer->Write(dataPtr + written, remaining);
            
            if (wrote == 0) {
                // Buffer is full - wait a bit and retry
                // This is a simple backpressure mechanism
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }

            written += wrote;
            remaining -= wrote;
        }

        // Update statistics
        m_bytesRead.fetch_add(bytesRead, std::memory_order_relaxed);

        // Notify consumer that data is available
        if (m_dataAvailableCallback && written > 0) {
            m_dataAvailableCallback();
        }
    }

    m_running.store(false);
}

std::wstring IoThread::FormatWin32Error(DWORD errorCode) {
    LPWSTR messageBuffer = nullptr;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&messageBuffer),
        0,
        nullptr
    );

    std::wstring result;
    if (size > 0 && messageBuffer) {
        // Remove trailing newlines
        while (size > 0 && (messageBuffer[size - 1] == L'\r' || messageBuffer[size - 1] == L'\n')) {
            messageBuffer[--size] = L'\0';
        }
        result = messageBuffer;
        LocalFree(messageBuffer);
    } else {
        result = L"Unknown error (code: " + std::to_wstring(errorCode) + L")";
    }

    return result;
}

} // namespace Console3::Core
