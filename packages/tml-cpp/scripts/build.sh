#!/bin/bash

# Build script for tml
# Requires CMake to be installed

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building tml${NC}"
echo ""

# Create build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Cleaning existing build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with emscripten
echo -e "${GREEN}Configuring CMake...${NC}"
# Use -S to specify source dir (parent), -B for build dir (current)
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF

# Build
echo -e "${GREEN}Building...${NC}"
cmake --build . -- -j4

