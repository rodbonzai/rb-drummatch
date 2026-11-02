#!/usr/bin/env bash
set -e

# Package KitMatch macOS Universal Installer (.dmg)
VERSION="1.0.0-beta.1"
APP_NAME="KitMatch"
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
APP_BUNDLE="${BUILD_DIR}/KitMatch_artefacts/Release/${APP_NAME}.app"
DIST_DIR="${PROJECT_DIR}/dist"
STAGING_DIR="${DIST_DIR}/dmg_staging"
DMG_NAME="${APP_NAME}-v${VERSION}-macOS-Universal.dmg"
DMG_PATH="${DIST_DIR}/${DMG_NAME}"

echo "=========================================="
echo "Packaging ${APP_NAME} v${VERSION} for macOS"
echo "=========================================="

if [ ! -d "${APP_BUNDLE}" ]; then
    echo "Error: ${APP_BUNDLE} does not exist. Run CMake Release build first."
    exit 1
fi

# Verify Universal Binary (Apple Silicon + Intel)
ARCHS=$(lipo -info "${APP_BUNDLE}/Contents/MacOS/${APP_NAME}")
echo "Binary architecture: ${ARCHS}"

if [[ "${ARCHS}" != *"x86_64"* ]] || [[ "${ARCHS}" != *"arm64"* ]]; then
    echo "Warning: Binary is not Universal 2 (both x86_64 and arm64). Got: ${ARCHS}"
else
    echo "Verified: Universal 2 Binary (Intel + Apple Silicon supported)."
fi

# Clean and create staging
rm -rf "${STAGING_DIR}" "${DMG_PATH}"
mkdir -p "${STAGING_DIR}" "${DIST_DIR}"

# Copy app bundle
echo "Copying ${APP_NAME}.app to staging..."
cp -R "${APP_BUNDLE}" "${STAGING_DIR}/"

# Create symlink to /Applications for drag-and-drop install
echo "Creating Applications shortcut..."
ln -s /Applications "${STAGING_DIR}/Applications"

# Add instructions
cat << 'EOF' > "${STAGING_DIR}/INSTRUCCIONES.txt"
==================================================
KitMatch - Smart Acoustic Drum Matching Workstation
Version 1.0.0-beta.1 (Universal: Apple Silicon + Intel)
==================================================

INSTALACION EN MACOS:
1. Arrastra el icono de "KitMatch" a la carpeta "Applications" (Aplicaciones).
2. Abre KitMatch desde la carpeta Aplicaciones o desde Spotlight (Cmd + Espacio).

NOTA DE SEGURIDAD (Gatekeeper):
Si macOS muestra una alerta de "Desarrollador no identificado" al abrir la app por primera vez:
- Haz clic derecho (o Control + Clic) en KitMatch en la carpeta Aplicaciones.
- Selecciona "Abrir".
- En el cuadro de diálogo, haz clic en "Abrir".
(Solo es necesario realizar este paso la primera vez).

Desarrollado con JUCE & C++20 por Rod Bonzai.
EOF

# Create DMG with hdiutil
echo "Building compressed DMG..."
hdiutil create \
    -volname "${APP_NAME} Installer" \
    -srcfolder "${STAGING_DIR}" \
    -ov \
    -format UDZO \
    "${DMG_PATH}"

# Generate checksum
echo "Generating SHA256 checksum..."
cd "${DIST_DIR}"
shasum -a 256 "${DMG_NAME}" > "${DMG_NAME}.sha256"

# Cleanup staging
rm -rf "${STAGING_DIR}"

echo "=========================================="
echo "Successfully created installer:"
echo "DMG: ${DMG_PATH}"
echo "Checksum: $(cat "${DMG_PATH}.sha256")"
echo "=========================================="
