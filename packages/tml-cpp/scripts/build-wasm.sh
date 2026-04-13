#!/bin/bash

# Build script for tml WASM module
# Requires Emscripten SDK to be installed and activated

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building tml WASM module${NC}"
echo ""

# Check if emscripten is available
if ! which emcc > /dev/null 2>&1; then
    echo -e "${RED}Error: Emscripten compiler (emcc) not found${NC}"
    echo "Please install and activate the Emscripten SDK:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

echo -e "${GREEN}Found Emscripten${NC}"
echo ""

# Create build directory
BUILD_DIR="build/wasm"
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Cleaning existing build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with emscripten
echo -e "${GREEN}Configuring CMake with Emscripten...${NC}"
# Use -S to specify source dir (parent), -B for build dir (current)
emcmake cmake -S ../.. -B . -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_BUILD=ON

# Build
echo -e "${GREEN}Building...${NC}"
cmake --build . -- -j4

# Check if build was successful
if [ -f "tml.js" ] && [ -f "tml.wasm" ]; then
    echo ""
    echo -e "${GREEN}Build successful!${NC}"
    echo ""
    echo "Output files:"
    echo "  - tml.js"
    echo "  - tml.wasm"
    echo ""
    echo "File sizes:"
    ls -lh tml.js tml.wasm
    echo ""
    echo -e "${GREEN}You can now use these files in your web application.${NC}"
    echo "See examples/test.html for usage example."
else
    echo -e "${RED}Build failed - output files not found${NC}"
    exit 1
fi
