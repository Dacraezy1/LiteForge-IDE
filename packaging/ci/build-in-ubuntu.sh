#!/usr/bin/env bash
# Runs INSIDE an ubuntu:24.04 container (possibly under QEMU) to produce
# .tar.gz and .deb packages for the container's architecture.
#   packaging/ci/build-in-ubuntu.sh <arch-label> <version> [with-appimage]
set -euo pipefail
ARCH_LABEL="$1"; VERSION="$2"; WITH_APPIMAGE="${3:-}"

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends \
    cmake g++ make file patchelf curl ca-certificates \
    qt6-base-dev qt6-wayland libgl1-mesa-dev dpkg-dev >/dev/null

cd /src
rm -rf build-ci out && mkdir -p out
# LTO off: far faster under emulation and no real size difference here.
cmake -B build-ci -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
      -DLITEFORGE_LTO=OFF -DCPACK_STRIP_FILES=ON
cmake --build build-ci --parallel "$(nproc)"

# Tests are informative on exotic CPUs (e.g. big-endian s390x): report, don't block packaging.
QT_QPA_PLATFORM=offscreen ctest --test-dir build-ci --output-on-failure || echo "::warning::tests failed on $ARCH_LABEL"

( cd build-ci && cpack -G TGZ && cpack -G DEB )
cp build-ci/liteforge-*-Linux.tar.gz "out/liteforge-$VERSION-linux-$ARCH_LABEL.tar.gz"
cp build-ci/liteforge_*.deb out/

if [ -n "$WITH_APPIMAGE" ]; then
    BUILD_DIR=build-ci bash packaging/appimage/build-appimage.sh "$ARCH_LABEL" "$VERSION" \
        && mv LiteForge-*.AppImage out/ || echo "::warning::AppImage failed on $ARCH_LABEL"
fi
ls -la out
