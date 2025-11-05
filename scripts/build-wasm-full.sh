#!/bin/bash
# Build full WASM variant (SIMD + Exceptions + Bulk Memory + BigInt)
set -e
mkdir -p build/wasm-full
cd build/wasm-full
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DWASM_VARIANT=full ../..
cmake --build . -- -j4
echo "✓ Full WASM build complete: build/wasm-full/tml-full.{js,wasm}"
