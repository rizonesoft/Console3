// Console3 - Settings.cpp
// Settings persistence using JSON

#include "Core/Settings.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <ShlObj.h>

namespace Console3::Core {

using json = nlohmann::json;

// ============================================================================
// JSON Serialization Helpers
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

std::string ColorToHex(uint32_t color) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%06X", color & 0xFFFFFF);
    return buf;
}

uint32_t HexToColor(const std::string& hex) {
    if (hex.empty()) return 0;
    const char* str = hex.c_str();
    if (*str == '#') str++;
    return static_cast<uint32_t>(strtoul(str, nullptr, 16));
}

} // namespace

// ============================================================================
// Settings Defaults
// ============================================================================

Settings Settings::GetDefaults() {
    Settings s;

    // Default profiles
    ShellProfile cmdProfile;
    cmdProfile.name = L"Command Prompt";
    cmdProfile.shell = L"cmd.exe";
    s.profiles.push_back(cmdProfile);

    ShellProfile pwshProfile;
    pwshProfile.name = L"PowerShell";
    pwshProfile.shell = L"powershell.exe";
    pwshProfile.args = L"-NoLogo";
    s.profiles.push_back(pwshProfile);

    s.defaultProfile = L"PowerShell";

    // Default shortcuts
    s.shortcuts = {
        {L"copy", L"Ctrl+Shift+C"},
        {L"paste", L"Ctrl+Shift+V"},
        {L"newTab", L"Ctrl+Shift+T"},
        {L"closeTab", L"Ctrl+Shift+W"},
        {L"find", L"Ctrl+Shift+F"},
        {L"settings", L"Ctrl+,"},
        {L"nextTab", L"Ctrl+Tab"},
        {L"prevTab", L"Ctrl+Shift+Tab"}
    };

    return s;
}

// ============================================================================
// SettingsManager Implementation
// ============================================================================

SettingsManager::SettingsManager() {
    m_settings = Settings::GetDefaults();
}

std::filesystem::path SettingsManager::GetSettingsPath() const {
    wchar_t* appDataPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath))) {
        std::filesystem::path path = appDataPath;
        CoTaskMemFree(appDataPath);
        return path / L"Console3" / L"settings.json";
    }
    return L"settings.json";
}

bool SettingsManager::SettingsFileExists() const {
    return std::filesystem::exists(GetSettingsPath());
}

bool SettingsManager::Load() {
    auto path = GetSettingsPath();
    
    if (!std::filesystem::exists(path)) {
        // Use defaults if no settings file
        m_settings = Settings::GetDefaults();
        return true;
    }

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            m_lastError = L"Could not open settings file";
            return false;
        }

        json j = json::parse(file);

        // Parse general settings
        if (j.contains("defaultProfile")) {
            m_settings.defaultProfile = Utf8ToWide(j["defaultProfile"]);
        }
        if (j.contains("scrollbackLines")) {
            m_settings.scrollbackLines = j["scrollbackLines"];
        }
        if (j.contains("copyOnSelect")) {
            m_settings.copyOnSelect = j["copyOnSelect"];
        }

        // Parse font
        if (j.contains("font")) {
            auto& font = j["font"];
            if (font.contains("family")) {
                m_settings.font.family = Utf8ToWide(font["family"]);
            }
            if (font.contains("size")) {
                m_settings.font.size = font["size"];
            }
        }

        // Parse color scheme
        if (j.contains("colorScheme")) {
            auto& cs = j["colorScheme"];
            if (cs.contains("name")) {
                m_settings.colorScheme.name = Utf8ToWide(cs["name"]);
            }
            if (cs.contains("foreground")) {
                m_settings.colorScheme.foreground = HexToColor(cs["foreground"]);
            }
            if (cs.contains("background")) {
                m_settings.colorScheme.background = HexToColor(cs["background"]);
            }
            if (cs.contains("cursor")) {
                m_settings.colorScheme.cursorColor = HexToColor(cs["cursor"]);
            }
            if (cs.contains("selection")) {
                m_settings.colorScheme.selectionBackground = HexToColor(cs["selection"]);
            }
            if (cs.contains("palette") && cs["palette"].is_array()) {
                auto& palette = cs["palette"];
                for (size_t i = 0; i < 16 && i < palette.size(); ++i) {
                    m_settings.colorScheme.palette[i] = HexToColor(palette[i]);
                }
            }
        }

        // Parse cursor
        if (j.contains("cursor")) {
            auto& cur = j["cursor"];
            if (cur.contains("style")) {
                m_settings.cursor.style = Utf8ToWide(cur["style"]);
            }
            if (cur.contains("blink")) {
                m_settings.cursor.blink = cur["blink"];
            }
            if (cur.contains("blinkRate")) {
                m_settings.cursor.blinkRate = cur["blinkRate"];
            }
        }

        // Parse window
        if (j.contains("window")) {
            auto& win = j["window"];
            if (win.contains("width")) m_settings.window.width = win["width"];
            if (win.contains("height")) m_settings.window.height = win["height"];
            if (win.contains("startMaximized")) m_settings.window.startMaximized = win["startMaximized"];
            if (win.contains("confirmClose")) m_settings.window.confirmClose = win["confirmClose"];
            if (win.contains("opacity")) m_settings.window.opacity = win["opacity"];
            if (win.contains("useAcrylic")) m_settings.window.useAcrylic = win["useAcrylic"];
        }

        // Parse profiles
        if (j.contains("profiles") && j["profiles"].is_array()) {
            m_settings.profiles.clear();
            for (auto& p : j["profiles"]) {
                ShellProfile profile;
                if (p.contains("name")) profile.name = Utf8ToWide(p["name"]);
                if (p.contains("shell")) profile.shell = Utf8ToWide(p["shell"]);
                if (p.contains("args")) profile.args = Utf8ToWide(p["args"]);
                if (p.contains("workingDir")) profile.workingDir = Utf8ToWide(p["workingDir"]);
                if (p.contains("hidden")) profile.hidden = p["hidden"];
                m_settings.profiles.push_back(profile);
            }
        }

        // Parse shortcuts
        if (j.contains("shortcuts") && j["shortcuts"].is_array()) {
            m_settings.shortcuts.clear();
            for (auto& s : j["shortcuts"]) {
                Shortcut shortcut;
                if (s.contains("action")) shortcut.action = Utf8ToWide(s["action"]);
                if (s.contains("keys")) shortcut.keys = Utf8ToWide(s["keys"]);
                m_settings.shortcuts.push_back(shortcut);
            }
        }

        return true;

    } catch (const std::exception& e) {
        m_lastError = Utf8ToWide(e.what());
        return false;
    }
}

bool SettingsManager::Save() {
    auto path = GetSettingsPath();
    
    // Create directory if needed
    std::filesystem::create_directories(path.parent_path());

    try {
        json j;

        // General
        j["defaultProfile"] = WideToUtf8(m_settings.defaultProfile);
        j["scrollbackLines"] = m_settings.scrollbackLines;
        j["copyOnSelect"] = m_settings.copyOnSelect;
        j["wordWrap"] = m_settings.wordWrap;

        // Font
        j["font"]["family"] = WideToUtf8(m_settings.font.family);
        j["font"]["size"] = m_settings.font.size;
        j["font"]["bold"] = m_settings.font.bold;
        j["font"]["italic"] = m_settings.font.italic;

        // Color scheme
        j["colorScheme"]["name"] = WideToUtf8(m_settings.colorScheme.name);
        j["colorScheme"]["foreground"] = ColorToHex(m_settings.colorScheme.foreground);
        j["colorScheme"]["background"] = ColorToHex(m_settings.colorScheme.background);
        j["colorScheme"]["cursor"] = ColorToHex(m_settings.colorScheme.cursorColor);
        j["colorScheme"]["selection"] = ColorToHex(m_settings.colorScheme.selectionBackground);
        
        json palette = json::array();
        for (int i = 0; i < 16; ++i) {
            palette.push_back(ColorToHex(m_settings.colorScheme.palette[i]));
        }
        j["colorScheme"]["palette"] = palette;

        // Cursor
        j["cursor"]["style"] = WideToUtf8(m_settings.cursor.style);
        j["cursor"]["blink"] = m_settings.cursor.blink;
        j["cursor"]["blinkRate"] = m_settings.cursor.blinkRate;

        // Window
        j["window"]["width"] = m_settings.window.width;
        j["window"]["height"] = m_settings.window.height;
        j["window"]["startMaximized"] = m_settings.window.startMaximized;
        j["window"]["confirmClose"] = m_settings.window.confirmClose;
        j["window"]["opacity"] = m_settings.window.opacity;
        j["window"]["useAcrylic"] = m_settings.window.useAcrylic;

        // Profiles
        json profiles = json::array();
        for (const auto& p : m_settings.profiles) {
            json profile;
            profile["name"] = WideToUtf8(p.name);
            profile["shell"] = WideToUtf8(p.shell);
            profile["args"] = WideToUtf8(p.args);
            profile["workingDir"] = WideToUtf8(p.workingDir);
            profile["hidden"] = p.hidden;
            profiles.push_back(profile);
        }
        j["profiles"] = profiles;

        // Shortcuts
        json shortcuts = json::array();
        for (const auto& s : m_settings.shortcuts) {
            json shortcut;
            shortcut["action"] = WideToUtf8(s.action);
            shortcut["keys"] = WideToUtf8(s.keys);
            shortcuts.push_back(shortcut);
        }
        j["shortcuts"] = shortcuts;

        // Write to file
        std::ofstream file(path);
        if (!file.is_open()) {
            m_lastError = L"Could not open settings file for writing";
            return false;
        }

        file << j.dump(2);  // Pretty print with 2-space indent
        return true;

    } catch (const std::exception& e) {
        m_lastError = Utf8ToWide(e.what());
        return false;
    }
}

void SettingsManager::ResetToDefaults() {
    m_settings = Settings::GetDefaults();
}

} // namespace Console3::Core
