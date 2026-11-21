#!/usr/bin/env bash
# Builds a self-contained AppImage (Qt libraries + platform plugins bundled).
#
#   packaging/appimage/build-appimage.sh [x86_64|aarch64|armhf] [version]
#
# Requires a finished build in ./build (cmake --build build) and qmake6 on PATH.
# NOTE: the AppImage inherits the glibc of the machine that builds it, so it
# runs on distributions at least as new as the build image (Ubuntu 24.04 -> glibc 2.39).
set -euo pipefail

cd "$(dirname "$0")/../.."
ROOT="$PWD"

case "${1:-$(uname -m)}" in
    x86_64|amd64)            ARCH=x86_64 ;;
    aarch64|arm64)           ARCH=aarch64 ;;
    armhf|armv7|armv7l|arm)  ARCH=armhf ;;
    *) echo "unsupported AppImage architecture: ${1:-$(uname -m)}" >&2; exit 1 ;;
esac
VERSION="${2:-$(sed -n 's/^ *VERSION \([0-9.]*\)$/\1/p' CMakeLists.txt | head -n1)}"
BUILD_DIR="${BUILD_DIR:-build}"
TOOLS="${TOOLS_DIR:-$ROOT/.appimage-tools}"
mkdir -p "$TOOLS"

fetch() { # url dest
    [ -x "$2" ] || { echo "Downloading $1"; curl -fsSL --retry 3 -o "$2" "$1"; chmod +x "$2"; }
}
LD="$TOOLS/linuxdeploy-$ARCH.AppImage"
LDQT="$TOOLS/linuxdeploy-plugin-qt-$ARCH.AppImage"
fetch "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage" "$LD"
fetch "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage" "$LDQT"

rm -rf AppDir
DESTDIR="$ROOT/AppDir" cmake --install "$BUILD_DIR" --prefix /usr --strip

export PATH="$TOOLS:$PATH"                  # linuxdeploy finds its plugin by name
ln -sf "$LDQT" "$TOOLS/linuxdeploy-plugin-qt"
export APPIMAGE_EXTRACT_AND_RUN=1           # no FUSE needed (CI, containers, QEMU)
export QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake)}"
export EXTRA_PLATFORM_PLUGINS="libqwayland-generic.so;libqwayland-egl.so"
export LINUXDEPLOY_OUTPUT_VERSION="$VERSION"
export OUTPUT="LiteForge-$VERSION-$ARCH.AppImage"
export ARCH

"$LD" --appdir AppDir \
    --desktop-file AppDir/usr/share/applications/liteforge.desktop \
    --icon-file assets/icon.svg \
    --plugin qt \
    --output appimage

echo "Built $OUTPUT"
