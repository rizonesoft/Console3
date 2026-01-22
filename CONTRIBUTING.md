# Contributing to Console3

Thank you for your interest in contributing to Console3! This document provides guidelines for contributing to the project.

## Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md).

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in [Issues](https://github.com/rizonesoft/Console3/issues)
2. If not, create a new issue with:
   - A clear, descriptive title
   - Steps to reproduce the bug
   - Expected vs actual behavior
   - Windows version and build number
   - Console3 version

### Suggesting Features

1. Check existing [Issues](https://github.com/rizonesoft/Console3/issues) and [Discussions](https://github.com/rizonesoft/Console3/discussions)
2. Create a feature request with:
   - Clear description of the feature
   - Use case and benefits
   - Any relevant mockups or examples

### Pull Requests

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes following the coding standards below
4. Write or update tests as needed
5. Ensure all tests pass: `ctest --test-dir build`
6. Commit with clear messages: `git commit -m "Add: feature description"`
7. Push and create a Pull Request

## Development Setup

```powershell
# Clone your fork
git clone https://github.com/YOUR_USERNAME/Console3.git
cd Console3

# Configure
cmake -B build -G "Visual Studio 18 2026" -A x64

# Build
cmake --build build --config Debug

# Run tests
ctest --test-dir build --build-config Debug
```

## Coding Standards

### C++ Style

- Use C++20 features where appropriate
- Follow Microsoft's C++ Core Guidelines
- Use `snake_case` for variables and functions
- Use `PascalCase` for classes and structs
- Use `UPPER_CASE` for constants and macros
- Prefer `std::string_view` over `const char*`
- Use RAII (WIL helpers) for resource management

### Commit Messages

- Use present tense: "Add feature" not "Added feature"
- Prefix with type: `Add:`, `Fix:`, `Update:`, `Remove:`, `Refactor:`
- Keep the first line under 72 characters
- Reference issues when applicable: `Fix: crash on resize (#123)`

## Questions?

Feel free to open a [Discussion](https://github.com/rizonesoft/Console3/discussions) for any questions!
