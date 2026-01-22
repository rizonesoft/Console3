# Console3

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Build Status](https://github.com/rizonesoft/Console3/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/rizonesoft/Console3/actions)
[![Windows](https://img.shields.io/badge/Platform-Windows%2010%2F11-0078D6?logo=windows)](https://github.com/rizonesoft/Console3)

A lightweight, native Windows terminal emulator built on the modern **ConPTY** (Windows Pseudo Console) API with **Direct2D** hardware-accelerated rendering.

## ✨ Features

- 🚀 **Native Performance** - Built with C++20 and WTL, no Electron/web bloat
- 🎨 **Hardware Accelerated** - Direct2D + DirectWrite for smooth rendering
- 📑 **Multi-Tab Interface** - Run multiple shells in one window
- 🔤 **Font Ligatures** - Full support for programming fonts (Fira Code, JetBrains Mono)
- 🌍 **Unicode & Emoji** - Complete UTF-8 support with font fallback
- 🎯 **TrueColor** - Full 24-bit color support
- 🖥️ **High-DPI** - Per-monitor DPI awareness for 4K displays
- ⚡ **Lightweight** - Small executable size (<5MB), zero runtime dependencies

## 🛠️ Tech Stack

| Component | Technology |
|-----------|------------|
| Language | C++20 (MSVC v144) |
| Build System | CMake 4.1+ |
| OS Backend | Windows ConPTY API |
| Windowing | WTL (Windows Template Library) |
| Rendering | Direct2D + DirectWrite |
| VT Parser | libvterm |
| Helpers | WIL (Windows Implementation Libraries) |

## 📋 Requirements

- Windows 10 version 1809+ or Windows 11
- Visual Studio 2026 Build Tools (MSVC v144)
- CMake 3.21+

## 🔧 Building

```powershell
# Clone the repository
git clone https://github.com/rizonesoft/Console3.git
cd Console3

# Configure with CMake
cmake -B build -G "Visual Studio 18 2026" -A x64

# Build
cmake --build build --config Release

# Run
.\build\bin\Release\Console3.exe
```

### Build Variants

```powershell
# x64 Release
cmake -B build-x64 -G "Visual Studio 18 2026" -A x64

# ARM64
cmake -B build-arm64 -G "Visual Studio 18 2026" -A ARM64

# x64 with AVX2
cmake -B build-avx2 -G "Visual Studio 18 2026" -A x64 -DENABLE_AVX2=ON
```

## 📁 Project Structure

```
Console3/
├── src/
│   ├── Core/           # ConPTY, terminal buffer, IO
│   ├── UI/             # WTL windows, Direct2D rendering
│   └── Emulation/      # libvterm wrapper
├── vendor/
│   ├── wtl/            # Windows Template Library
│   ├── wil/            # Windows Implementation Libraries
│   └── libvterm/       # VT terminal emulator library
├── tests/              # Unit tests (Google Test)
├── docs/               # Documentation
└── assets/             # Icons, resources
```

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting a pull request.

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [Console2](https://sourceforge.net/projects/console/) - The original inspiration
- [Windows Terminal](https://github.com/microsoft/terminal) - For ConPTY API documentation
- [libvterm](https://github.com/neovim/libvterm) - VT terminal emulation library
- [WTL](https://github.com/Win32-WTL/WTL) - Windows Template Library
