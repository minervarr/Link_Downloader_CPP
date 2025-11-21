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
# System Dependencies
# ============================================================================
find_package(Threads REQUIRED)
find_package(CURL QUIET)

if(NOT CURL_FOUND)
    message(STATUS "libcurl not found - download functionality will use subprocess only")
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

if(CURL_FOUND)
    target_link_libraries(utec_downloader_core PUBLIC CURL::libcurl)
    target_compile_definitions(utec_downloader_core PUBLIC HAS_CURL=1)
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
message(STATUS "")
