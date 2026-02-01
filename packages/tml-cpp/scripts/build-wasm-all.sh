#!/bin/bash

# TextMateLib WASM 2023 - Multi-Variant Build Script
# Builds minimal, standard, full, and debug variants with WASM 2023 features

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VARIANTS=("minimal" "standard" "full" "debug")
BUILD_TYPE="${BUILD_TYPE:-Release}"

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}TextMateLib WASM 2023 Builder${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""

# Check for Emscripten
if ! command -v emcc &> /dev/null; then
    echo -e "${RED}Error: Emscripten not found. Please activate Emscripten SDK.${NC}"
    echo "Run: source /path/to/emsdk/emsdk_env.sh"
    exit 1
fi

EMSCRIPTEN_VERSION=$(emcc --version | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -1)
echo -e "${GREEN}✓ Emscripten ${EMSCRIPTEN_VERSION} detected${NC}"
echo ""

# Function to build a variant
build_variant() {
    local variant=$1
    local build_dir="build/wasm-${variant}"

    echo -e "${YELLOW}Building variant: ${variant}${NC}"
    echo "  Build directory: ${build_dir}"

    # Create build directory
    mkdir -p "${SCRIPT_DIR}/../${build_dir}"
    cd "${SCRIPT_DIR}/../${build_dir}"

    # Configure with CMake
    echo "  [1/3] Configuring CMake..."
    emcmake cmake \
        -DUSE_WASM_BUILD=ON \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DWASM_VARIANT="${variant}" \
        -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG" \
        ../.. 2>&1 | grep -E "(Building|WASM|Error|Warning)" || true

    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ CMake configuration failed${NC}"
        return 1
    fi

    # Build with Ninja/Make
    echo "  [2/3] Compiling..."
    cmake --build . --config "${BUILD_TYPE}" -- -j4 2>&1 | tail -5

    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Build failed${NC}"
        return 1
    fi

    # Check output files - both archive and browser builds
    ARCHIVE_PATH="${SCRIPT_DIR}/../${build_dir}/wasm/libtml-${variant}.a"
    JS_PATH="${SCRIPT_DIR}/../${build_dir}/browser/tml-${variant}.js"
    WASM_PATH="${SCRIPT_DIR}/../${build_dir}/browser/tml-${variant}.wasm"

    if [ -f "${ARCHIVE_PATH}" ]; then
        ARCHIVE_SIZE=$(stat -f%z "${ARCHIVE_PATH}" 2>/dev/null || stat -c%s "${ARCHIVE_PATH}")
        ARCHIVE_SIZE_KB=$((ARCHIVE_SIZE / 1024))
        echo "  [3a/3] Archive generated"
        echo -e "    ${GREEN}✓ libtml-${variant}.a${NC} (${ARCHIVE_SIZE_KB} KB, WebAssembly Object Files)"
    else
        echo -e "${RED}✗ Output archive file not found at ${ARCHIVE_PATH}${NC}"
        return 1
    fi

    if [ -f "${JS_PATH}" ] && [ -f "${WASM_PATH}" ]; then
        JS_SIZE=$(stat -f%z "${JS_PATH}" 2>/dev/null || stat -c%s "${JS_PATH}")
        WASM_SIZE=$(stat -f%z "${WASM_PATH}" 2>/dev/null || stat -c%s "${WASM_PATH}")
        JS_SIZE_KB=$((JS_SIZE / 1024))
        WASM_SIZE_KB=$((WASM_SIZE / 1024))
        echo "  [3b/3] Browser executables generated"
        echo -e "    ${GREEN}✓ tml-${variant}.js${NC} (${JS_SIZE_KB} KB)"
        echo -e "    ${GREEN}✓ tml-${variant}.wasm${NC} (${WASM_SIZE_KB} KB)"
    else
        echo -e "${RED}✗ Browser output files not found${NC}"
        return 1
    fi

    cd "${SCRIPT_DIR}"
    echo -e "${GREEN}✓ ${variant} variant complete${NC}"
    echo ""
    return 0
}

# Build all variants
FAILED_VARIANTS=()
SUCCESSFUL_VARIANTS=()

for variant in "${VARIANTS[@]}"; do
    if build_variant "${variant}"; then
        SUCCESSFUL_VARIANTS+=("${variant}")
    else
        FAILED_VARIANTS+=("${variant}")
    fi
done

# Summary
echo ""
echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}Build Summary${NC}"
echo -e "${BLUE}============================================${NC}"

if [ ${#SUCCESSFUL_VARIANTS[@]} -gt 0 ]; then
    echo -e "${GREEN}Successful builds:${NC}"
    for variant in "${SUCCESSFUL_VARIANTS[@]}"; do
        echo -e "  ${GREEN}✓ ${variant}${NC}"
    done
fi

if [ ${#FAILED_VARIANTS[@]} -gt 0 ]; then
    echo -e "${RED}Failed builds:${NC}"
    for variant in "${FAILED_VARIANTS[@]}"; do
        echo -e "  ${RED}✗ ${variant}${NC}"
    done
    exit 1
fi

echo ""
echo "Build outputs available at:"
echo ""
for variant in "${SUCCESSFUL_VARIANTS[@]}"; do
    echo "  ${variant}:"
    echo "    Archive:  build/wasm-${variant}/wasm/libtml-${variant}.a"
    echo "    Browser:  build/wasm-${variant}/browser/tml-${variant}.{js,wasm}"
done

echo ""
echo "Dual Output Formats:"
echo "  • GNU archive (.a) - WebAssembly Object Files for Unity 2021.2+"
echo "  • JavaScript (.js/.wasm) - Browser testing and Node.js support"
echo ""
echo -e "${GREEN}All WASM 2023 variants built successfully!${NC}"
