#!/usr/bin/env bash
set -e

ARCH=${1:-"x86_64"}
APP_DIR="AppDir"

echo "Building AppImage for architecture: ${ARCH}"

rm -rf "${APP_DIR}"
mkdir -p "${APP_DIR}/usr/bin"
mkdir -p "${APP_DIR}/usr/share/applications"
mkdir -p "${APP_DIR}/usr/share/icons/hicolor/scalable/apps"

# Copy binary and assets
cp -v build/liteforge "${APP_DIR}/usr/bin/"
cp -v assets/liteforge.desktop "${APP_DIR}/usr/share/applications/"
cp -v assets/liteforge.desktop "${APP_DIR}/"
cp -v assets/icon.svg "${APP_DIR}/usr/share/icons/hicolor/scalable/apps/liteforge.svg"
cp -v assets/icon.svg "${APP_DIR}/liteforge.svg"
cp -v packaging/appimage/AppRun "${APP_DIR}/AppRun"
chmod +x "${APP_DIR}/AppRun"

# Download appimagetool if not present
if [ ! -f "appimagetool" ]; then
    if [ "${ARCH}" = "x86_64" ]; then
        AI_URL="https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
    else
        AI_URL="https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-aarch64.AppImage"
    fi
    echo "Downloading ${AI_URL}..."
    curl -sLo appimagetool "${AI_URL}" || true
    chmod +x appimagetool || true
fi

# Package AppImage
if [ -x "./appimagetool" ]; then
    ARCH="${ARCH}" ./appimagetool --appimage-extract-and-run "${APP_DIR}" "LiteForge-${ARCH}.AppImage" || true
else
    echo "Creating fallback archive bundle..."
    tar -czf "LiteForge-${ARCH}.tar.gz" -C "${APP_DIR}" .
fi

echo "AppImage build finished."
