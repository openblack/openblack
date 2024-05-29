#!/usr/bin/env bash

# Check if the version argument is provided
# without revision (for a tagged release): git describe
# with revision (once a release exists):   git describe --long --abbrev=7 | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <version>"
    exit 1
fi

# Set the version variable
VERSION=$1

set -ex
set -o pipefail

# use RAM disk if possible
if [ -z "$CI" ] && [ -d /dev/shm ]; then
    TEMP_BASE=/dev/shm
else
    TEMP_BASE=/tmp
fi

BUILD_DIR=$(mktemp -d -p "$TEMP_BASE" openblack-appimage-build-XXXXXX)

cleanup () {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}

if [ -z "$NO_CLEANUP" ]; then
    trap cleanup EXIT
fi

# store repo root as variable
REPO_ROOT=$(readlink -f "$(dirname "$0")/../../../..")
OLD_CWD=$(readlink -f .)

pushd "$BUILD_DIR"

# standard linuxdeploy pattern
#see https://docs.appimage.org/packaging-guide/from-source/index.html for more information
cmake -S"$REPO_ROOT" -B "$BUILD_DIR"/cmake-build-appimage -DOPENBLACK_USE_VCPKG=ON -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_CONFIGURATION_TYPES=Release
cmake --build "$BUILD_DIR"/cmake-build-appimage --parallel "$(nproc)"
cmake --install "$BUILD_DIR"/cmake-build-appimage -v --strip --prefix AppDir/usr

curl -fLO https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy*.AppImage

NO_STRIP=true ./linuxdeploy*.AppImage --appimage-extract-and-run --appdir AppDir/ --output appimage

mv -v openblack*.AppImage "$OLD_CWD"
