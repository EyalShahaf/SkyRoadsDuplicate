#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

if [[ -f "${BUILD_DIR}/skyroads" ]]; then
    exec "${BUILD_DIR}/skyroads"
elif [[ -f "${BUILD_DIR}/${BUILD_TYPE}/skyroads" ]]; then
    exec "${BUILD_DIR}/${BUILD_TYPE}/skyroads"
else
    echo "Could not find built executable. Run ./scripts/build.sh first."
    exit 1
fi
