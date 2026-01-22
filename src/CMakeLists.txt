# Console3 Source CMake Configuration

# Core library (ConPTY, Terminal Buffer, IO)
add_library(Console3Core STATIC
    Core/PtySession.cpp
    Core/TerminalBuffer.cpp
    Core/IoThread.cpp
    Core/RingBuffer.cpp
    Core/Session.cpp
    Core/Settings.cpp
)

target_include_directories(Console3Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Console3Core
    PRIVATE
        kernel32
)

# Emulation library (libvterm wrapper)
add_library(Console3Emulation STATIC
    Emulation/VTermWrapper.cpp
)

target_include_directories(Console3Emulation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Console3Emulation
    PRIVATE
        Console3Core
        # libvterm will be linked here
)

# UI library (WTL + Direct2D)
add_library(Console3UI STATIC
    UI/MainFrame.cpp
    UI/TerminalView.cpp
    UI/TabControl.cpp
    UI/D2DRenderer.cpp
    UI/DirectWriteFont.cpp
)

target_include_directories(Console3UI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Console3UI
    PRIVATE
        Console3Core
        Console3Emulation
        d2d1
        dwrite
        dxgi
        dwmapi
        comctl32
        uxtheme
)

# Main executable
add_executable(Console3 WIN32
    main.cpp
    Console3.rc  # Resource file (will be created later)
)

target_link_libraries(Console3
    PRIVATE
        Console3Core
        Console3Emulation
        Console3UI
)

# Set Windows subsystem
set_target_properties(Console3 PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DPI_AWARE "PerMonitor"
)
