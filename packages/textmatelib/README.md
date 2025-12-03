# TextMateLib

High-performance syntax highlighting engine with TextMate grammar support and WebAssembly bindings for JavaScript/Node.js.

**Features:**

- ✨ Accurate syntax highlighting using TextMate-format grammars
- 🚀 High-performance tokenization via WebAssembly (compiled C++)
- 🎨 Theme/color scheme support for styled output
- 📦 Pre-built WASM binaries - no compilation step needed
- 📘 Full TypeScript support with type definitions
- 🔄 Dual CommonJS + ES Module support
- 🌐 Works in browsers and Node.js

## Installation

```bash
npm install textmatelib
```

## Quick Start

### High-Level API (Recommended for most use cases)

The simplest way to highlight code:

```typescript
import { createSyntaxHighlighter } from 'textmatelib';

const highlighter = await createSyntaxHighlighter({
  grammarContent: javascriptGrammarJson, // JSON string
  themeContent: draculaThemeJson, // JSON string
  scopeName: 'source.js',
});

// Highlight code - returns 2D array [lines[tokens]]
const highlighted = highlighter.highlight('const x = 42;');

// Each token includes: startIndex, endIndex, scopes, foreground, background, fontStyle
highlighted[0].forEach((token) => {
  console.log(`Token: "${code.substring(token.startIndex, token.endIndex)}"`);
  console.log(`Color: ${token.foreground}`);
  console.log(`Scopes: ${token.scopes.join(' > ')}`);
});
```

### Low-Level API (For advanced use cases)

Direct access to tokenization and state management:

```typescript
import { createRegistry } from 'textmatelib';

const registry = await createRegistry();
const grammar = registry.loadGrammarFromContent(grammarJson, 'source.js');
registry.setTheme(themeJson);

// Tokenize with state management (for line-by-line editors)
let ruleStack = null;
for (const line of lines) {
  const result = grammar.tokenizeLine(line, ruleStack);
  ruleStack = result.ruleStack; // Pass to next line

  result.tokens.forEach((token) => {
    console.log(token);
  });
}
```

## API Reference

### High-Level API: `createSyntaxHighlighter(options)`

Creates a `SyntaxHighlighter` instance for simple, one-shot highlighting.

```typescript
interface SyntaxHighlighterOptions {
  grammarContent: string;  // JSON string of TextMate grammar
  themeContent: string;    // JSON string of theme/color scheme
  scopeName: string;       // Scope name (e.g., 'source.javascript')
}

const highlighter = await createSyntaxHighlighter(options);

// Methods:
highlighter.highlight(code: string): HighlightedToken[][]
highlighter.setGrammar(content: string, scopeName: string): void
highlighter.setTheme(content: string): void
```

#### Result Structure

```typescript
interface HighlightedToken {
  startIndex: number; // Character position in line
  endIndex: number; // End position (exclusive)
  scopes: string[]; // Scope path (e.g., ["source.js", "string.quoted"])
  foreground?: string; // Hex color (e.g., "#ff0000")
  background?: string; // Hex color
  fontStyle?: string; // Space-separated: "bold italic underline"
}
```

### Low-Level API: `createRegistry()`

For advanced scenarios like editor integration with line caching:

```typescript
import { createRegistry } from 'textmatelib';

const registry = await createRegistry();

// Load grammar
const grammar = registry.loadGrammarFromContent(grammarJson, 'source.js');

// Set theme
registry.setTheme(themeJson);

// Tokenize
const result = grammar.tokenizeLine('const x = 42;', null);
result.tokens.forEach((token) => {
  console.log(token.scopes); // No colors - raw token data
});

// Get color palette
const colors = registry.getColorMap();
```

#### Registry Methods

```typescript
class Registry {
  loadGrammarFromContent(content: string, scopeName: string): Grammar;
  setTheme(content: string): void;
  getColorMap(): string[];
}

class Grammar {
  getScopeName(): string;
  tokenizeLine(line: string, ruleStack: RuleStack | null): TokenizeResult;
  tokenizeLine2(line: string, ruleStack: RuleStack | null): object; // Compact format
}

interface TokenizeResult {
  tokens: Token[];
  ruleStack: RuleStack | null;
  stoppedEarly?: boolean;
}

interface Token {
  startIndex: number;
  endIndex: number;
  scopes: string[];
}
```

## Supported Grammars & Themes

The package includes pre-built WASM binaries but no grammars or themes. You need to provide them as JSON:

- **Grammars**: Download from [TextMate](https://github.com/textmate/textmate), [VS Code](https://github.com/microsoft/vscode/tree/main/extensions), or other sources
- **Themes**: [VS Code Themes](https://github.com/microsoft/vscode/tree/main/extensions/theme-*), [TextMate Themes](http://colorsublime.com/), etc.

Example sources:

- https://github.com/microsoft/vscode/tree/main/extensions/javascript/
- https://github.com/one-dark/vscode-one-dark-pro

## Browser Usage

```html
<script type="module">
  import { createSyntaxHighlighter } from 'https://cdn.jsdelivr.net/npm/textmatelib@1.0.0/dist/index.mjs';

  const highlighter = await createSyntaxHighlighter({
    grammarContent: fetch('/grammars/javascript.json').then((r) => r.text()),
    themeContent: fetch('/themes/dracula.json').then((r) => r.text()),
    scopeName: 'source.js',
  });

  const highlighted = highlighter.highlight('const x = 42;');
</script>
```

## Node.js Usage

```javascript
const { createSyntaxHighlighter } = require('textmatelib');
const fs = require('fs');

// Load grammar and theme from files
const grammar = fs.readFileSync('./javascript.json', 'utf-8');
const theme = fs.readFileSync('./dracula.json', 'utf-8');

const highlighter = await createSyntaxHighlighter({
  grammarContent: grammar,
  themeContent: theme,
  scopeName: 'source.js',
});

const code = fs.readFileSync('./example.js', 'utf-8');
const highlighted = highlighter.highlight(code);
```

## Performance Considerations

- **WASM Variant**: This package uses the `standard` variant (balanced feature set)
- **State Caching**: For line-by-line editing, use the low-level API to cache `ruleStack` between lines
- **Compact Format**: Use `grammar.tokenizeLine2()` for compact token format (more efficient)
- **Early Stopping**: The tokenizer can be tuned to stop early for performance vs. accuracy tradeoffs

## Architecture

```
┌─────────────────────────────────┐
│  SyntaxHighlighter (High-level) │ ◄── Most users
└────────────────┬────────────────┘
                 │
┌─────────────────▼────────────────┐
│  Registry + Grammar (Low-level)  │ ◄── Advanced users
└────────────────┬────────────────┘
                 │
┌─────────────────▼────────────────┐
│  WASM Module (Emscripten)        │
└────────────────┬────────────────┘
                 │
┌─────────────────▼────────────────┐
│  C++ (Oniguruma regex, etc.)     │
└──────────────────────────────────┘
```

## WASM Module Details

- **Format**: Emscripten-compiled (ES Module + CommonJS compatible)
- **Size**: ~862 KB total (48 KB JS + 814 KB WASM)
- **Variant**: Standard (Session + Registry + Theme APIs)
- **Browser Support**: Modern browsers (ES2020+)
- **Node Support**: Node.js 14+

## TypeScript Support

Full TypeScript definitions are included:

```typescript
import type {
  Token,
  TokenizeResult,
  HighlightedToken,
  SyntaxHighlighterOptions,
  ISyntaxHighlighter,
  IRegistry,
  IGrammar,
} from 'textmatelib';

// Your code here with full type checking
```

## Examples

See `examples/` directory for complete working examples:

- `examples/basic.js` - Low-level API usage
- `examples/highlighter.js` - High-level API usage

## Troubleshooting

### WASM Module Not Loading

Ensure the WASM files are accessible:

- In browser: Check network tab, CORS headers
- In Node.js: Verify `node_modules/textmatelib/wasm/` exists
- Custom path: Pass `wasmPath` option to `loadTextMateModule()`

```typescript
import { loadTextMateModule } from 'textmatelib';

await loadTextMateModule({
  wasmPath: '/custom/path/to/wasm/',
  debug: true, // Enable logging
});
```

### Tokens Have No Colors

Make sure you:

1. Called `highlighter.setTheme()` or set theme in options
2. Theme has `tokenColors` defined
3. Scopes in tokenizer match theme scope selectors

### Performance Issues

- Use `grammar.tokenizeLine2()` (compact format) for performance
- Cache `ruleStack` for line-by-line tokenization
- Only highlight visible lines in large documents
- Consider the low-level API for custom optimization

## License

MIT - See LICENSE file

## Development

Building the package locally:

```bash
npm install
npm run build     # Compile TypeScript and bundle
npm run dev       # Development mode
npm test          # Run tests
```

## Related Projects

- [TextMateLib (C++)](https://github.com/jbltx/TextMateLib) - The underlying C++ library
- [Oniguruma](https://github.com/kkos/oniguruma) - Regex engine used for pattern matching
- [TextMate](https://github.com/textmate/textmate) - Original grammar format specification

## Contributing

Contributions welcome! Please read [CONTRIBUTING.md](../../CONTRIBUTING.md) first.

## Changelog

See [CHANGELOG.md](../../CHANGELOG.md) for version history.
