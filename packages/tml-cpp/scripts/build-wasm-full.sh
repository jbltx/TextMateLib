#!/bin/bash
# Build full WASM variant (SIMD + Exceptions + Bulk Memory + BigInt)
# Outputs:
#   - GNU archive file (.a) for Unity 2021.2+
#   - JavaScript executable (.js + .wasm) for browser testing
set -e
mkdir -p build/wasm-full
cd build/wasm-full
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_BUILD=ON -DWASM_VARIANT=full ../..
cmake --build . -- -j4
echo "✓ Full WASM build complete:"
echo "  Archive:  build/wasm-full/wasm/libtml-full.a"
echo "  Browser:  build/wasm-full/browser/tml-full.js + tml-full.wasm"
