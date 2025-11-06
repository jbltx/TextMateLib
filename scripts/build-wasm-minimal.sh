#!/bin/bash
# Build minimal WASM variant (core tokenization only)
set -e
mkdir -p build/wasm-minimal
cd build/wasm-minimal
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_BUILD=ON -DWASM_VARIANT=minimal ../..
cmake --build . -- -j4
echo "✓ Minimal WASM build complete: build/wasm-minimal/tml-minimal.{js,wasm}"
