# TextMateLib (tml)

A modern C++ implementation of the TextMate syntax highlighting engine. TextMateLib tokenizes source code using TextMate-format grammars, applies color schemes (themes), and provides native C/C++, C FFI, and WebAssembly APIs for high-performance syntax highlighting.

## Try the Playground!

Want to see TextMateLib in action? Check out our **[interactive playground](packages/playground/)** with:
- 40+ themes (Dark+, Monokai, Dracula, Tokyo Night, and more)
- 30+ language grammars (JavaScript, Python, Rust, C++, and more)
- Debug view showing detailed tokenization
- Real-time syntax highlighting

[**Open the Playground**](https://tml.jbltx.com)

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
- **Interactive playground**: Web-based demo with extensive grammars and themes

## Monorepo Structure

This project uses npm workspaces to manage multiple packages:

```
TextMateLib/
├── packages/
│   ├── tml-cpp/               # C++/WASM core library
│   │   ├── src/               # C++ sources
│   │   │   └── wasm/          # WASM bindings
│   │   ├── tests/             # C++ tests
│   │   ├── scripts/           # Build scripts
│   │   ├── docs/              # Documentation (Doxygen)
│   │   ├── CMakeLists.txt
│   │   └── conanfile.py
│   │
│   ├── tml-cs/                # C# bindings
│   │   ├── src/TextMateLib.Bindings/
│   │   └── tests/TextMateLib.Tests/
│   │
│   ├── tml-js/                # JS/TS package
│   │   ├── src/
│   │   ├── tests/
│   │   └── package.json
│   │
│   └── playground/            # Web playground
│       ├── src/
│       ├── public/
│       └── package.json
│
├── thirdparty/                # Shared dependencies
├── .github/                   # CI/CD workflows
├── .changeset/                # Changesets for versioning
├── package.json               # Monorepo root
└── README.md
```

## Quick Start

### Prerequisites

- **C++17** compatible compiler (GCC, Clang, or MSVC)
- **CMake 3.15+**
- **Git** (for submodules)
- **Node.js 20+** (for JS package and playground)
- For WebAssembly: **Emscripten SDK (emsdk)** activated in your environment

### Install Dependencies

```bash
# Clone the repository
git clone https://github.com/jbltx/TextMateLib.git
cd TextMateLib

# Install npm dependencies (for JS package and playground)
npm install
```

### Native Build

```bash
# Build the C++ library
npm run build:native

# Or directly:
cd packages/tml-cpp && ./scripts/build.sh
```

**Output:**
- Library: `packages/tml-cpp/build/lib/libtm.so` (Linux/macOS) or `.dll` (Windows)
- Headers: `packages/tml-cpp/build/include/tml/`

### WebAssembly Build

```bash
# Activate Emscripten toolchain
source /path/to/emsdk/emsdk_env.sh

# Build standard variant
npm run build:wasm

# Or build all variants
npm run build:wasm:all
```

**Output (for each variant):**
- **Archive** (for Unity): `packages/tml-cpp/build/wasm-{variant}/wasm/libtml-{variant}.a`
- **Browser**: `packages/tml-cpp/build/wasm-{variant}/browser/tml-{variant}.js` + `.wasm`

### JavaScript Package

```bash
# Build the JS package
npm run build:js

# Run tests
npm run test:js
```

### Playground

```bash
# Start development server
npm run dev:playground

# Build for production
npm run build:playground
```

### Running Tests

```bash
# Run C++ tests
cd packages/tml-cpp/build && ctest -V

# Run JS tests
npm run test:js

# Run C# tests
dotnet test packages/tml-cs/tests/TextMateLib.Tests/TextMateLib.Tests.csproj
```

## Usage Examples

### JavaScript / TypeScript (NPM)

```typescript
import { TextMate, Registry, Grammar } from 'textmatelib';

// Create a TextMate instance
const textmate = new TextMate();

// Initialize with WASM module
await textmate.init();

// Create a registry and load grammars
const registry = new Registry();
await registry.loadGrammarFromFile('path/to/javascript.json');

// Get grammar and tokenize
const grammar = registry.grammarForScopeName('source.js');
const tokens = grammar.tokenizeString('const x = 42;');

// Process tokens
tokens.forEach(token => {
  console.log(`Token: ${token.value} (scope: ${token.scope})`);
});
```

See [JavaScript Bindings Documentation](packages/tml-js/README.md) for detailed usage.

### C# / .NET

```csharp
using TextMateLib.Bindings;

// Load a theme
using var theme = Theme.LoadFromFile("path/to/theme.json");

// Get colors and styles for scopes
uint foreground = theme.GetForeground("keyword.control", 0xFFFFFFFF);
FontStyle style = theme.GetFontStyle("comment", FontStyle.None);

// Create a registry and load grammars
using var registry = new Registry();
registry.AddGrammarFromFile("path/to/javascript.json");
using var grammar = registry.LoadGrammar("source.js");
```

See [C# Bindings Documentation](packages/tml-cs/README.md) for detailed usage.

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
