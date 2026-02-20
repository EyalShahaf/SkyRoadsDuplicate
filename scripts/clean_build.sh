#!/usr/bin/env bash
# Clean build directory, especially useful when raylib build gets stuck

set -euo pipefail

BUILD_DIR="${1:-build}"

echo "Cleaning build directory: ${BUILD_DIR}"

if [[ -d "${BUILD_DIR}" ]]; then
    # Remove raylib build cache to force rebuild
    if [[ -d "${BUILD_DIR}/_deps/raylib-build" ]]; then
        echo "Removing raylib build cache..."
        rm -rf "${BUILD_DIR}/_deps/raylib-build"
    fi
    
    # Optionally remove entire build directory
    if [[ "${2:-}" == "--full" ]]; then
        echo "Removing entire build directory..."
        rm -rf "${BUILD_DIR}"
    else
        echo "Build directory cleaned. Run ./scripts/build.sh to rebuild."
    fi
else
    echo "Build directory does not exist: ${BUILD_DIR}"
fi
