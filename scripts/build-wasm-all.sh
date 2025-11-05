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
    local build_dir="build-wasm-${variant}"

    echo -e "${YELLOW}Building variant: ${variant}${NC}"
    echo "  Build directory: ${build_dir}"

    # Create build directory
    mkdir -p "${SCRIPT_DIR}/${build_dir}"
    cd "${SCRIPT_DIR}/${build_dir}"

    # Configure with CMake
    echo "  [1/3] Configuring CMake..."
    emcmake cmake \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DWASM_VARIANT="${variant}" \
        -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG" \
        .. 2>&1 | grep -E "(Building|WASM|Error|Warning)" || true

    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ CMake configuration failed${NC}"
        return 1
    fi

    # Build with Ninja/Make
    echo "  [2/3] Compiling..."
    cmake --build . --config "${BUILD_TYPE}" -- -j$(nproc) 2>&1 | tail -5

    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Build failed${NC}"
        return 1
    fi

    # Check output files
    if [ -f "wasm/textmate-${variant}.js" ] && [ -f "wasm/textmate-${variant}.wasm" ]; then
        JS_SIZE=$(stat -f%z "wasm/textmate-${variant}.js" 2>/dev/null || stat -c%s "wasm/textmate-${variant}.js")
        WASM_SIZE=$(stat -f%z "wasm/textmate-${variant}.wasm" 2>/dev/null || stat -c%s "wasm/textmate-${variant}.wasm")

        JS_SIZE_KB=$((JS_SIZE / 1024))
        WASM_SIZE_KB=$((WASM_SIZE / 1024))

        echo "  [3/3] Output files generated"
        echo -e "    ${GREEN}✓ textmate-${variant}.js${NC} (${JS_SIZE_KB} KB)"
        echo -e "    ${GREEN}✓ textmate-${variant}.wasm${NC} (${WASM_SIZE_KB} KB)"
    else
        echo -e "${RED}✗ Output files not found${NC}"
        return 1
    fi

    # Try to optimize with wasm-opt if available
    if command -v wasm-opt &> /dev/null; then
        echo "  [Bonus] Optimizing with wasm-opt..."
        WASM_OPT_OUTPUT="${SCRIPT_DIR}/${build_dir}/wasm/textmate-${variant}-opt.wasm"
        wasm-opt \
            -O4 \
            --enable-simd \
            --enable-exceptions \
            --enable-bulk-memory \
            "wasm/textmate-${variant}.wasm" \
            -o "${WASM_OPT_OUTPUT}" 2>&1

        if [ -f "${WASM_OPT_OUTPUT}" ]; then
            OPT_SIZE=$(stat -f%z "${WASM_OPT_OUTPUT}" 2>/dev/null || stat -c%s "${WASM_OPT_OUTPUT}")
            OPT_SIZE_KB=$((OPT_SIZE / 1024))
            REDUCTION=$(( (WASM_SIZE - OPT_SIZE) * 100 / WASM_SIZE ))
            echo -e "    ${GREEN}✓ Optimized output${NC} (${OPT_SIZE_KB} KB, ${REDUCTION}% reduction)"
            mv "${WASM_OPT_OUTPUT}" "wasm/textmate-${variant}.wasm"
        fi
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
for variant in "${SUCCESSFUL_VARIANTS[@]}"; do
    echo "  build-wasm-${variant}/wasm/"
done

echo ""
echo -e "${GREEN}All WASM 2023 variants built successfully!${NC}"
