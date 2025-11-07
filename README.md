# TextMateLib (tml)

A modern C++ implementation of the TextMate syntax highlighting engine. TextMateLib tokenizes source code using TextMate-format grammars, applies color schemes (themes), and provides native C/C++, C FFI, and WebAssembly APIs for high-performance syntax highlighting.

## Features

- **Grammar-based syntax highlighting** for multiple languages using TextMate format
- **Stateful incremental tokenization** with line-by-line caching for editor integration
- **Theme/color scheme support** with scope-based color mapping
- **Regex pattern matching** via Oniguruma engine
- **High-performance design** with early stopping and state comparison optimizations
- **Multiple API layers**: Native C++, C FFI, and JavaScript/WASM
- **WebAssembly 2023 support**: SIMD, exceptions, bulk memory, BigInt
- **Dual WASM output**: Static archives for Unity 2021.2+ and JavaScript executables for browser testing
- **Cross-platform**: Linux, macOS, Windows, WebAssembly, and Unity WebGL

## Quick Start

### Prerequisites

- **C++17** compatible compiler (GCC, Clang, or MSVC)
- **CMake 3.15+**
- **Git** (for submodules)
- For WebAssembly: **Emscripten SDK (emsdk)** activated in your environment

### Native Build

```bash
# Clone the repository
git clone https://github.com/your-org/TextMateLib.git
cd TextMateLib

# Build the library (Release)
# Dependencies are built automatically by CMake
./scripts/build.sh
```

**Output:**
- Library: `build/lib/libtm.so` (Linux/macOS) or `.dll` (Windows)
- Headers: `build/include/tml/`

### WebAssembly Build

```bash
# Activate Emscripten toolchain
source /path/to/emsdk/emsdk_env.sh

# Build standard variant (recommended)
./scripts/build-wasm-standard.sh

# Or build all variants
./scripts/build-wasm-all.sh
```

**Output (for each variant):**
- **Archive** (for Unity): `build/wasm-{variant}/wasm/libtml-{variant}.a`
- **Browser**: `build/wasm-{variant}/browser/tml-{variant}.js` + `.wasm`

See [Building for WebAssembly](#building-for-webassembly) for detailed instructions.

### Running Tests

```bash
# Run all tests
cd build && ctest -V

# Run specific test suite
make test_first_mate
make test_session
make test_syntax_highlighter
make test_theme

# Run performance benchmarks
make benchmark_large
make benchmark_session
```

## Architecture

### High-Level Design

```
┌─────────────────────────────────────────────────┐
│  Public APIs (C++, C, JavaScript/WASM)         │
├─────────────────────────────────────────────────┤
│  Session API        │  Syntax Highlighter      │
│  (Stateful)         │  (Convenience Wrapper)   │
├─────────────────────────────────────────────────┤
│  Grammar (Tokenization Logic)                   │
│  - Rule matching, state transitions             │
│  - Scope stack management                       │
├─────────────────────────────────────────────────┤
│  Registry (Grammar/Theme Management)            │
│  - Grammar lookup and caching                   │
│  - Theme application                           │
├─────────────────────────────────────────────────┤
│  Core Components                                │
│  - Regex engine (Oniguruma)                    │
│  - JSON parser (RapidJSON)                     │
│  - Scope/attribute providers                   │
└─────────────────────────────────────────────────┘
```

### Key Modules

**Grammar Processing Pipeline:**
- `src/parseRawGrammar.h/cpp` - Parse JSON grammar definitions (TextMate format)
- `src/rawGrammar.h/cpp` - Raw grammar data structures
- `src/grammar.h/cpp` - Compiled grammar representation with rule matching
- `src/rule.h/cpp` - Individual grammar rules (pattern, begin/end, captures, etc.)
- `src/matcher.h/cpp` - Pattern matching utilities and caching
- `src/onigLib.h/cpp` - Oniguruma regex engine wrapper

**Tokenization Pipeline:**
- `src/session.h/cpp` - High-level editor session with line caching
- `src/tokenizeString.h/cpp` - Core tokenization logic, state transitions
- `src/encodedTokenAttributes.h/cpp` - Token representation and encoding
- `src/syntax_highlighter.h/cpp` - Convenience wrapper combining grammar + theme

**Theme and Registry:**
- `src/registry.h/cpp` - Central grammar/theme manager and lookup
- `src/theme.h/cpp` - Theme parsing and color/style application
- `src/basicScopesAttributeProvider.h/cpp` - Scope attribute handling

**APIs:**
- `src/main.h` - Public C++ API (aggregates all types)
- `src/c_api.h/cpp` - C FFI API for language bindings
- `src/session_c_api.h`, `src/theme_c_api.h`, `src/syntax_highlighter_c_api.h` - C API modules

**WebAssembly Bindings (src/wasm/):**
- `src/wasm/bindings.cpp` - Emscripten JavaScript bindings
- `src/wasm/simd_bindings.h/cpp` - SIMD optimization support
- `src/wasm/bulk_memory_bindings.h/cpp` - Bulk memory operations
- `src/wasm/exception_bindings.h/cpp` - Exception handling
- `src/wasm/bigint_bindings.h` - BigInt support

### Key Architectural Concepts

**StateStack**: An immutable stack representing the parsing state at the end of a line. Enables resuming on the next line. Two StateStacks are equal if parsing can resume from the same grammar rules.

**RuleId**: An opaque integer identifier for grammar rules, used to track which rules matched.

**ScopeStack**: Nested scopes during tokenization (e.g., `source.js > string.quoted.double > constant.character.escape.js`), which maps to theme colors.

**Incremental Tokenization**: The Session API caches tokens per line and skips re-parsing when StateStack is unchanged (early stopping).

**Injection Grammars**: Support for embedding one grammar within another (e.g., regex patterns within strings), managed via grammar includes.

**Balanced Brackets**: A special rule type for context-aware bracket matching.

## Usage Examples

### Basic Tokenization (C++)

```cpp
#include <tml/main.h>

using namespace tml;

int main() {
    // Create registry and load grammars
    auto registry = std::make_shared<Registry>();
    registry->loadGrammarFromFile("path/to/javascript.json");
    registry->loadThemeFromFile("path/to/theme.json");

    // Get grammar
    auto grammar = registry->grammarForScopeName("source.js");

    // Tokenize a line
    std::string code = "const x = 42;";
    auto tokens = grammar->tokenizeString(code);

    // Process tokens
    for (const auto& token : tokens) {
        std::cout << "Token: " << token.value
                  << " (scope: " << token.scope << ")" << std::endl;
    }

    return 0;
}
```

### Incremental Tokenization with Session API (C++)

```cpp
#include <tml/main.h>

using namespace tml;

int main() {
    auto registry = std::make_shared<Registry>();
    registry->loadGrammarFromFile("path/to/javascript.json");

    auto grammar = registry->grammarForScopeName("source.js");
    auto session = std::make_shared<Session>(grammar);

    // Tokenize multiple lines incrementally
    std::vector<std::string> lines = {
        "function hello() {",
        "    console.log('world');",
        "}"
    };

    for (size_t i = 0; i < lines.size(); ++i) {
        auto tokens = session->tokenizeLine(lines[i]);
        std::cout << "Line " << i << ": " << tokens.size() << " tokens" << std::endl;
    }

    return 0;
}
```

### Using the Syntax Highlighter (C++)

```cpp
#include <tml/main.h>

using namespace tml;

int main() {
    auto registry = std::make_shared<Registry>();
    registry->loadGrammarFromFile("path/to/javascript.json");
    registry->loadThemeFromFile("path/to/theme.json");

    SyntaxHighlighter highlighter(registry, "source.js");

    std::string code = "const x = 42;";
    auto highlighted = highlighter.highlight(code);

    // highlighted now contains tokens with color/style information
    for (const auto& token : highlighted) {
        std::cout << token.value << " -> " << token.foreground << std::endl;
    }

    return 0;
}
```

### WebAssembly / JavaScript

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>TextMateLib WASM Demo</title>
</head>
<body>
    <h1>Syntax Highlighting with TextMateLib</h1>
    <script async src="tml-standard.js"></script>
    <script>
        Module.onRuntimeInitialized = async () => {
            console.log('TextMateLib WASM loaded!');

            // Use the C++ API through JavaScript bindings
            // The exact API depends on the bindings exported in src/wasm/bindings.cpp
            // Example (bindings may vary):
            // const tokens = Module.tokenizeString(code, grammarName);

            // See src/wasm/bindings.cpp for exported functions
        };
    </script>
</body>
</html>
```

## Directory Structure

```
.
├── src/                          # Source code
│   ├── Core API layer
│   │   ├── main.h               # Public C++ API
│   │   ├── c_api.h/cpp          # C FFI API
│   │   └── session.h/cpp        # Stateful editor session
│   │
│   ├── Grammar processing
│   │   ├── parseRawGrammar.h/cpp
│   │   ├── rawGrammar.h/cpp
│   │   ├── grammar.h/cpp
│   │   └── rule.h/cpp
│   │
│   ├── Registry & Theme
│   │   ├── registry.h/cpp
│   │   ├── theme.h/cpp
│   │   └── basicScopesAttributeProvider.h/cpp
│   │
│   ├── Tokenization
│   │   ├── tokenizeString.h/cpp
│   │   ├── encodedTokenAttributes.h/cpp
│   │   └── syntax_highlighter.h/cpp
│   │
│   ├── Regex & Utilities
│   │   ├── onigLib.h/cpp
│   │   ├── matcher.h/cpp
│   │   ├── utils.h/cpp
│   │   └── types.h
│   │
│   └── wasm/                     # WebAssembly bindings
│       ├── bindings.cpp
│       ├── simd_bindings.h/cpp
│       ├── bulk_memory_bindings.h/cpp
│       ├── exception_bindings.h/cpp
│       └── bigint_bindings.h
│
├── tests/                        # Test suites
│   ├── test_first_mate/         # Core tokenization tests
│   ├── test_session/            # Session API tests
│   ├── test_syntax_highlighter/ # Highlighter tests
│   ├── test_theme/              # Theme tests
│   ├── benchmark_large/         # Large file benchmarks
│   ├── benchmark_session/       # Session performance
│   └── CMakeLists.txt
│
├── scripts/                      # Build scripts
│   ├── build.sh                 # Native Release build
│   ├── build-wasm-standard.sh   # Standard WASM variant
│   ├── build-wasm-minimal.sh    # Minimal WASM variant
│   ├── build-wasm-full.sh       # Full-featured WASM variant
│   ├── build-wasm-debug.sh      # Debug WASM variant
│   └── build-wasm-all.sh        # Build all variants
│
├── thirdparty/                   # External dependencies
│   ├── oniguruma/               # Regex engine
│   └── rapidjson/               # JSON parser & GTest
│
├── CMakeLists.txt               # Main build configuration
└── README.md                    # This file
```

## Building for WebAssembly

### Overview

The WASM build system produces **dual outputs** for each variant - one for Unity integration and one for browser testing.

### Variants

| Variant | Features | Use Case |
|---------|----------|----------|
| **minimal** | Core tokenization only | Lightweight deployments |
| **standard** | Session + Registry + Theme | General purpose (recommended) |
| **full** | SIMD + Exceptions + Bulk Memory + BigInt | Performance-critical applications |
| **debug** | Debug symbols + assertions | Development & debugging |

### Quick Build

```bash
# Activate Emscripten
source /path/to/emsdk/emsdk_env.sh

# Build all variants with dual outputs
./scripts/build-wasm-all.sh

# Or build a specific variant
./scripts/build-wasm-standard.sh
./scripts/build-wasm-full.sh
./scripts/build-wasm-minimal.sh
./scripts/build-wasm-debug.sh
```

### Output Formats

Each variant produces two outputs:

#### 1. Archive (for Unity 2021.2+ WebGL plugins)
- **Location**: `build/wasm-{variant}/wasm/libtml-{variant}.a`
- **Format**: GNU ar archive containing WebAssembly Object Files (.o)
- **Size**: ~1.2 MB per variant
- **Benefits**: Faster compilation in Unity, standard format
- **Usage**: Copy to `Assets/Plugins/WebGL/` in Unity project

#### 2. Browser Executable (for web/Node.js testing)
- **Location**: `build/wasm-{variant}/browser/tml-{variant}.js` + `.wasm`
- **JS file**: Emscripten JavaScript runtime + bindings (~48 KB)
- **WASM file**: Compiled WebAssembly binary (~807 KB)
- **Usage**: Test in browsers or Node.js before Unity deployment

### Testing in Browser

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>TextMateLib WASM Test</title>
</head>
<body>
    <h1>TextMateLib WASM Test</h1>
    <script async src="tml-standard.js"></script>
    <script>
        Module.onRuntimeInitialized = async () => {
            console.log('TextMateLib loaded!');
            // Use exported functions here
        };
    </script>
</body>
</html>
```

### Manual WASM Build

```bash
# Activate Emscripten
source /path/to/emsdk/emsdk_env.sh

# Create build directory
mkdir -p build/wasm-standard
cd build/wasm-standard

# Configure with Emscripten
emcmake cmake -S ../.. -B . \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_WASM_BUILD=ON \
    -DWASM_VARIANT=standard

# Build
cmake --build .
```

### WebAssembly 2023 Features

All WASM builds automatically include WebAssembly 2023 mandatory features:

| Feature | Purpose | Status |
|---------|---------|--------|
| SIMD128 | Vectorized operations | ✓ Enabled |
| Exception Handling | WebAssembly exceptions | ✓ Enabled |
| Bulk Memory | Efficient data movement | ✓ Enabled |
| BigInt | Large integer support | ✓ Enabled |
| Non-trapping float-to-int | Safe conversions | ✓ Enabled |

These are defined in `CMakeLists.txt` and automatically applied when `USE_WASM_BUILD=ON`.

## Testing

### Running Tests

```bash
# Run all tests with verbose output
cd build && ctest -V

# Run specific test suite
make test_first_mate
make test_session
make test_syntax_highlighter
make test_theme
make test_theme_simple
make test_theme_file
make test_theme_debug

# Run with gtest filter
cd build/tests/test_first_mate
./test_first_mate --gtest_filter="FirstMateTests.SomeTest"

# List all available tests
cd build/tests/test_first_mate
./test_first_mate --gtest_list_tests
```

### Performance Benchmarks

```bash
# Run performance benchmarks
make benchmark_large              # Large file tokenization
make benchmark_session            # Session performance
make benchmark_session_comparison # Comparative benchmarking

# Run benchmark with custom settings
cd build/tests/benchmark_large
./benchmark_large
```

### Test Organization

Tests are organized in individual subdirectories under `tests/` with:
- Source files (`.cpp`)
- Test fixtures and data (`.json`, grammar files)
- Working directory structure for relative path access

**Important**: Tests run from their own subdirectory (e.g., `build/tests/test_first_mate/`), so fixtures and data files must be located relative to that directory.

## CMake Configuration

### Build Options

```bash
# Configure with options
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Option reference:
# CMAKE_BUILD_TYPE      - Release (default) or Debug
# USE_WASM_BUILD        - ON/OFF for WebAssembly compilation
# WASM_VARIANT          - minimal, standard, full, debug (WASM only)
```

### Compiler Requirements

- **C++17** standard
- GCC/Clang on Unix-like systems
- MSVC on Windows
- Emscripten for WebAssembly

## Platform Support

- **Linux** - Full support (GCC/Clang)
- **macOS** - Full support (Clang)
- **Windows** - Full support (MSVC)
- **WebAssembly** - Emscripten with WASM 2023 features
- **Unity 2021.2+** - Via static archive plugins
- **Node.js** - Via WASM binding
- **Browsers** - Via WASM binding with JavaScript interop

## Dependencies

All dependencies are included as Git submodules and built automatically by CMake:

| Library | Purpose | Location |
|---------|---------|----------|
| Oniguruma | Regex engine for pattern matching | `thirdparty/oniguruma/` |
| RapidJSON | JSON parsing and GTest | `thirdparty/rapidjson/` |

Dependencies are built as part of the standard build process and linked statically into the library.

## Common Development Tasks

### Adding Support for a New Language

1. Obtain a TextMate grammar file (JSON format)
2. Load it:
   ```cpp
   auto registry = std::make_shared<Registry>();
   registry->loadGrammarFromFile("path/to/grammar.json");
   ```
3. Register and associate with file extensions:
   ```cpp
   registry->addGrammar(grammar, "source.js", {".js"});
   ```
4. Test tokenization:
   ```cpp
   auto grammar = registry->grammarForScopeName("source.js");
   auto tokens = grammar->tokenizeString(code);
   ```

### Modifying Grammar Rules or Tokenization

1. Grammar/rule matching: `src/grammar.h/cpp` or `src/rule.h/cpp`
2. Tokenization state: `src/tokenizeString.h/cpp`
3. Regex/pattern matching: `src/onigLib.h/cpp` or `src/matcher.h/cpp`
4. Test changes:
   ```bash
   make test_first_mate
   ```

### Adding Theme or Color Scheme Support

1. Modify theme parsing: `src/theme.h/cpp`
2. Update scope-to-color mapping: `src/basicScopesAttributeProvider.h/cpp`
3. Update C API if needed: `src/theme_c_api.h`
4. Test:
   ```bash
   make test_theme
   ```

## Debugging and Performance

### WebAssembly Debugging

```bash
# Build debug variant with symbols and assertions
./scripts/build-wasm-debug.sh

# Test in browser using debug build
# Use Chrome DevTools Performance tab for profiling
```

### Performance Optimization

```bash
# Identify bottlenecks
make benchmark_large

# Key optimization points:
# - StateStack equality checks (incremental tokenization)
# - Session line caching
# - Early stopping for large files
# - Full variant with SIMD for performance-critical code
```

## Git Workflow

**Current branch**: `develop`
**Main branch**: `main`

All feature work should:
1. Branch from `develop`
2. Merge back to `develop` for testing
3. Then merge to `main` for release

## Contributing

To contribute to TextMateLib:

1. Ensure code follows the existing style and architecture
2. Add tests for new features in appropriate test suite
3. Run all tests and benchmarks before submitting
4. Update documentation if adding new APIs or features
5. Follow the Git workflow above

## License

See LICENSE file for project licensing.

## Resources

- **TextMate Documentation**: https://macromates.com/manual/en/
- **TextMate Grammars**: https://github.com/textmate/grammars
- **Oniguruma**: https://github.com/kkos/oniguruma
- **WebAssembly**: https://webassembly.org/

## Support

For issues, questions, or contributions, please open an issue on the project repository.
