#!/usr/bin/env bash
# Quick screenshot script for SkyRoads
# Usage: ./scripts/screenshot.sh [level] [palette] [seed]

set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
LEVEL="${1:-1}"
PALETTE="${2:-0}"
SEED="${3:-0xC0FFEE}"
OUTPUT_DIR="${SCREENSHOT_OUTPUT_DIR:-docs/screenshots-raw}"
INTERVAL="${SCREENSHOT_INTERVAL:-1200}"  # Every 10 seconds

# Colors
CYAN='\033[0;36m'
GREEN='\033[0;32m'
NC='\033[0m'

echo -e "${CYAN}Taking screenshots...${NC}"
echo "Level: ${LEVEL}, Palette: ${PALETTE}, Seed: ${SEED}"

mkdir -p "${OUTPUT_DIR}"

"${BUILD_DIR}/sim_runner" \
    --seed "${SEED}" \
    --level "${LEVEL}" \
    --palette "${PALETTE}" \
    --screenshots \
    --screenshot-output "${OUTPUT_DIR}" \
    --screenshot-interval "${INTERVAL}" \
    --ticks 6000

echo -e "${GREEN}✅ Screenshots saved to ${OUTPUT_DIR}${NC}"
