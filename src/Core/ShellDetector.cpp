// Console3 - ShellDetector.cpp
// Shell detection and enumeration implementation

#include "Core/ShellDetector.h"
#include <algorithm>
#include <array>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "version.lib")

namespace Console3::Core {

std::vector<ShellInfo> ShellDetector::DetectShells() {
    if (m_cacheValid) {
        return m_cachedShells;
    }

    m_cachedShells.clear();

    // Detect each known shell type
    auto cmd = DetectCmd();
    if (cmd.isAvailable) {
        m_cachedShells.push_back(std::move(cmd));
    }

    auto powershell = DetectPowerShell();
    if (powershell.isAvailable) {
        m_cachedShells.push_back(std::move(powershell));
    }

    auto pwsh = DetectPwsh();
    if (pwsh.isAvailable) {
        m_cachedShells.push_back(std::move(pwsh));
    }

    auto wsl = DetectWsl();
    if (wsl.isAvailable) {
        m_cachedShells.push_back(std::move(wsl));
    }

    auto gitBash = DetectGitBash();
    if (gitBash.isAvailable) {
        m_cachedShells.push_back(std::move(gitBash));
    }

    // Mark first available shell as default
    if (!m_cachedShells.empty()) {
        m_cachedShells[0].isDefault = true;
    }

    m_cacheValid = true;
    return m_cachedShells;
}

std::optional<ShellInfo> ShellDetector::GetDefaultShell() {
    auto shells = DetectShells();
    
    // Prefer PowerShell Core, then PowerShell, then Cmd
    for (const auto& shell : shells) {
        if (shell.type == ShellType::Pwsh) {
            return shell;
        }
    }
    for (const auto& shell : shells) {
        if (shell.type == ShellType::PowerShell) {
            return shell;
        }
    }
    for (const auto& shell : shells) {
        if (shell.type == ShellType::Cmd) {
            return shell;
        }
    }

    if (!shells.empty()) {
        return shells[0];
    }

    return std::nullopt;
}

bool ShellDetector::IsShellAvailable(ShellType type) {
    auto shells = DetectShells();
    return std::any_of(shells.begin(), shells.end(),
        [type](const ShellInfo& s) { return s.type == type && s.isAvailable; });
}

std::optional<ShellInfo> ShellDetector::GetShellByType(ShellType type) {
    auto shells = DetectShells();
    auto it = std::find_if(shells.begin(), shells.end(),
        [type](const ShellInfo& s) { return s.type == type; });
    
    if (it != shells.end()) {
        return *it;
    }
    return std::nullopt;
}

std::vector<std::wstring> ShellDetector::DetectWslDistros() {
    std::vector<std::wstring> distros;

    // Try to enumerate WSL distributions from registry
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Lxss",
        0,
        KEY_READ,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        return distros;
    }

    DWORD index = 0;
    wchar_t subKeyName[256];
    DWORD subKeyNameSize;

    while (true) {
        subKeyNameSize = static_cast<DWORD>(std::size(subKeyName));
        result = RegEnumKeyExW(hKey, index++, subKeyName, &subKeyNameSize,
                               nullptr, nullptr, nullptr, nullptr);
        
        if (result != ERROR_SUCCESS) {
            break;
        }

        // Open the distro subkey to get its name
        HKEY hDistroKey;
        if (RegOpenKeyExW(hKey, subKeyName, 0, KEY_READ, &hDistroKey) == ERROR_SUCCESS) {
            wchar_t distroName[256];
            DWORD distroNameSize = sizeof(distroName);
            DWORD type;

            if (RegQueryValueExW(hDistroKey, L"DistributionName", nullptr, &type,
                                 reinterpret_cast<LPBYTE>(distroName), &distroNameSize) == ERROR_SUCCESS) {
                if (type == REG_SZ) {
                    distros.emplace_back(distroName);
                }
            }
            RegCloseKey(hDistroKey);
        }
    }

    RegCloseKey(hKey);
    return distros;
}

ShellInfo ShellDetector::CreateWslDistroShell(const std::wstring& distroName) {
    ShellInfo info;
    info.type = ShellType::Wsl;
    info.name = L"WSL: " + distroName;
    info.path = ExpandPath(L"%SystemRoot%\\System32\\wsl.exe");
    info.args = L"-d " + distroName;
    info.isAvailable = FileExists(info.path);
    return info;
}

bool ShellDetector::ValidateShellPath(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    std::wstring expanded = ExpandPath(path);
    return FileExists(expanded);
}

std::wstring ShellDetector::GetShellTypeName(ShellType type) {
    switch (type) {
        case ShellType::Cmd:        return L"Command Prompt";
        case ShellType::PowerShell: return L"Windows PowerShell";
        case ShellType::Pwsh:       return L"PowerShell";
        case ShellType::Wsl:        return L"WSL";
        case ShellType::GitBash:    return L"Git Bash";
        case ShellType::Cygwin:     return L"Cygwin";
        case ShellType::Custom:     return L"Custom";
        default:                    return L"Unknown";
    }
}

// ============================================================================
// Private Implementation
// ============================================================================

ShellInfo ShellDetector::DetectCmd() {
    ShellInfo info;
    info.type = ShellType::Cmd;
    info.name = L"Command Prompt";
    info.path = ExpandPath(L"%ComSpec%");
    
    // Fallback if COMSPEC is not set
    if (info.path.empty() || !FileExists(info.path)) {
        info.path = ExpandPath(L"%SystemRoot%\\System32\\cmd.exe");
    }

    info.isAvailable = FileExists(info.path);
    if (info.isAvailable) {
        info.version = GetFileVersion(info.path);
    }

    return info;
}

ShellInfo ShellDetector::DetectPowerShell() {
    ShellInfo info;
    info.type = ShellType::PowerShell;
    info.name = L"Windows PowerShell";
    info.path = ExpandPath(L"%SystemRoot%\\System32\\WindowsPowerShell\\v1.0\\powershell.exe");
    info.args = L"-NoLogo";
    info.isAvailable = FileExists(info.path);
    
    if (info.isAvailable) {
        info.version = GetFileVersion(info.path);
    }

    return info;
}

ShellInfo ShellDetector::DetectPwsh() {
    ShellInfo info;
    info.type = ShellType::Pwsh;
    info.name = L"PowerShell";
    info.args = L"-NoLogo";

    // Check common installation paths for PowerShell Core
    std::array<std::wstring, 3> searchPaths = {
        ExpandPath(L"%ProgramFiles%\\PowerShell\\7\\pwsh.exe"),
        ExpandPath(L"%ProgramFiles(x86)%\\PowerShell\\7\\pwsh.exe"),
        ExpandPath(L"%LocalAppData%\\Microsoft\\WindowsApps\\pwsh.exe")  // Windows Store version
    };

    for (const auto& path : searchPaths) {
        if (FileExists(path)) {
            info.path = path;
            info.isAvailable = true;
            info.version = GetFileVersion(path);
            break;
        }
    }

    return info;
}

ShellInfo ShellDetector::DetectWsl() {
    ShellInfo info;
    info.type = ShellType::Wsl;
    info.name = L"WSL";
    info.path = ExpandPath(L"%SystemRoot%\\System32\\wsl.exe");
    info.isAvailable = FileExists(info.path);

    return info;
}

ShellInfo ShellDetector::DetectGitBash() {
    ShellInfo info;
    info.type = ShellType::GitBash;
    info.name = L"Git Bash";
    info.args = L"--login -i";

    // Check common installation paths for Git Bash
    std::array<std::wstring, 3> searchPaths = {
        ExpandPath(L"%ProgramFiles%\\Git\\bin\\bash.exe"),
        ExpandPath(L"%ProgramFiles(x86)%\\Git\\bin\\bash.exe"),
        ExpandPath(L"%LocalAppData%\\Programs\\Git\\bin\\bash.exe")
    };

    for (const auto& path : searchPaths) {
        if (FileExists(path)) {
            info.path = path;
            info.isAvailable = true;
            break;
        }
    }

    return info;
}

std::wstring ShellDetector::ExpandPath(const std::wstring& path) {
    wchar_t buffer[MAX_PATH];
    DWORD result = ExpandEnvironmentStringsW(path.c_str(), buffer, MAX_PATH);
    
    if (result > 0 && result < MAX_PATH) {
        return std::wstring(buffer);
    }
    return path;
}

bool ShellDetector::FileExists(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring ShellDetector::GetFileVersion(const std::wstring& path) {
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
    
    if (size == 0) {
        return L"";
    }

    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoW(path.c_str(), handle, size, buffer.data())) {
        return L"";
    }

    VS_FIXEDFILEINFO* fileInfo = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<void**>(&fileInfo), &len)) {
        return L"";
    }

    return std::to_wstring(HIWORD(fileInfo->dwFileVersionMS)) + L"." +
           std::to_wstring(LOWORD(fileInfo->dwFileVersionMS)) + L"." +
           std::to_wstring(HIWORD(fileInfo->dwFileVersionLS)) + L"." +
           std::to_wstring(LOWORD(fileInfo->dwFileVersionLS));
}

} // namespace Console3::Core
