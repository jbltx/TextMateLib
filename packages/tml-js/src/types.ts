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
 * Result of tokenizing a whole document with binary format in a single call.
 *
 * Tokens for every line are packed contiguously into `tokens` as [startIndex, metadata]
 * pairs. `lineTokenCounts[i]` is the number of uint32 entries (2 per token) belonging to
 * line `i`, so the per-line slices can be reconstructed by walking `lineTokenCounts`.
 */
export interface TokenizeLinesResult2 {
  /** Flat binary token data for all lines, concatenated in line order */
  tokens: Uint32Array;
  /** Number of uint32 entries in `tokens` produced for each line */
  lineTokenCounts: Uint32Array;
}

/**
 * Result of the flat scope batch path.
 *
 * `tokens` is a single flat buffer; per token it holds
 * `[startIndex, endIndex, scopeCount, scopeId0, scopeId1, ...]`. `lineTokenCounts[i]` is the
 * number of tokens on line `i`, and `scopeNames[id]` resolves a scope id back to its string.
 * This avoids building a JS object per token across the WASM boundary.
 */
export interface TokenizeLinesScopeFlatResult {
  /** Flat token buffer: per token [startIndex, endIndex, scopeCount, ...scopeIds] */
  tokens: Uint32Array;
  /** Number of tokens produced for each line */
  lineTokenCounts: Uint32Array;
  /** Scope-name dictionary; index with the scope ids stored in `tokens` */
  scopeNames: string[];
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
  tokenizeLineScopeFlat(
    line: string,
    ruleStack: RuleStack
  ): { tokens: Uint32Array; scopeNames: string[]; ruleStack: RuleStack };
  tokenizeLine2(line: string, ruleStack: RuleStack): TokenizeResult2;
  tokenizeLines(lines: string[]): Token[][];
  tokenizeLines2(lines: string[]): { tokens: Uint32Array; lineTokenCounts: Uint32Array };
  tokenizeLinesScopeFlat(lines: string[]): {
    tokens: Uint32Array;
    lineTokenCounts: Uint32Array;
    scopeNames: string[];
  };
  getScopeName(): string;
}
