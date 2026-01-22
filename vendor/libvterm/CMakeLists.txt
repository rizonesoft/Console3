# libvterm CMake build for MSVC
# This file enables building libvterm with Visual Studio

cmake_minimum_required(VERSION 3.21)

project(libvterm
    VERSION 0.3.3
    LANGUAGES C
)

# Source files
set(LIBVTERM_SOURCES
    src/encoding.c
    src/keyboard.c
    src/mouse.c
    src/parser.c
    src/pen.c
    src/screen.c
    src/state.c
    src/unicode.c
    src/vterm.c
)

# Create static library
add_library(vterm STATIC ${LIBVTERM_SOURCES})

target_include_directories(vterm
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# MSVC-specific settings
if(MSVC)
    target_compile_definitions(vterm PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
    )
    target_compile_options(vterm PRIVATE
        /W3
        /utf-8
    )
endif()

# Install headers for use by parent project
set_target_properties(vterm PROPERTIES
    PUBLIC_HEADER "include/vterm.h;include/vterm_keycodes.h"
)
