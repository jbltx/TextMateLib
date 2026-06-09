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

## Benchmarks

TextMateLib is benchmarked against the two reference JavaScript TextMate engines:
[`microsoft/vscode-textmate`](https://github.com/microsoft/vscode-textmate) and
[`shikijs/shiki`](https://github.com/shikijs/shiki). All three are Oniguruma-backed
TextMate engines, so they tokenize identical grammars to identical tokens — the
benchmark verifies this (tml-js and vscode-textmate produce the exact same token counts).

**Methodology.** Each engine tokenizes the same real-world files using the same grammars
([`tm-grammars`](https://github.com/shikijs/textmate-grammars-themes)) and the same theme
(`github-dark`). We report the median of 7 runs (3 warmup) as MB/s. Two modes are measured:
**scope** tokenization (scope-name tokens) and **themed** tokenization (binary/themed tokens —
what an editor or highlighter actually consumes). For `tml-js` we show two call paths:
**per-line** (looping `tokenizeLine`, carrying the rule stack in JS — the right API for live
editing) and **batch** (`tokenizeLines`/`tokenizeLines2`, which tokenizes a whole document in
one call with the rule stack carried inside WASM — the right API for highlighting static text).
We also include the **.NET binding** (`tml-cs`), which P/Invokes the same native engine through
a shared library.
Numbers below were measured on an **Apple M4 Max**, **Node v24.4.0**, **.NET 9.0.303**, macOS arm64,
against `vscode-textmate@9.3.2` and `shiki@1.29.2`.

### Scope tokenization

| File | Size | tml-js (per-line) | tml-js (batch) | tml-cs (P/Invoke) ¹ | vscode-textmate | native C++ ² |
|------|------|-------------------|----------------|---------------------|-----------------|--------------|
| TypeScript (`vscode.d.ts`) | 0.71 MB | 2.2 MB/s | 2.7 MB/s | 3.6 MB/s | 3.7 MB/s | 4.0 MB/s |
| JavaScript (jQuery) | 0.27 MB | 0.6 MB/s | 0.6 MB/s | 0.9 MB/s | 0.8 MB/s | 0.9 MB/s |
| CSS (Bootstrap) | 0.27 MB | 0.8 MB/s | 0.9 MB/s | 1.2 MB/s | 1.3 MB/s | 1.3 MB/s |
| Python (`typing.py`) | 0.13 MB | 1.3 MB/s | 1.4 MB/s | 1.9 MB/s | 1.7 MB/s | 2.0 MB/s |

### Themed tokenization

| File | Size | tml-js (per-line) | tml-js (batch) | tml-cs (P/Invoke) ¹ | vscode-textmate | shiki | native C++ ² |
|------|------|-------------------|----------------|---------------------|-----------------|-------|--------------|
| TypeScript (`vscode.d.ts`) | 0.71 MB | 3.2 MB/s | 3.6 MB/s | 4.7 MB/s | 3.7 MB/s | 1.9 MB/s | 4.9 MB/s |
| JavaScript (jQuery) | 0.27 MB | 0.8 MB/s | 0.8 MB/s | 1.1 MB/s | 0.8 MB/s | 0.4 MB/s | 1.1 MB/s |
| CSS (Bootstrap) | 0.27 MB | 1.0 MB/s | 1.0 MB/s | 1.4 MB/s | 1.3 MB/s | 0.7 MB/s | 1.4 MB/s |
| Python (`typing.py`) | 0.13 MB | 1.5 MB/s | 1.6 MB/s | 2.1 MB/s | 1.7 MB/s | 0.8 MB/s | 2.3 MB/s |

**What this shows.** Throughput is dominated by Oniguruma regex matching, which every engine
shares. TextMateLib's **native C++ engine matches or slightly beats `vscode-textmate`**. The
**WASM/JS binding (`tml-js`) is slower**; two costs account for the gap:

1. **Result marshalling.** The scope path returns a rich token object (`{startIndex, endIndex,
   scopes[]}`) per token. Building those across the WASM/embind boundary is expensive — a
   separate diagnostic put it at roughly a third of the scope-path cost. So **both** call paths
   now cross the boundary as a flat numeric buffer plus an interned scope-name dictionary and
   rebuild the objects in plain JS. Same tokens, identical output (the benchmark asserts exact
   token-count parity), but faster on every path (e.g. per-line TypeScript scope 1.9 → 2.2 MB/s
   versus the old per-token-object path). With marshalling flattened, batch throughput
   essentially matches the engine running with no marshalling at all. The themed path was
   already flat-encoded, so it changes little.
2. **The WASM engine itself.** Even with marshalling flattened, `tml-js` (2.7 MB/s batch on
   TypeScript scope) trails `vscode-textmate`'s pure-JS engine (3.7) and the native C++ build
   (4.0). This residual is WASM-vs-JIT execution, not the binding — and it is the next thing to
   optimize. The **.NET binding (`tml-cs`) confirms this**: it crosses a P/Invoke boundary with
   the same per-token scope marshalling as the JS path, yet runs the engine natively and lands at
   3.6 MB/s on TypeScript scope — right at the native ceiling and well above `tml-js`. Same engine,
   different execution model.

The remaining **per-line vs batch** difference (2.2 → 2.7 on TypeScript scope) is just the
JS↔WASM call overhead: batch makes one crossing for the whole document, per-line makes one per
line. Marshalling no longer separates them.

In **themed** tokenization `tml-js` lands within a few percent of `vscode-textmate` on most
files (CSS is the widest gap) and is **roughly 2× faster than `shiki`**, which builds richer
themed token objects. The **native C++ and .NET** paths are **faster in themed mode than in scope
mode** — themed tokenization returns a flat binary buffer (uint32 pairs) with no per-token
scope-string construction, so the marshalling cost that dominates the scope path vanishes.
For highlighting static text,
prefer the batch API (`tokenizeLines` / `tokenizeLines2`); for live editing where individual
lines change, use the per-line API (`tokenizeLine` / `tokenizeLine2`).

¹ *`tml-cs` is the managed .NET binding ([`tml-cs`](packages/tml-cs/)) calling the native engine
through P/Invoke. It is a real binding number (includes per-token scope marshalling on the scope
path), shown here to contrast the native execution model with the WASM one. The themed path
returns a flat binary buffer, so marshalling cost is negligible.*

² *Native C++ is a raw-engine reference, **not** an apples-to-apples comparison: it runs
in-process with no JS↔WASM marshalling. It is included to show the engine's ceiling.*

### Reproduce

```bash
npm install                 # installs the benchmarks workspace
npm run bench:fixtures      # download the real-world sample files
npm run bench               # JS + native C++ + .NET P/Invoke
```

The harness lives in [`benchmarks/`](benchmarks/) and writes a Markdown summary to
`benchmarks/results.md`. The C# row needs the .NET SDK and builds the native shared library
on first run (`scripts/build-shared.sh`); it is skipped gracefully when `dotnet` is absent.
Fixtures download from upstream (jQuery, Bootstrap, VS Code, CPython)
and fall back to synthesized files when offline.

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
