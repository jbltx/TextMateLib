#!/bin/bash
# Test WASM build with WASM 2023 features
set -e

source ~/dev/emsdk/emsdk_env.sh >/dev/null 2>&1

cd /Users/mickaelbonfill/dev/vscode-textmate/textmate-cpp

echo "Building WASM standard variant with WASM 2023 features..."
rm -rf build-wasm-test
mkdir -p build-wasm-test
cd build-wasm-test

emcmake cmake -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_BUILD=ON -DWASM_VARIANT=standard ..

echo ""
echo "Build configuration complete. Now building..."
echo ""

cmake --build . --verbose 2>&1 | tail -100
