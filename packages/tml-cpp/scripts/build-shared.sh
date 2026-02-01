#!/bin/bash

# Build script for tml shared library for C# bindings

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building tml shared library for C# bindings${NC}"
echo ""

# Create build directory
BUILD_DIR="build-shared"
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Cleaning existing build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with shared library
echo -e "${GREEN}Configuring CMake for shared library...${NC}"
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON

# Build
echo -e "${GREEN}Building shared library...${NC}"
cmake --build . -- -j4

echo -e "${GREEN}Shared library build complete!${NC}"
echo ""
echo "Output:"
if [ -f "libtml.so" ]; then
    echo "  libtml.so"
elif [ -f "libtml.dylib" ]; then
    echo "  libtml.dylib"
elif [ -f "tml.dll" ]; then
    echo "  tml.dll"
fi
