#!/bin/bash
# Build standard WASM variant (balanced: Session + Registry + Theme)
# Outputs:
#   - GNU archive file (.a) for Unity 2021.2+
#   - JavaScript executable (.js + .wasm) for browser testing
set -e
mkdir -p build/wasm-standard
cd build/wasm-standard
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_BUILD=ON -DWASM_VARIANT=standard ../..
cmake --build . -- -j4
echo "✓ Standard WASM build complete:"
echo "  Archive:  build/wasm-standard/wasm/libtml-standard.a"
echo "  Browser:  build/wasm-standard/browser/tml-standard.js + tml-standard.wasm"
