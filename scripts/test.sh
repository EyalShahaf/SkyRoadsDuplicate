#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"

cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-Release}" -DFETCHCONTENT_QUIET=OFF
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE:-Release}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure
