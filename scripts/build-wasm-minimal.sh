#!/bin/bash
# Build minimal WASM variant (core tokenization only)
# Outputs:
#   - GNU archive file (.a) for Unity 2021.2+
#   - JavaScript executable (.js + .wasm) for browser testing
set -e
mkdir -p build/wasm-minimal
cd build/wasm-minimal
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_BUILD=ON -DWASM_VARIANT=minimal ../..
cmake --build . -- -j4
echo "✓ Minimal WASM build complete:"
echo "  Archive:  build/wasm-minimal/wasm/libtml-minimal.a"
echo "  Browser:  build/wasm-minimal/browser/tml-minimal.js + tml-minimal.wasm"
