#!/bin/bash
set -e

# UTEC Downloader Build Script

BUILD_DIR="build"
BUILD_TYPE="${1:-Release}"

echo "Building UTEC Downloader..."
echo "Build type: $BUILD_TYPE"

# Initialize submodules if needed
if [ ! -f "external/json/CMakeLists.txt" ]; then
    echo "Initializing git submodules..."
    git submodule update --init --recursive
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_TESTS=ON

# Build
echo "Building..."
cmake --build . --parallel $(nproc 2>/dev/null || echo 4)

echo ""
echo "Build complete!"
echo "Executable: $BUILD_DIR/utec_downloader"
echo "Tests: $BUILD_DIR/utec_downloader_tests"
echo ""
echo "Run tests with: cd $BUILD_DIR && ctest --output-on-failure"
