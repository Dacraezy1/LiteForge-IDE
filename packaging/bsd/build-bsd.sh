#!/bin/sh
# Build LiteForge from source on FreeBSD / OpenBSD / NetBSD / DragonFly.
#
#   FreeBSD:  pkg install cmake qt6-base
#   OpenBSD:  pkg_add cmake qt6
#   NetBSD:   pkgin install cmake qt6-qtbase
set -eu

cd "$(dirname "$0")/../.."

command -v cmake >/dev/null 2>&1 || {
    echo "error: cmake not found (FreeBSD: pkg install cmake qt6-base | OpenBSD: pkg_add cmake qt6)" >&2
    exit 1
}

PREFIX="${PREFIX:-/usr/local}"
JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 2)}"

echo "=== Building LiteForge for $(uname -s) $(uname -m) ==="
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_PREFIX_PATH="$PREFIX;$PREFIX/lib/qt6;/usr/pkg" \
    -DLITEFORGE_LTO=OFF
cmake --build build --parallel "$JOBS"

echo "=== Running tests (headless) ==="
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure || \
    echo "warning: tests failed or the Qt 'offscreen' plugin is unavailable; continuing"

echo "=== Done ==="
echo "Run:      ./build/liteforge"
echo "Install:  doas cmake --install build   (or: sudo cmake --install build)"
