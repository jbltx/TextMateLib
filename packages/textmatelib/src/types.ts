/**
 * Type definitions for TextMateLib WASM bindings
 * Low-level and high-level API types
 */

/**
 * Opaque handle to a grammar rule stack
 * Used for stateful tokenization across lines
 */
export type RuleStack = any;

/**
 * A single token in the tokenized output
 */
export interface Token {
  /** Character index where token starts in the line */
  startIndex: number;

  /** Character index where token ends (exclusive) */
  endIndex: number;

  /** Scope path for the token (e.g., ["source.js", "string.quoted.double"]) */
  scopes: string[];
}

/**
 * Result from tokenizing a single line
 */
export interface TokenizeResult {
  /** Array of tokens in the line */
  tokens: Token[];

  /** Rule stack to pass to the next line for stateful tokenization */
  ruleStack: RuleStack | null;

  /** Whether tokenization stopped early (for performance reasons) */
  stoppedEarly?: boolean;
}

/**
 * Low-level Registry API for managing grammars and themes
 */
export interface IRegistry {
  /**
   * Load a grammar from JSON content
   * @param content - Raw JSON string of the grammar definition
   * @param scopeName - The scope name to register under (e.g., "source.javascript")
   * @returns Grammar object for tokenization
   */
  loadGrammarFromContent(content: string, scopeName: string): IGrammar;

  /**
   * Set the active theme for color/style queries
   * @param themeContent - Raw JSON string of the theme definition
   */
  setTheme(themeContent: string): void;

  /**
   * Get the color map from the current theme
   * @returns Array of color strings (hex format)
   */
  getColorMap(): string[];
}

/**
 * Low-level Grammar API for tokenizing code
 */
export interface IGrammar {
  /**
   * Get the scope name of this grammar
   * @returns The scope name (e.g., "source.javascript")
   */
  getScopeName(): string;

  /**
   * Tokenize a single line of code
   * @param lineText - The source code line to tokenize
   * @param ruleStack - The rule stack from the previous line (null for first line)
   * @returns TokenizeResult with tokens and next rule stack
   */
  tokenizeLine(lineText: string, ruleStack: RuleStack | null): TokenizeResult;
}

/**
 * Options for creating a SyntaxHighlighter instance
 */
export interface SyntaxHighlighterOptions {
  /** Raw JSON string of the grammar definition */
  grammarContent: string;

  /** Raw JSON string of the theme definition */
  themeContent: string;

  /** Scope name to register the grammar under */
  scopeName: string;
}

/**
 * Token with color and style information from the theme
 */
export interface HighlightedToken extends Token {
  /** Foreground color as hex string (e.g., "#ff0000") */
  foreground?: string;

  /** Background color as hex string */
  background?: string;

  /** Font style flags (bold, italic, underline) as space-separated string */
  fontStyle?: string;
}

/**
 * High-level convenience API combining grammar, theme, and tokenization
 */
export interface ISyntaxHighlighter {
  /**
   * Highlight code with the current grammar and theme
   * @param code - Source code to highlight (may contain multiple lines)
   * @returns 2D array of HighlightedTokens, indexed by line then token
   */
  highlight(code: string): HighlightedToken[][];

  /**
   * Change the active grammar
   * @param grammarContent - Raw JSON string of the new grammar
   * @param scopeName - Scope name for the new grammar
   */
  setGrammar(grammarContent: string, scopeName: string): void;

  /**
   * Change the active theme for colors and styles
   * @param themeContent - Raw JSON string of the new theme
   */
  setTheme(themeContent: string): void;
}

/**
 * WASM module initialization result
 */
export interface TextMateWasmModule {
  /** Registry constructor for low-level usage */
  Registry: new () => any;

  /** Grammar constructor for low-level usage */
  Grammar: new (handle: any) => any;

  /** Emscripten WASM module object */
  _module?: any;
}

/**
 * Options for loading the WASM module
 */
export interface WasmLoaderOptions {
  /** Custom path to the WASM files (default: wasm/) */
  wasmPath?: string;

  /** Enable debug logging */
  debug?: boolean;
}
