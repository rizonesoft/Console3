#pragma once
// Console3 - ShellDetector.h
// Shell detection and enumeration utilities
//
// Detects available shells on the system and provides their paths and metadata.

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS5
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#include <Windows.h>
#include <string>
#include <vector>
#include <optional>

namespace Console3::Core {

/// Known shell types
enum class ShellType {
    Unknown,
    Cmd,            ///< Windows Command Prompt (cmd.exe)
    PowerShell,     ///< Windows PowerShell 5.x (powershell.exe)
    Pwsh,           ///< PowerShell Core 7+ (pwsh.exe)
    Wsl,            ///< Windows Subsystem for Linux (wsl.exe)
    GitBash,        ///< Git Bash (bash.exe from Git for Windows)
    Cygwin,         ///< Cygwin Bash
    Custom          ///< User-defined custom shell
};

/// Information about a detected shell
struct ShellInfo {
    ShellType type = ShellType::Unknown;
    std::wstring name;          ///< Display name (e.g., "PowerShell Core")
    std::wstring path;          ///< Full path to executable
    std::wstring args;          ///< Default arguments
    std::wstring icon;          ///< Path to icon (optional)
    std::wstring version;       ///< Version string (if detected)
    bool isDefault = false;     ///< Is this the system default shell?
    bool isAvailable = false;   ///< Is the executable present?
};

/// Shell detection and enumeration
class ShellDetector {
public:
    ShellDetector() = default;
    ~ShellDetector() = default;

    /// Detect all available shells on the system
    /// @return Vector of detected shells
    [[nodiscard]] std::vector<ShellInfo> DetectShells();

    /// Get the default shell for the system
    /// @return ShellInfo for the default shell, or nullopt if none detected
    [[nodiscard]] std::optional<ShellInfo> GetDefaultShell();

    /// Check if a specific shell is available
    /// @param type Shell type to check
    /// @return true if the shell is available
    [[nodiscard]] bool IsShellAvailable(ShellType type);

    /// Get shell info by type
    /// @param type Shell type to find
    /// @return ShellInfo if found, nullopt otherwise
    [[nodiscard]] std::optional<ShellInfo> GetShellByType(ShellType type);

    /// Detect WSL distributions
    /// @return Vector of WSL distribution names
    [[nodiscard]] std::vector<std::wstring> DetectWslDistros();

    /// Create shell info for a WSL distribution
    /// @param distroName Name of the WSL distribution
    /// @return ShellInfo for the WSL distro
    [[nodiscard]] ShellInfo CreateWslDistroShell(const std::wstring& distroName);

    /// Validate that a shell executable exists and is runnable
    /// @param path Path to the shell executable
    /// @return true if valid
    [[nodiscard]] static bool ValidateShellPath(const std::wstring& path);

    /// Get the display name for a shell type
    [[nodiscard]] static std::wstring GetShellTypeName(ShellType type);

private:
    /// Detect Windows Command Prompt
    [[nodiscard]] ShellInfo DetectCmd();

    /// Detect Windows PowerShell (5.x)
    [[nodiscard]] ShellInfo DetectPowerShell();

    /// Detect PowerShell Core (7+)
    [[nodiscard]] ShellInfo DetectPwsh();

    /// Detect WSL
    [[nodiscard]] ShellInfo DetectWsl();

    /// Detect Git Bash
    [[nodiscard]] ShellInfo DetectGitBash();

    /// Expand environment variables in a path
    [[nodiscard]] static std::wstring ExpandPath(const std::wstring& path);

    /// Check if a file exists
    [[nodiscard]] static bool FileExists(const std::wstring& path);

    /// Get file version info
    [[nodiscard]] static std::wstring GetFileVersion(const std::wstring& path);

private:
    std::vector<ShellInfo> m_cachedShells;
    bool m_cacheValid = false;
};

} // namespace Console3::Core
