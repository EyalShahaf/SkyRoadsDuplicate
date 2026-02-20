#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

# Colors and icons
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Icons
ICON_CONFIG="⚙️"
ICON_BUILD="🔨"
ICON_SUCCESS="✅"
ICON_INFO="ℹ️"
ICON_WARN="⚠️"

# Get number of CPU cores for parallel builds
if command -v nproc &> /dev/null; then
    JOBS=$(nproc)
elif command -v sysctl &> /dev/null; then
    JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo "4")
else
    JOBS=4  # fallback
fi

# Function to print colored messages
print_header() {
    echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${CYAN}$1${NC}"
    echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

print_step() {
    echo -e "\n${BOLD}${BLUE}$1${NC}"
}

print_success() {
    echo -e "${GREEN}${ICON_SUCCESS} $1${NC}"
}

print_info() {
    echo -e "${CYAN}${ICON_INFO} $1${NC}"
}

# Start timer
START_TIME=$(date +%s)

# Check if we need to reconfigure
NEED_RECONFIGURE=true
if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    CACHED_BUILD_TYPE=$(grep -E "^CMAKE_BUILD_TYPE:" "${BUILD_DIR}/CMakeCache.txt" | cut -d'=' -f2 || echo "")
    if [[ "${CACHED_BUILD_TYPE}" == "${BUILD_TYPE}" ]]; then
        NEED_RECONFIGURE=false
    fi
fi

print_header "SkyRoads Build System"

if [[ "${NEED_RECONFIGURE}" == "true" ]]; then
    print_step "${ICON_CONFIG} Configuring (${BUILD_TYPE})..."
    print_info "First run downloads raylib and can take a little while."
    
    # macOS: Set SDKROOT if available to help with raylib/GLFW builds
    if [[ "$(uname)" == "Darwin" ]]; then
        SDK_PATH=$(xcrun --show-sdk-path 2>/dev/null || echo "")
        if [[ -n "${SDK_PATH}" ]]; then
            export SDKROOT="${SDK_PATH}"
            export CMAKE_OSX_SYSROOT="${SDK_PATH}"
        fi
    fi
    
    # Configure with suppressed warnings
    cmake -S . -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DFETCHCONTENT_QUIET=OFF \
        -Wno-dev \
        2>&1 | grep -v "CMake Deprecation Warning" | grep -v "OpenGL is deprecated" | grep -v "This warning is for project developers" || true
    
    print_success "Configuration complete"
else
    print_info "Using existing configuration (${BUILD_TYPE})"
fi

print_step "${ICON_BUILD} Building with ${JOBS} parallel jobs..."

# macOS: Ensure SDKROOT is set for the build process
if [[ "$(uname)" == "Darwin" ]]; then
    SDK_PATH=$(xcrun --show-sdk-path 2>/dev/null || echo "")
    if [[ -n "${SDK_PATH}" ]]; then
        export SDKROOT="${SDK_PATH}"
    fi
fi

# Build with parallel jobs
# Use a temporary file to capture exit code since pipe creates subshell
BUILD_OUTPUT=$(mktemp)
set +e  # Temporarily disable exit on error to capture exit code
cmake --build "${BUILD_DIR}" \
    --config "${BUILD_TYPE}" \
    --parallel "${JOBS}" \
    > "${BUILD_OUTPUT}" 2>&1
BUILD_EXIT=$?
set -e  # Re-enable exit on error

# Process output with progress bar
LAST_PERCENT=0
while IFS= read -r line; do
    # Filter out unwanted warnings
    if [[ "$line" =~ (CMake Deprecation Warning|OpenGL is deprecated|This warning is for project developers) ]]; then
        continue
    fi
    
    # Parse CMake progress output and show percentage if available
    # CMake outputs percentages like [ 38%], [ 55%], etc.
    if [[ "$line" =~ \[[[:space:]]*([0-9]+)%\] ]]; then
        PERCENT="${BASH_REMATCH[1]}"
        LAST_PERCENT=${PERCENT}
        # Show progress bar
        BAR_LENGTH=30
        FILLED=$((PERCENT * BAR_LENGTH / 100))
        BAR=$(printf "%${FILLED}s" | tr ' ' '█')
        EMPTY=$(printf "%$((BAR_LENGTH - FILLED))s")
        echo -ne "\r${CYAN}[${BAR}${EMPTY}] ${PERCENT}%${NC}  "
    elif [[ "$line" =~ (Building|Linking|Built\ target) ]]; then
        # Show build steps with color
        echo -e "\r${YELLOW}→${NC} $line"
    elif [[ "$line" =~ (error:|Error:|ERROR:|failed|Failed|FAILED|ld:.*error|clang:.*error|make.*Error) ]]; then
        # Highlight actual errors in red (but not test messages)
        echo -e "${RED}$line${NC}"
    else
        # Show other output
        echo "$line"
    fi
done < "${BUILD_OUTPUT}"
# Clear progress line if we showed one
if [[ ${LAST_PERCENT} -gt 0 ]]; then
    echo -ne "\r$(printf '%*s' 80)\r"  # Clear line
fi
rm -f "${BUILD_OUTPUT}"

# Check build result
if [[ ${BUILD_EXIT} -eq 0 ]]; then
    # Calculate elapsed time
    END_TIME=$(date +%s)
    ELAPSED=$((END_TIME - START_TIME))
    MINUTES=$((ELAPSED / 60))
    SECONDS=$((ELAPSED % 60))
    
    echo -e "\n"
    print_success "Build complete: ${BUILD_DIR}"
    if [[ ${MINUTES} -gt 0 ]]; then
        print_info "Elapsed time: ${MINUTES}m ${SECONDS}s"
    else
        print_info "Elapsed time: ${SECONDS}s"
    fi
    print_info "Built with ${JOBS} parallel jobs"
else
    echo -e "\n${RED}${ICON_WARN} Build failed!${NC}"
    exit 1
fi
