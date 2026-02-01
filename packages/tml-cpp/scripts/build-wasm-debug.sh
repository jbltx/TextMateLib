#!/bin/bash
# Build debug WASM variant (with debug symbols and runtime checks)
# Outputs:
#   - GNU archive file (.a) for Unity 2021.2+
#   - JavaScript executable (.js + .wasm) for browser testing
set -e
mkdir -p build/wasm-debug
cd build/wasm-debug
emcmake cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_WASM_BUILD=ON -DWASM_VARIANT=debug ../..
cmake --build . -- -j4
echo "✓ Debug WASM build complete:"
echo "  Archive:  build/wasm-debug/wasm/libtml-debug.a"
echo "  Browser:  build/wasm-debug/browser/tml-debug.js + tml-debug.wasm"
