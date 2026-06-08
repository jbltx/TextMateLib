# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**TextMateLib (tml)** is a modern C++ implementation of the TextMate syntax highlighting engine. It's a production-grade library that tokenizes source code using TextMate-format grammars and applies color schemes (themes) for syntax highlighting.

- **Type**: C++ Library with WebAssembly bindings
- **Primary Language**: C++17
- **Key APIs**: C++ native, C FFI, JavaScript/WASM
- **Platforms**: Linux, macOS, Windows, WebAssembly, Unity WebGL
- **Status**: Production-ready with comprehensive testing and benchmarking

## Commonly Used Commands

### Building

```bash
# Native Release build (C++)
./scripts/build.sh

# WebAssembly builds (requires activated Emscripten: source /path/to/emsdk/emsdk_env.sh)
./scripts/build-wasm-standard.sh    # Balanced variant (recommended)
./scripts/build-wasm-all.sh         # All variants
./scripts/build-wasm-full.sh        # Feature-rich with SIMD/exceptions
./scripts/build-wasm-minimal.sh     # Lightweight core only
./scripts/build-wasm-debug.sh       # Debug with symbols
```

### Testing

```bash
# Run all tests with verbose output
cd build && ctest -V

# Run specific test suites
make test_first_mate              # Core tokenization tests
make test_session                 # Session API tests
make test_syntax_highlighter      # Highlighter tests
make test_theme                   # Theme tests

# Run performance benchmarks
make benchmark_large              # Large file tokenization
make benchmark_session            # Session performance
make benchmark_session_comparison # Comparative analysis

# Run single test with GTest filtering
cd build/tests/test_first_mate
./test_first_mate --gtest_filter="FirstMateTests.SomeTest"
./test_first_mate --gtest_list_tests
```

### Playground Development

```bash
# Build WASM for playground
cd playground
./build-wasm.sh

# Serve playground locally (required for WASM)
./serve.sh
# Then open http://localhost:8000
```

## High-Level Architecture

### Layered Design

```
Presentation Layer
    ↓
Public APIs (C++, C FFI, JavaScript/WASM)
    ↓
Session API (Stateful) + Syntax Highlighter (Convenience)
    ↓
Grammar Engine (Tokenization Logic)
    ↓
Registry (Grammar/Theme Manager)
    ↓
Oniguruma (Regex) + RapidJSON (JSON)
```

### Key Components

**Grammar Processing** (`src/grammar*.h/cpp`, `src/rule*.h/cpp`, `src/parseRawGrammar.h/cpp`):
- Parses TextMate JSON grammars into compiled rule structures
- Manages pattern matching and scope stack during tokenization
- Supports injection grammars for embedding one grammar in another

**Tokenization Pipeline** (`src/tokenizeString.h/cpp`, `src/session.h/cpp`, `src/syntax_highlighter.h/cpp`):
- `tokenizeString`: Core line-by-line tokenization with state transitions
- `session`: Stateful editor API with per-line caching and early-stopping optimization
- `syntax_highlighter`: Convenience wrapper combining grammar + theme

**Registry & Theme** (`src/registry.h/cpp`, `src/theme.h/cpp`):
- Central manager for grammar and theme lookup with lazy loading and caching
- Theme parsing maps scopes to colors and styles
- `basicScopesAttributeProvider`: Scope-to-attribute mapping

**Infrastructure** (`src/onigLib.h/cpp`, `src/matcher.h/cpp`, `src/types.h`):
- Oniguruma regex engine wrapper for pattern matching
- Matcher utilities with caching
- Core type definitions

**Public APIs**:
- `src/main.h`: Primary C++ API (aggregates all exports)
- `src/c_api.h/cpp`: C FFI for language bindings
- `src/wasm/bindings.cpp`: JavaScript/WASM interop (Emscripten)

### Critical Architectural Concepts

**StateStack**: Immutable stack representing parsing state at end of a line. Two equal StateStacks mean parsing can resume from the same rules without re-scanning.

**RuleId**: Opaque integer identifier for grammar rules, tracks which rules matched during tokenization.

**ScopeStack**: Nested scope paths (e.g., `source.js > string.quoted.double > constant.character.escape.js`), directly maps to theme colors.

**Incremental Tokenization**: Session API caches tokens per line and compares StateStack between lines. When state is unchanged, re-parsing is skipped (critical performance optimization).

**Injection Grammars**: Support embedding one grammar within another (regex in strings, code in Markdown, etc.).

**Balanced Brackets**: Special rule type for context-aware bracket matching.

## Directory Structure

**Core Library**:
- `src/` - All C++ source code
  - Grammar processing: `grammar.h/cpp`, `rule.h/cpp`, `parseRawGrammar.h/cpp`, `matcher.h/cpp`
  - Tokenization: `tokenizeString.h/cpp`, `session.h/cpp`, `syntax_highlighter.h/cpp`, `encodedTokenAttributes.h/cpp`
  - Registry & Theme: `registry.h/cpp`, `theme.h/cpp`, `basicScopesAttributeProvider.h/cpp`
  - Infrastructure: `onigLib.h/cpp`, `types.h`, `utils.h/cpp`
  - APIs: `main.h`, `c_api.h/cpp`, `session_c_api.h`, `theme_c_api.h`, `syntax_highlighter_c_api.h`
  - WebAssembly: `wasm/bindings.cpp`, `wasm/simd_bindings.*`, `wasm/bulk_memory_bindings.*`, `wasm/exception_bindings.*`, `wasm/bigint_bindings.h`

**Tests** (`tests/`):
- `test_first_mate/` - Tokenization logic and rule matching
- `test_session/` - Session API and line caching
- `test_syntax_highlighter/` - Highlighter convenience wrapper
- `test_theme/` - Theme parsing and application
- `test_theme_simple/`, `test_theme_file/`, `test_theme_debug/` - Theme-specific tests
- `benchmark_large/` - Large file tokenization performance
- `benchmark_session/` - Session incremental tokenization performance
- `benchmark_session_comparison/` - Comparative benchmarking

**Build & Scripts**:
- `scripts/build.sh` - Native release build
- `scripts/build-wasm-*.sh` - WebAssembly variants
- `CMakeLists.txt` - Main build configuration

**Playground**:
- `playground/` - Interactive web-based demo
  - `index.html`, `app.js`, `styles.css` - UI and application logic
  - `grammars.js`, `themes.js` - 30+ grammars and 40+ themes
  - `build-wasm.sh`, `serve.sh` - Build and serve scripts

**Dependencies** (`thirdparty/`):
- `oniguruma/` - Regex engine (built as CMake ExternalProject)
- `rapidjson/` - JSON parser and GTest framework
- `textmate-grammars-themes/` - Grammar and theme collection (Git submodule)

## Technology Stack

**Languages & Standards**:
- C++17 (core library)
- JavaScript/Emscripten (WASM bindings)
- C FFI (language bindings)

**Build Tools**:
- CMake 3.14+ (main build orchestration)
- Emscripten SDK (WebAssembly compilation)
- GCC/Clang (Unix-like systems)
- MSVC (Windows)

**Dependencies**:
- Oniguruma (regex pattern matching)
- RapidJSON (JSON parsing)
- Google Test (GTest framework)

**WASM Features** (automatically enabled):
- SIMD128, Exception Handling, Bulk Memory, BigInt, Non-trapping float-to-int

## Important Implementation Patterns

1. **Reference Counting**: Session objects use reference counting for memory management
2. **Lazy Loading**: Grammars and themes are lazy-loaded and cached in Registry
3. **State Comparison**: Session API compares StateStack for early stopping optimization
4. **Scope Stacking**: Nested scope management during tokenization for proper styling
5. **ExternalProject**: Oniguruma is built as CMake ExternalProject (not pre-built)
6. **Static Linking**: All dependencies linked statically (`BUILD_SHARED_LIBS=OFF`)
7. **Dual WASM Output**: CMake produces both `.a` archive (Unity) and `.js`+`.wasm` (Browser)

## Git Workflow

- **Main branch**: `main` (stable releases)
- **Development branch**: `develop` (feature integration)
- **Current branch**: `copilot/add-playground-mini-website-again`
- Feature branches should be based on `develop`, then merged back to `develop` before `main`

## Key Files to Understand First

1. `README.md` - Comprehensive project overview with usage examples
2. `CMakeLists.txt` - Build configuration and dependency management
3. `src/main.h` - Public C++ API showing all major types and classes
4. `src/session.h` - Stateful tokenization API (critical for editor integration)
5. `src/grammar.h` - Grammar engine and rule matching
6. `src/registry.h` - Grammar/theme manager
7. `src/theme.h` - Theme parsing and color application
8. `.github/workflows/build-and-test.yml` - CI/CD configuration

## Common Development Scenarios

**Adding a new grammar rule type**: Modify `src/rule.h/cpp` and update matching logic in `src/grammar.h/cpp`. Add tests to `tests/test_first_mate/`.

**Improving tokenization performance**: Likely involves StateStack comparison or early-stopping logic in `src/session.h/cpp`. Profile with `benchmark_session` and `benchmark_large`.

**Fixing theme/color issues**: Check `src/theme.h/cpp` for parsing and `src/basicScopesAttributeProvider.h/cpp` for scope-to-attribute mapping.

**WebAssembly-specific changes**: Modify `src/wasm/bindings.cpp` for JavaScript interop. Test with playground or browser testing.

**WASM variant differences**: Check CMakeLists.txt for compile flags and `src/wasm/*.h` conditional includes based on `WASM_VARIANT`.
