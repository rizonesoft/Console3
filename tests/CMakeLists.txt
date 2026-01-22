# Console3 Tests CMake Configuration

# Option to enable/disable tests
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    # Fetch GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # Prevent overriding parent project's compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    # Test executable
    add_executable(Console3Tests
        test_main.cpp
        test_RingBuffer.cpp
        test_TerminalBuffer.cpp
        test_VTermWrapper.cpp
        test_Settings.cpp
    )

    target_link_libraries(Console3Tests
        PRIVATE
            Console3Core
            Console3Emulation
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
    )

    target_include_directories(Console3Tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )

    include(GoogleTest)
    gtest_discover_tests(Console3Tests)
endif()
