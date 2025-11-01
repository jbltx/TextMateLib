#!/bin/bash
# Build debug WASM variant (with debug symbols and runtime checks)
set -e
mkdir -p build-wasm-debug
cd build-wasm-debug
emcmake cmake -DCMAKE_BUILD_TYPE=Debug -DWASM_VARIANT=debug ..
cmake --build . -- -j$(nproc)
echo "✓ Debug WASM build complete: wasm/textmate-debug.{js,wasm}"
