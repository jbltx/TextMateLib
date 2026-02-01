# tml-cpp

The C++ core library for TextMateLib, providing syntax highlighting using TextMate grammars.

## Overview

This package contains:
- C++ source code for the TextMate parser
- WASM bindings for WebAssembly builds
- Build scripts for native and WASM compilation
- C++ unit tests and benchmarks

## Building

### Native Build

```bash
./scripts/build.sh
```

Output: `build/libtml.a` (or `.so`/`.dll` for shared builds)

### WebAssembly Build

```bash
# Ensure Emscripten is activated
source /path/to/emsdk/emsdk_env.sh

# Build standard variant
./scripts/build-wasm-standard.sh

# Build all variants
./scripts/build-wasm-all.sh
```

## Testing

```bash
cd build
ctest -V
```

## Documentation

Generate Doxygen documentation:

```bash
cd docs
doxygen Doxyfile
```

Output: `docs/html/`
