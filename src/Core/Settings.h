#pragma once
// Console3 - Settings.h
// Application settings model and persistence

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace Console3::Core {

/// Font settings
struct FontSettings {
    std::wstring family = L"Consolas";
    float size = 12.0f;
    bool bold = false;
    bool italic = false;
};

/// Color scheme (16 ANSI colors + extras)
struct ColorScheme {
    std::wstring name = L"Default";
    
    // Basic colors
    uint32_t foreground = 0xCCCCCC;
    uint32_t background = 0x0C0C0C;
    uint32_t cursorColor = 0xFFFFFF;
    uint32_t selectionBackground = 0x264F78;

    // ANSI 16 colors (0-15)
    uint32_t palette[16] = {
        0x0C0C0C, // Black
        0xC50F1F, // Red
        0x13A10E, // Green
        0xC19C00, // Yellow
        0x0037DA, // Blue
        0x881798, // Magenta
        0x3A96DD, // Cyan
        0xCCCCCC, // White
        0x767676, // Bright Black
        0xE74856, // Bright Red
        0x16C60C, // Bright Green
        0xF9F1A5, // Bright Yellow
        0x3B78FF, // Bright Blue
        0xB4009E, // Bright Magenta
        0x61D6D6, // Bright Cyan
        0xF2F2F2  // Bright White
    };
};

/// Shell profile
struct ShellProfile {
    std::wstring name = L"Default";
    std::wstring shell;
    std::wstring args;
    std::wstring workingDir;
    std::wstring icon;
    bool hidden = false;
};

/// Cursor settings
struct CursorSettings {
    std::wstring style = L"block";  // block, underline, bar
    bool blink = true;
    uint32_t blinkRate = 530;  // milliseconds
};

/// Window settings
struct WindowSettings {
    int width = 800;
    int height = 600;
    bool startMaximized = false;
    bool confirmClose = true;
    float opacity = 1.0f;
    bool useAcrylic = false;
};

/// Keyboard shortcut
struct Shortcut {
    std::wstring action;
    std::wstring keys;  // e.g., "Ctrl+Shift+C"
};

/// Tab behavior settings
struct TabSettings {
    std::wstring newTabPosition = L"afterCurrent";  // afterCurrent, atEnd
    std::wstring closeLastTabAction = L"closeWindow";  // closeWindow, newTab
    int tabWidthMin = 100;
    int tabWidthMax = 200;
    bool showCloseButton = true;
    bool confirmTabClose = false;
    bool duplicateOnMiddleClick = false;
    bool showNewTabButton = true;
    bool restoreTabsOnStartup = true;
};

/// Application settings
struct Settings {
    // General
    std::wstring defaultProfile;
    int scrollbackLines = 10000;
    bool copyOnSelect = false;
    bool wordWrap = false;

    // Appearance
    FontSettings font;
    ColorScheme colorScheme;
    CursorSettings cursor;
    WindowSettings window;
    TabSettings tabs;

    // Profiles
    std::vector<ShellProfile> profiles;

    // Keyboard shortcuts
    std::vector<Shortcut> shortcuts;

    /// Get default settings
    static Settings GetDefaults();
};

/// Settings manager - load/save settings
class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager() = default;

    /// Load settings from file
    /// @return true on success
    bool Load();

    /// Save settings to file
    /// @return true on success
    bool Save();

    /// Reset to defaults
    void ResetToDefaults();

    /// Get current settings
    [[nodiscard]] Settings& GetSettings() { return m_settings; }
    [[nodiscard]] const Settings& GetSettings() const { return m_settings; }

    /// Get settings file path
    [[nodiscard]] std::filesystem::path GetSettingsPath() const;

    /// Check if settings file exists
    [[nodiscard]] bool SettingsFileExists() const;

    /// Get the last error message
    [[nodiscard]] const std::wstring& GetLastError() const { return m_lastError; }

private:
    Settings m_settings;
    std::wstring m_lastError;
};

} // namespace Console3::Core
