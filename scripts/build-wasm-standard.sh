#!/bin/bash
# Build standard WASM variant (balanced: Session + Registry + Theme)
set -e
mkdir -p build/wasm-standard
cd build/wasm-standard
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_BUILD=ON -DWASM_VARIANT=standard ../..
cmake --build . -- -j4
echo "✓ Standard WASM build complete: build/wasm-standard/tml-standard.{js,wasm}"
