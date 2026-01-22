# Console3 - Modern Windows Terminal Emulator
# Root CMake Configuration

cmake_minimum_required(VERSION 3.21)

project(Console3
    VERSION 1.0.0
    DESCRIPTION "Lightweight native Windows terminal emulator using ConPTY"
    LANGUAGES CXX
)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC-specific settings
if(MSVC)
    add_compile_options(
        /W4             # Warning level 4
        /utf-8          # UTF-8 source and execution charset
        /permissive-    # Strict conformance mode
        /Zc:__cplusplus # Correct __cplusplus macro
    )
    
    # Release optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
endif()

# AVX2 Optimization Option
option(ENABLE_AVX2 "Enable AVX2 SIMD optimizations (x64 only)" OFF)
if(ENABLE_AVX2 AND MSVC)
    message(STATUS "AVX2 optimizations ENABLED")
    add_compile_options(/arch:AVX2)
    add_compile_definitions(CONSOLE3_AVX2=1)
endif()

# Output directories (can be overridden by build scripts)
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

# Per-configuration output directories (for multi-config generators like VS)
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
endforeach()

# Windows-specific definitions
add_compile_definitions(
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0A00  # Windows 10+
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/vendor/wil/include
    ${CMAKE_SOURCE_DIR}/vendor/wtl/Include
    ${CMAKE_SOURCE_DIR}/vendor/libvterm/include
)

# Fetch nlohmann/json (header-only, downloads at configure time)
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Subdirectories
add_subdirectory(vendor/libvterm)
add_subdirectory(src)
add_subdirectory(tests)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
