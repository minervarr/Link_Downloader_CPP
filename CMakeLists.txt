cmake_minimum_required(VERSION 3.16)
project(utec_downloader
    VERSION 1.0.0
    DESCRIPTION "UTEC Video Downloader - Modern C++17 parallel downloader"
    LANGUAGES CXX
)

# ============================================================================
# Build Options
# ============================================================================
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(USE_NATIVE_YTDLP "Use native C++ yt-dlp library instead of subprocess" ON)

# ============================================================================
# C++ Standard Configuration
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs and tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Compiler Flags
# ============================================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    endif()
elseif(MSVC)
    add_compile_options(/W4)
endif()

# ============================================================================
# External Dependencies (from submodules)
# ============================================================================
message(STATUS "Configuring external dependencies from submodules...")

# nlohmann/json - Header-only JSON library
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(external/json)

# fmt - Modern formatting library
set(FMT_DOC OFF CACHE INTERNAL "")
set(FMT_TEST OFF CACHE INTERNAL "")
set(FMT_INSTALL OFF CACHE INTERNAL "")
add_subdirectory(external/fmt)

# GoogleTest - Testing framework (only if tests enabled)
if(BUILD_TESTS)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK ON CACHE INTERNAL "")
    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    add_subdirectory(external/googletest)
endif()

# ============================================================================
# Native yt-dlp C++ Library (Downloader_Multi - supports Zoom!)
# ============================================================================
include(FetchContent)
set(YTDLP_NATIVE_AVAILABLE OFF)

if(USE_NATIVE_YTDLP)
    # Check if already exists locally
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/ytdlp/src")
        message(STATUS "Found local Downloader_Multi library")
        set(YTDLP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/ytdlp")
        set(YTDLP_NATIVE_AVAILABLE ON)
    else()
        # Fetch Downloader_Multi (has working Zoom extractor!)
        message(STATUS "Fetching Downloader_Multi (C++ yt-dlp with Zoom support)...")
        FetchContent_Declare(
            downloader_multi
            GIT_REPOSITORY https://github.com/minervarr/Downloader_Multi.git
            GIT_TAG        claude/refactor-ytdlp-library-01B4dxq4oxJ4VjWyT1qS7qnQ
            GIT_SHALLOW    TRUE
            GIT_SUBMODULES ""  # Don't fetch nested submodules
            SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/external/ytdlp"
        )

        FetchContent_GetProperties(downloader_multi)
        if(NOT downloader_multi_POPULATED)
            FetchContent_Populate(downloader_multi)
            set(YTDLP_SOURCE_DIR "${downloader_multi_SOURCE_DIR}")
            set(YTDLP_NATIVE_AVAILABLE ON)
            message(STATUS "Downloader_Multi fetched successfully")
        endif()
    endif()

    if(YTDLP_NATIVE_AVAILABLE)
        # Create interface library for headers
        # The library has headers in src/ directory
        add_library(ytdlp_native INTERFACE)
        target_include_directories(ytdlp_native
            INTERFACE
                ${YTDLP_SOURCE_DIR}/src
        )
        message(STATUS "Native C++ yt-dlp library enabled (with Zoom support)")
    endif()
endif()

if(NOT YTDLP_NATIVE_AVAILABLE)
    message(STATUS "Native yt-dlp not available - using subprocess fallback")
endif()

# ============================================================================
# System Dependencies
# ============================================================================
find_package(Threads REQUIRED)
find_package(CURL QUIET)
find_package(OpenSSL QUIET)

if(NOT CURL_FOUND)
    message(STATUS "libcurl not found - download functionality will use subprocess only")
endif()

if(NOT OpenSSL_FOUND)
    message(STATUS "OpenSSL not found - some features may be limited")
endif()

# ============================================================================
# Core Library
# ============================================================================
set(LIB_SOURCES
    src/core/ClassInfo.cpp
    src/core/ConfigManager.cpp
    src/core/DataParser.cpp
    src/core/SemesterClassifier.cpp
    src/core/FileOrganizer.cpp
    src/core/DownloadManager.cpp
    src/core/SubprocessYtdlp.cpp
    src/core/NativeYtdlp.cpp
    src/utils/Logger.cpp
)

add_library(utec_downloader_core ${LIB_SOURCES})

target_include_directories(utec_downloader_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(utec_downloader_core
    PUBLIC
        nlohmann_json::nlohmann_json
        fmt::fmt
        Threads::Threads
)

# Link native yt-dlp library if available
if(YTDLP_NATIVE_AVAILABLE)
    target_link_libraries(utec_downloader_core PUBLIC ytdlp_native)
    target_compile_definitions(utec_downloader_core PUBLIC USE_NATIVE_YTDLP=1)
endif()

if(CURL_FOUND)
    target_link_libraries(utec_downloader_core PUBLIC CURL::libcurl)
    target_compile_definitions(utec_downloader_core PUBLIC HAS_CURL=1)
endif()

if(OpenSSL_FOUND)
    target_link_libraries(utec_downloader_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(utec_downloader_core PUBLIC HAS_OPENSSL=1)
endif()

# ============================================================================
# Main Executable
# ============================================================================
add_executable(utec_downloader src/main.cpp)
target_link_libraries(utec_downloader PRIVATE utec_downloader_core)

# ============================================================================
# Tests
# ============================================================================
if(BUILD_TESTS)
    enable_testing()

    set(TEST_SOURCES
        tests/test_main.cpp
        tests/test_ClassInfo.cpp
        tests/test_ConfigManager.cpp
        tests/test_DataParser.cpp
        tests/test_SemesterClassifier.cpp
        tests/test_FileOrganizer.cpp
    )

    add_executable(utec_downloader_tests ${TEST_SOURCES})

    target_link_libraries(utec_downloader_tests
        PRIVATE
            utec_downloader_core
            gtest
            gmock
    )

    include(GoogleTest)
    gtest_discover_tests(utec_downloader_tests)
endif()

# ============================================================================
# Installation
# ============================================================================
install(TARGETS utec_downloader
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "")
message(STATUS "=== UTEC Downloader Build Configuration ===")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Compiler:   ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests:    ${BUILD_TESTS}")
message(STATUS "  CURL found:     ${CURL_FOUND}")
message(STATUS "  OpenSSL found:  ${OpenSSL_FOUND}")
message(STATUS "  Native yt-dlp:  ${YTDLP_NATIVE_AVAILABLE}")
message(STATUS "")
