#!/bin/sh
# Build script for FreeBSD and OpenBSD systems
set -e

echo "=== Building LiteForge on BSD ==="

# Check for cmake
if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: cmake is required. Install via 'pkg install cmake' (FreeBSD) or 'pkg_add cmake' (OpenBSD)."
    exit 1
fi

# Configure
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_INSTALL_PREFIX="/usr/local"

# Build
cmake --build build --parallel

# Test
ctest --test-dir build --output-on-failure

echo "=== LiteForge build completed successfully on BSD ==="
echo "Run with: ./build/liteforge"
echo "Install with: sudo cmake --install build"
