#!/bin/bash
# Build minimal WASM variant (core tokenization only)
set -e
mkdir -p build-wasm-minimal
cd build-wasm-minimal
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DWASM_VARIANT=minimal ..
cmake --build . -- -j$(nproc)
echo "✓ Minimal WASM build complete: wasm/textmate-minimal.{js,wasm}"
