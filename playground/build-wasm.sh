#!/bin/bash
# Build WASM module and copy to playground

set -e

echo "Building TextMateLib WASM module for playground..."

# Check if emsdk is available
if ! command -v emcc &> /dev/null; then
    echo "Error: Emscripten not found!"
    echo "Please install and activate Emscripten SDK:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

# Navigate to project root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR/.."

# Build WASM standard variant
echo "Building WASM standard variant..."
./scripts/build-wasm-standard.sh

# Create playground wasm directory if it doesn't exist
mkdir -p playground/public/wasm

# Copy WASM files to playground
echo "Copying WASM files to playground..."
cp build/wasm-standard/browser/tml-standard.js playground/public/wasm/
cp build/wasm-standard/browser/tml-standard.wasm playground/public/wasm/

echo ""
echo "✅ WASM module built and copied successfully!"
echo ""
echo "To run the playground:"
echo "  npm run dev"

