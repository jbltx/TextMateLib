/**
 * Represents a single token from tokenization
 */
export interface Token {
  /** Start index (inclusive) in the line */
  startIndex: number;
  /** End index (exclusive) in the line */
  endIndex: number;
  /** Scope names for this token */
  scopes: string[];
}

/**
 * Result of tokenizing a single line
 */
export interface TokenizeResult {
  /** Array of tokens for this line */
  tokens: Token[];
  /** Rule stack to pass to the next line's tokenization */
  ruleStack: RuleStack;
}

/**
 * Result of tokenizing a single line with binary format
 */
export interface TokenizeResult2 {
  /** Binary token data (Uint32Array-like) */
  tokens: number[];
  /** Rule stack to pass to the next line's tokenization */
  ruleStack: RuleStack;
}

/**
 * Opaque rule stack type (pointer)
 */
export type RuleStack = number | null;

/**
 * Theme settings for a scope
 */
export interface ThemeSettings {
  foreground?: string;
  background?: string;
  fontStyle?: string;
}

/**
 * Internal WASM module interface
 */
export interface WasmModule {
  Registry: new () => NativeRegistry;
  Grammar: new (handle: number) => NativeGrammar;
}

/**
 * Native Registry interface from WASM bindings
 */
export interface NativeRegistry {
  loadGrammarFromContent(content: string, scopeName: string): number | null;
  setTheme(themeContent: string): boolean;
  getColorMap(): string[];
}

/**
 * Native Grammar interface from WASM bindings
 */
export interface NativeGrammar {
  tokenizeLine(line: string, ruleStack: RuleStack): TokenizeResult;
  tokenizeLine2(line: string, ruleStack: RuleStack): TokenizeResult2;
  getScopeName(): string;
}
