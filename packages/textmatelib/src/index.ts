/**
 * TextMateLib - High-performance syntax highlighting with TextMate grammars
 * Modern C++ implementation with WebAssembly bindings
 *
 * @example
 * ```typescript
 * import { createSyntaxHighlighter } from 'textmatelib';
 *
 * const highlighter = await createSyntaxHighlighter({
 *   grammarContent: javascriptGrammarJson,
 *   themeContent: draculaThemeJson,
 *   scopeName: 'source.js'
 * });
 *
 * const highlighted = highlighter.highlight('const x = 42;');
 * ```
 */

// High-level convenience API
export { SyntaxHighlighter, createSyntaxHighlighter } from './high-level';

// Low-level APIs for advanced use cases
export { Registry, Grammar, createRegistry, createGrammar } from './low-level';

// WASM module loader
export { loadTextMateModule, initializeModule, getTextMateModule } from './loader';

// Type definitions
export type {
  Token,
  TokenizeResult,
  RuleStack,
  IRegistry,
  IGrammar,
  ISyntaxHighlighter,
  HighlightedToken,
  SyntaxHighlighterOptions,
  TextMateWasmModule,
  WasmLoaderOptions,
} from './types';
