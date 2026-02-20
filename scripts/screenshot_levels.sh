#!/usr/bin/env bash
# Screenshot automation script for SkyRoads
# Generates screenshots for all levels with different palettes

set -euo pipefail

BUILD_DIR="${1:-build}"
OUTPUT_DIR="${SCREENSHOT_OUTPUT_DIR:-docs/screenshots-raw}"
SEED="${SCREENSHOT_SEED:-0xC0FFEE}"
INTERVAL="${SCREENSHOT_INTERVAL:-2400}"  # Screenshot every 20 seconds at 120Hz

# Colors
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color
BOLD='\033[1m'

print_header() {
    echo -e "\n${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${CYAN}$1${NC}"
    echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
}

print_info() {
    echo -e "${CYAN}ℹ️  $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warn() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

# Check if sim_runner exists
if [[ ! -f "${BUILD_DIR}/sim_runner" ]]; then
    print_warn "sim_runner not found. Building..."
    ./scripts/build.sh "${BUILD_DIR}"
fi

print_header "SkyRoads Screenshot Automation"

print_info "Output directory: ${OUTPUT_DIR}"
print_info "Seed: ${SEED}"
print_info "Screenshot interval: ${INTERVAL} ticks (every ~20 seconds)"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

TOTAL_LEVELS=30
TOTAL_PALETTES=3
TOTAL=$((TOTAL_LEVELS * TOTAL_PALETTES))
CURRENT=0

START_TIME=$(date +%s)

for level in $(seq 1 ${TOTAL_LEVELS}); do
    for palette in $(seq 0 $((TOTAL_PALETTES - 1))); do
        CURRENT=$((CURRENT + 1))
        
        LEVEL_DIR="${OUTPUT_DIR}/level_${level}/palette_${palette}"
        mkdir -p "${LEVEL_DIR}"
        
        print_info "[${CURRENT}/${TOTAL}] Level ${level}, Palette ${palette}..."
        
        # Run sim_runner with screenshots
        "${BUILD_DIR}/sim_runner" \
            --seed "${SEED}" \
            --level "${level}" \
            --palette "${palette}" \
            --screenshots \
            --screenshot-output "${LEVEL_DIR}" \
            --screenshot-interval "${INTERVAL}" \
            --ticks 12000 \
            --quiet || true  # Continue even if bot dies
        
        print_success "Completed level ${level}, palette ${palette}"
    done
done

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
MINUTES=$((ELAPSED / 60))
SECONDS=$((ELAPSED % 60))

print_header "Screenshot Generation Complete"
print_success "Generated screenshots for ${TOTAL} level/palette combinations"
if [[ ${MINUTES} -gt 0 ]]; then
    print_info "Total time: ${MINUTES}m ${SECONDS}s"
else
    print_info "Total time: ${SECONDS}s"
fi
print_info "Screenshots saved to: ${OUTPUT_DIR}"
