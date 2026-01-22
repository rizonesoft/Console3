// Console3 - Session.cpp
// Terminal session implementation

#include "Core/Session.h"

namespace Console3::Core {

Session::Session() = default;

Session::~Session() {
    Stop();
}

bool Session::Start(const SessionConfig& config) {
    if (m_state == SessionState::Running) {
        return false;
    }

    m_rows = config.rows;
    m_cols = config.cols;
    m_title = config.title;

    // Create terminal buffer
    TerminalBufferConfig bufConfig;
    bufConfig.rows = config.rows;
    bufConfig.cols = config.cols;
    bufConfig.scrollbackLines = config.scrollbackLines;

    try {
        m_buffer = std::make_unique<TerminalBuffer>(bufConfig);
    } catch (...) {
        return false;
    }

    // Create output ring buffer
    m_outputBuffer = std::make_unique<ByteRingBuffer>(65536);

    // Create VTerm wrapper
    try {
        m_vterm = std::make_unique<Emulation::VTermWrapper>(config.rows, config.cols);
    } catch (...) {
        return false;
    }

    // Set up VTerm callbacks
    m_vterm->SetDamageCallback([this](int sr, int er, int sc, int ec) {
        OnVTermDamage(sr, er, sc, ec);
    });

    m_vterm->SetTermPropCallback([this](const Emulation::TermProps& props) {
        OnVTermPropChange(props);
    });

    // Create PTY session
    m_pty = std::make_unique<PtySession>();

    // Set up PTY callbacks
    m_pty->SetOutputCallback([this](const char* data, size_t len) {
        OnPtyOutput(data, len);
    });

    m_pty->SetExitCallback([this](DWORD exitCode) {
        OnPtyExit(exitCode);
    });

    // Configure and start PTY
    PtyConfig ptyConfig;
    ptyConfig.shell = config.shell;
    ptyConfig.args = config.args;
    ptyConfig.workingDir = config.workingDir;
    ptyConfig.cols = config.cols;
    ptyConfig.rows = config.rows;

    if (!m_pty->Start(ptyConfig)) {
        return false;
    }

    m_state = SessionState::Running;
    return true;
}

void Session::Stop() {
    if (m_pty) {
        m_pty->Stop();
    }
    m_state = SessionState::Idle;
}

int Session::Write(const char* data, size_t length) {
    if (!m_pty || m_state != SessionState::Running) {
        return -1;
    }
    return m_pty->Write(data, length);
}

bool Session::Resize(int cols, int rows) {
    if (!m_pty || m_state != SessionState::Running) {
        return false;
    }

    // Resize PTY
    if (!m_pty->Resize(cols, rows)) {
        return false;
    }

    // Resize VTerm
    if (m_vterm) {
        m_vterm->Resize(rows, cols);
    }

    // Resize buffer
    if (m_buffer) {
        m_buffer->Resize(rows, cols);
    }

    m_cols = cols;
    m_rows = rows;

    return true;
}

void Session::SetExitCallback(SessionExitCallback callback) {
    m_exitCallback = std::move(callback);
}

void Session::SetTitleChangeCallback(TitleChangeCallback callback) {
    m_titleCallback = std::move(callback);
}

void Session::ProcessOutput() {
    if (!m_outputBuffer || !m_vterm) {
        return;
    }

    // Read data from ring buffer and feed to VTerm
    char buf[4096];
    size_t read;
    while ((read = m_outputBuffer->Read(buf, sizeof(buf))) > 0) {
        m_vterm->InputWrite(buf, read);
    }

    // Flush damage to trigger callbacks
    m_vterm->FlushDamage();
}

void Session::OnPtyOutput(const char* data, size_t length) {
    // Write to ring buffer (called from IO thread)
    if (m_outputBuffer) {
        m_outputBuffer->Write(data, length);
    }
}

void Session::OnPtyExit(DWORD exitCode) {
    m_exitCode = exitCode;
    m_state = SessionState::Exited;

    if (m_exitCallback) {
        m_exitCallback(exitCode);
    }
}

void Session::OnVTermDamage(int startRow, int endRow, int startCol, int endCol) {
    // Update terminal buffer from VTerm screen
    if (!m_buffer || !m_vterm) return;

    for (int row = startRow; row < endRow; ++row) {
        for (int col = startCol; col < endCol; ++col) {
            auto vtCell = m_vterm->GetCell(row, col);
            auto& bufCell = m_buffer->GetCell(row, col);

            // Copy character
            bufCell.charCode = vtCell.chars.empty() ? U' ' : vtCell.chars[0];
            
            // Copy combining characters
            for (size_t i = 0; i < 3 && i + 1 < vtCell.chars.size(); ++i) {
                bufCell.combining[i] = vtCell.chars[i + 1];
            }

            // Copy colors
            bufCell.fg = CellColor::Rgb(vtCell.fg.r, vtCell.fg.g, vtCell.fg.b);
            if (vtCell.fg.isDefault) bufCell.fg = CellColor::Default();
            
            bufCell.bg = CellColor::Rgb(vtCell.bg.r, vtCell.bg.g, vtCell.bg.b);
            if (vtCell.bg.isDefault) bufCell.bg = CellColor::Default();

            // Copy attributes
            bufCell.attrs.bold = vtCell.attrs.bold;
            bufCell.attrs.italic = vtCell.attrs.italic;
            bufCell.attrs.underline = vtCell.attrs.underlineStyle;
            bufCell.attrs.blink = vtCell.attrs.blink;
            bufCell.attrs.reverse = vtCell.attrs.reverse;
            bufCell.attrs.strikethrough = vtCell.attrs.strikethrough;

            bufCell.width = static_cast<uint8_t>(vtCell.width);
        }
        m_buffer->MarkDirty(row);
    }
}

void Session::OnVTermPropChange(const Emulation::TermProps& props) {
    if (m_title != props.title && !props.title.empty()) {
        m_title = props.title;
        if (m_titleCallback) {
            m_titleCallback(m_title);
        }
    }
}

// ============================================================================
// Serialization
// ============================================================================

namespace {

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, utf8.data(), size, nullptr, nullptr);
    return utf8;
}

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wide(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), size);
    return wide;
}

} // anonymous namespace

SessionConfig Session::GetConfig() const {
    SessionConfig config;
    config.shell = m_pty ? m_pty->GetConfig().shell : L"cmd.exe";
    config.args = m_pty ? m_pty->GetConfig().args : L"";
    config.workingDir = m_pty ? m_pty->GetConfig().workingDir : L"";
    config.title = m_title;
    config.rows = m_rows;
    config.cols = m_cols;
    config.scrollbackLines = m_buffer ? m_buffer->GetMaxScrollback() : 10000;
    return config;
}

std::string Session::Serialize() const {
    SessionConfig config = GetConfig();
    
    // Simple JSON serialization
    std::string json = "{\n";
    json += "  \"shell\": \"" + WideToUtf8(config.shell) + "\",\n";
    json += "  \"args\": \"" + WideToUtf8(config.args) + "\",\n";
    json += "  \"workingDir\": \"" + WideToUtf8(config.workingDir) + "\",\n";
    json += "  \"title\": \"" + WideToUtf8(config.title) + "\",\n";
    json += "  \"profileName\": \"" + WideToUtf8(config.profileName) + "\",\n";
    json += "  \"rows\": " + std::to_string(config.rows) + ",\n";
    json += "  \"cols\": " + std::to_string(config.cols) + ",\n";
    json += "  \"scrollbackLines\": " + std::to_string(config.scrollbackLines) + ",\n";
    json += "  \"tabIndex\": " + std::to_string(config.tabIndex) + "\n";
    json += "}";
    
    return json;
}

std::optional<SessionConfig> Session::Deserialize(const std::string& json) {
    SessionConfig config;
    
    // Simple JSON parsing (find key-value pairs)
    auto findString = [&json](const std::string& key) -> std::string {
        std::string searchKey = "\"" + key + "\": \"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return "";
        pos += searchKey.length();
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    };
    
    auto findInt = [&json](const std::string& key) -> int {
        std::string searchKey = "\"" + key + "\": ";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return 0;
        pos += searchKey.length();
        return std::atoi(json.c_str() + pos);
    };
    
    config.shell = Utf8ToWide(findString("shell"));
    config.args = Utf8ToWide(findString("args"));
    config.workingDir = Utf8ToWide(findString("workingDir"));
    config.title = Utf8ToWide(findString("title"));
    config.profileName = Utf8ToWide(findString("profileName"));
    config.rows = findInt("rows");
    config.cols = findInt("cols");
    config.scrollbackLines = static_cast<size_t>(findInt("scrollbackLines"));
    config.tabIndex = findInt("tabIndex");
    
    // Validation
    if (config.shell.empty()) config.shell = L"cmd.exe";
    if (config.rows <= 0) config.rows = 25;
    if (config.cols <= 0) config.cols = 80;
    if (config.scrollbackLines == 0) config.scrollbackLines = 10000;
    
    return config;
}

bool Session::SaveSessions(const std::vector<SessionConfig>& sessions, const std::wstring& path) {
    FILE* file = nullptr;
    if (_wfopen_s(&file, path.c_str(), L"w") != 0 || !file) {
        return false;
    }
    
    fprintf(file, "[\n");
    for (size_t i = 0; i < sessions.size(); ++i) {
        const auto& config = sessions[i];
        fprintf(file, "  {\n");
        fprintf(file, "    \"shell\": \"%s\",\n", WideToUtf8(config.shell).c_str());
        fprintf(file, "    \"args\": \"%s\",\n", WideToUtf8(config.args).c_str());
        fprintf(file, "    \"workingDir\": \"%s\",\n", WideToUtf8(config.workingDir).c_str());
        fprintf(file, "    \"title\": \"%s\",\n", WideToUtf8(config.title).c_str());
        fprintf(file, "    \"profileName\": \"%s\",\n", WideToUtf8(config.profileName).c_str());
        fprintf(file, "    \"rows\": %d,\n", config.rows);
        fprintf(file, "    \"cols\": %d,\n", config.cols);
        fprintf(file, "    \"scrollbackLines\": %zu,\n", config.scrollbackLines);
        fprintf(file, "    \"tabIndex\": %d\n", config.tabIndex);
        fprintf(file, "  }%s\n", (i < sessions.size() - 1) ? "," : "");
    }
    fprintf(file, "]\n");
    
    fclose(file);
    return true;
}

std::vector<SessionConfig> Session::LoadSessions(const std::wstring& path) {
    std::vector<SessionConfig> sessions;
    
    FILE* file = nullptr;
    if (_wfopen_s(&file, path.c_str(), L"r") != 0 || !file) {
        return sessions;
    }
    
    // Read entire file
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    std::string content(size, '\0');
    fread(content.data(), 1, size, file);
    fclose(file);
    
    // Parse array of sessions (simple parsing)
    size_t pos = 0;
    while ((pos = content.find("{", pos)) != std::string::npos) {
        size_t end = content.find("}", pos);
        if (end == std::string::npos) break;
        
        std::string sessionJson = content.substr(pos, end - pos + 1);
        auto config = Deserialize(sessionJson);
        if (config) {
            sessions.push_back(*config);
        }
        pos = end + 1;
    }
    
    return sessions;
}

} // namespace Console3::Core

