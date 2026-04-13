@mainpage TextMateLib - Modern C++ Syntax Highlighting Engine

# TextMateLib (tml)

**Modern C++ implementation of the TextMate syntax highlighting engine** for cross-platform syntax highlighting with high performance and minimal dependencies.

## Overview

TextMateLib provides **grammar-based syntax highlighting** using TextMate-format grammars and themes. It tokenizes source code line-by-line, returning tokens with scope information that can be styled using a theme.

**Key Features:**
- ✅ **Grammar-based tokenization** - TextMate format grammars for any language
- ✅ **Stateful incremental parsing** - Efficient line-by-line processing with caching
- ✅ **Theme support** - Apply colors and styles to tokens via scopes
- ✅ **Multiple language support** - 30+ built-in language grammars
- ✅ **High performance** - Optimized for editor integration
- ✅ **Zero dependencies** - Regex (Oniguruma) and JSON (RapidJSON) bundled
- ✅ **Cross-platform** - Linux, macOS, Windows, WebAssembly
- ✅ **Language bindings** - C, C#/.NET, JavaScript/WASM, Python (planned)

---

## Quick Start

### Basic C++ Usage

```cpp
#include "tml.h"
using namespace tml;

// 1. Create regex engine and registry
auto onigLib = std::make_shared<DefaultOnigLib>();
auto registry = std::make_shared<Registry>(onigLib);

// 2. Load a grammar
registry->addGrammarFromFile("path/to/javascript.json");
auto grammar = registry->loadGrammar("source.javascript");

// 3. Load a theme
auto theme = Theme::createFromPath("path/to/dark-plus.json");

// 4. Tokenize a line
auto prevState = StateStack::INITIAL;
auto result = grammar->tokenizeLine("const x = 42;", prevState);

// 5. Get colors for tokens
for (const auto& token : result->tokens) {
    auto scopePath = token->scopePath();
    uint32_t color = theme->match(scopePath);
    printf("Token '%s' -> color: #%08x\n", scopePath.c_str(), color);
}
```

### Basic C API Usage

```c
#include "c_api.h"

// 1. Create registry
TextMateOnigLib onigLib = textmate_oniglib_create();
TextMateRegistry registry = textmate_registry_create(onigLib);

// 2. Load grammar and theme
textmate_registry_add_grammar_from_file(registry, "javascript.json");
TextMateGrammar grammar = textmate_registry_load_grammar(registry, "source.javascript");
TextMateTheme theme = textmate_theme_load_from_file("dark-plus.json");

// 3. Tokenize a line
TextMateStateStack state = textmate_get_initial_state();
TextMateTokenizeResult* result = textmate_tokenize_line(grammar, "const x = 42;", state);

// 4. Get colors for tokens
for (int32_t i = 0; i < result->tokenCount; i++) {
    TextMateToken token = result->tokens[i];
    char* scope = token.scopes[0];
    uint32_t color = textmate_theme_get_foreground(theme, scope, 0xFFFFFFFF);
}

// 5. Cleanup
textmate_free_tokenize_result(result);
textmate_theme_dispose(theme);
textmate_registry_dispose(registry);
textmate_oniglib_dispose(onigLib);
```

---

## Core Concepts

### Grammar and Tokenization

A **grammar** defines syntax rules for a language. TextMateLib tokenizes text by:
1. Matching text against grammar patterns
2. Entering/exiting rules as patterns match/end
3. Outputting tokens with scope hierarchies

**Tokens** represent text ranges with **scope information**:
- Scope: `source.js keyword.control` (space-separated hierarchy)
- Range: character positions in the line

### StateStack - Incremental Parsing

The **StateStack** represents the parsing state at the end of a line:
- Encodes which grammar rules are active
- Enables resuming on the next line
- Two StateStacks that `equals()` mean the same parsing position

This enables **incremental tokenization**: if a line's initial state hasn't changed, its tokens might not have either (early stopping optimization).

### Themes - Styling Tokens

A **theme** maps scopes to colors and font styles:
- Scope path matching: `keyword.control` matches `source.js keyword.control`
- Returns: foreground color, background color, font style (italic/bold/underline)
- Fallback: default theme colors when no match found

---

## API Overview

### C++ API (`tml.h`)

**High-level, type-safe C++ interface:**

| Component | Purpose |
|-----------|---------|
| @ref core_types | Grammar, Theme, Registry, StateStack, Token types |
| @ref grammar_processing | Parse and compile grammar definitions |
| @ref tokenization | Core tokenization logic and token representation |
| @ref session_api | Stateful incremental API with line caching (recommended) |
| @ref theme_api | Load themes and query colors for scopes |
| @ref syntax_highlighting | Convenience wrapper combining grammar + theme |
| @ref constants | Global constants (INITIAL state) |

**Example - Using Session API (recommended for editors):**

```cpp
#include "tml.h"
using namespace tml;

// Create session for a document
auto session = SessionImpl::create(grammar, /*line count*/ 100);

// Set initial lines
std::vector<std::string> lines = {"const x = 1;", "const y = 2;", "..."};
session->setLines(lines);

// Query tokens for a line
auto tokens = session->getLine(0)->tokens;

// Edit: replace line 1-2 with new content
session->edit(1, 2, {"const a = 1;", "const b = 2;"});

// Incremental tokenization happens automatically
// Only affected lines are re-parsed (early stopping optimization)
```

### C API (`c_api.h`)

**FFI interface for language bindings (C#, Python, Node.js, etc.):**

| Group | Functions |
|-------|-----------|
| @ref opaque_types | Handle types (TextMateRegistry, TextMateGrammar, etc.) |
| @ref token_structures | Token and result structures (marshalling-friendly) |
| @ref theme_api | Load and query theme colors |
| @ref registry_api | Create registries, register/load grammars |
| @ref tokenization_api | Tokenize single/multiple lines, manage state |

**Key differences from C++:**
- Opaque handles instead of C++ objects
- Explicit memory management (malloc/free)
- Stateless tokenization (manage state yourself)
- Batch operations for reducing FFI overhead

---

## Common Workflows

### 1. Basic Syntax Highlighting

```cpp
// Setup (once)
auto onigLib = std::make_shared<DefaultOnigLib>();
auto registry = std::make_shared<Registry>(onigLib);
registry->addGrammarFromFile("javascript.json");
auto grammar = registry->loadGrammar("source.javascript");
auto theme = Theme::createFromPath("dark-plus.json");

// Highlight a single line
auto state = StateStack::INITIAL;
auto result = grammar->tokenizeLine("const x = 42;", state);
// Use result->tokens with theme->match() for colors
// Save result->ruleStack for next line
```

### 2. Multi-Line Document (Session API)

```cpp
// Setup
auto session = SessionImpl::create(grammar, lines.size());
session->setLines(lines);  // Initialize with all lines
session->setTheme(theme);  // Optional: for HighlightedLine

// Get highlighted tokens for line 0
auto highlightedLine = session->getLine(0);
for (const auto& token : highlightedLine->tokens) {
    // token.foreground, token.background, token.fontStyle
}

// Edit: replace lines 5-7
session->edit(5, 7, {"new line 1", "new line 2"});
// Incremental tokenization: only lines 5-10 (approx) are re-parsed
```

### 3. Stateless Tokenization (C API or Low-Level)

```c
// Tokenize multiple lines manually, managing state
TextMateStateStack state = textmate_get_initial_state();
TextMateTokenizeResult* lineResults[10];

for (int i = 0; i < 10; i++) {
    lineResults[i] = textmate_tokenize_line(grammar, lines[i], state);
    state = lineResults[i]->ruleStack;  // Pass state to next line
}
```

### 4. Batch Tokenization (Reducing FFI Overhead)

```c
// Tokenize all 10 lines in one FFI call (C API)
TextMateTokenizeMultiLinesResult* batch = textmate_tokenize_lines(
    grammar, lines, 10, textmate_get_initial_state()
);

for (int i = 0; i < 10; i++) {
    // batch->lineResults[i] -> tokens for line i
}

textmate_free_tokenize_lines_result(batch);
```

---

## Performance Tips

### 1. Use Session API for Editors

The **Session API** (C++) handles incremental tokenization automatically:
- Caches line tokens
- Detects when state hasn't changed (early stopping)
- Only re-parses affected lines

**Impact:** Editing a line in a 10,000-line file only re-tokenizes ~10 lines.

### 2. Reuse Grammar and Theme Objects

Create grammar and theme once, reuse for all tokenization:
```cpp
// GOOD: Create once
auto grammar = registry->loadGrammar("source.javascript");
auto theme = Theme::createFromPath("dark-plus.json");

// Use for many documents
for (auto& document : documents) {
    auto session = SessionImpl::create(grammar, document->lines.size());
    // ...
}

// BAD: Create repeatedly
for (auto& line : lines) {
    auto grammar = registry->loadGrammar("source.javascript");  // Wasteful!
    // ...
}
```

### 3. Use Encoded Tokens (C API)

For performance-critical code, use `textmate_tokenize_line2()` instead of `textmate_tokenize_line()`:
- Returns compact 32-bit encoded tokens instead of scope arrays
- Reduces memory overhead
- Faster for large files

### 4. Batch Operations (C API)

Use `textmate_tokenize_lines()` to tokenize multiple lines in one FFI call:
- Reduces FFI call overhead (important for .NET, Python bindings)
- Better for batch processing

### 5. Early Stopping

Session API automatically limits tokenization depth:
- Prevents pathological regex cases (catastrophic backtracking)
- Can be tuned via SessionImpl configuration

---

## Architecture

### Component Hierarchy

```
┌─────────────────────────────────────────┐
│  Public APIs (C++, C, WASM)            │
├─────────────────────────────────────────┤
│  Session API     │  Syntax Highlighter  │
│  (Stateful)      │  (Convenience)       │
├─────────────────────────────────────────┤
│  Grammar (Tokenization Logic)            │
│  - Rule matching, state transitions      │
│  - Scope stack management                │
├─────────────────────────────────────────┤
│  Registry (Grammar/Theme Management)     │
│  - Grammar lookup and caching            │
│  - Theme application                     │
├─────────────────────────────────────────┤
│  Core Components                         │
│  - Regex engine (Oniguruma)             │
│  - JSON parser (RapidJSON)              │
│  - Scope/attribute providers            │
└─────────────────────────────────────────┘
```

### Key Data Structures

| Type | Purpose |
|------|---------|
| `Grammar` | Compiled grammar with rule matching logic |
| `StateStack` | Immutable parsing state for incremental resumption |
| `Theme` | Scope-to-color-and-style mapping |
| `IToken` | Single token with scope hierarchy |
| `Registry` | Central manager for grammars and themes |
| `Session` | Stateful document with line caching |

---

## API Reference

### Core Modules

**Grammar & Tokenization:**
- @ref core_types - Type definitions and interfaces
- @ref grammar_processing - Parse grammars from JSON
- @ref tokenization - Core tokenization engine
- @ref session_api - High-level session API

**Styling:**
- @ref theme_api - Theme parsing and matching
- @ref styling - Font style flags and attributes

**C++ Convenience:**
- @ref syntax_highlighting - Combined grammar + theme API

**C Bindings:**
- @ref c_api.h "C_API" - Complete C FFI

---

## Platform Support

| Platform | Status | API |
|----------|--------|-----|
| Linux | ✅ | C++, C |
| macOS | ✅ | C++, C |
| Windows | ✅ | C++, C |
| WebAssembly | ✅ | C++, C, JavaScript |
| .NET (C#) | ✅ | C (via P/Invoke) |
| Node.js | ✅ | WASM |

---

## Building and Installation

### Native Build

```bash
./scripts/build.sh
# Output: build/lib/libtm.so, build/include/tml/
```

### WebAssembly Build

```bash
./scripts/build-wasm-standard.sh
# Output: build/wasm-standard/browser/tml-standard.js + .wasm
```

### C# / .NET Bindings

```bash
./scripts/build-shared.sh
cd tests/csharp/TextMateLib.Tests
dotnet test
```

See `CLAUDE.md` in the project root for detailed build instructions.

---

## Examples

### Load Multiple Grammars

```cpp
auto registry = std::make_shared<Registry>(onigLib);

// Register grammars for different languages
registry->addGrammarFromFile("javascript.json");
registry->addGrammarFromFile("python.json");
registry->addGrammarFromFile("markdown.json");

// Load as needed
auto jsGrammar = registry->loadGrammar("source.javascript");
auto pyGrammar = registry->loadGrammar("source.python");
auto mdGrammar = registry->loadGrammar("text.markdown");
```

### Grammar Injections (Embedded Grammars)

```cpp
// Inject regex highlighting into JavaScript strings
const char* injections[] = {"source.regexp"};
registry->setInjections("source.js string.quoted", injections, 1);

// Now JavaScript strings will highlight regex patterns
auto grammar = registry->loadGrammar("source.javascript");
auto result = grammar->tokenizeLine("const pattern = /[a-z]+/;", state);
// Token scopes: source.js string.quoted source.regexp
```

### Edit a Document

```cpp
auto session = SessionImpl::create(grammar, 100);
session->setLines(original_lines);

// User edits lines 10-12
std::vector<std::string> new_lines = {"edited line 1", "edited line 2"};
session->edit(10, 12, new_lines);

// Incremental tokenization happens automatically
// Only affected lines (10-20 approx) are re-parsed
auto tokens = session->getLine(10)->tokens;
```

---

## Troubleshooting

### Tokenization Not Working

1. **Grammar not found**: Ensure grammar was registered before loading
   ```cpp
   registry->addGrammarFromFile("grammar.json");  // Must do this first
   auto grammar = registry->loadGrammar("source.mylang");
   ```

2. **Empty tokens**: Check grammar has valid rules and patterns

3. **State issues**: Always pass the previous line's `ruleStack` as `prevState`
   ```cpp
   auto result1 = grammar->tokenizeLine(line1, INITIAL);
   auto result2 = grammar->tokenizeLine(line2, result1->ruleStack);  // Pass state!
   ```

### Theme Colors Not Applying

1. **Scope not matching**: Theme only returns colors for matched scopes
   - Use `theme->match(scopePath)` to test scope matching
   - TextMate scope matching uses prefix rules

2. **Default color**: If scope not found, theme returns `defaultColor`
   ```cpp
   // If "my.custom.scope" not in theme, returns defaultColor
   uint32_t color = theme->match("my.custom.scope", 0xFFFFFFFF);
   ```

### Performance Issues

1. **Slow tokenization**: Use Session API instead of manual state management
2. **High memory**: Reduce `SessionImpl` line cache size or use stateless tokenization
3. **FFI overhead**: Use batch operations (`textmate_tokenize_lines`) instead of per-line calls

---

## Contributing

TextMateLib is open source! Contributions welcome:
- Report issues: [GitHub Issues](https://github.com/jbltx/TextMateLib)
- Submit PRs: Bug fixes, optimizations, language bindings
- Add grammars: New language support via grammar files

See project README for contribution guidelines.

---

## License

TextMateLib is distributed under the [MIT License](https://github.com/jbltx/TextMateLib/blob/main/LICENSE).

---

## Further Reading

- **Architecture & Design**: See `CLAUDE.md` in the project root
- **API Reference**: Browse the detailed API documentation
- **Grammars**: TextMate grammar format [specification](https://macromates.com/manual/en/language_grammars)
- **Themes**: TextMate theme format [documentation](https://macromates.com/manual/en/themes)
- **Examples**: See `examples/` directory in the project

