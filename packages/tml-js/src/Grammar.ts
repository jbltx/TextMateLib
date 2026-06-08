import type {
  WasmModule,
  NativeGrammar,
  Token,
  TokenizeResult,
  TokenizeResult2,
  TokenizeLinesResult2,
  TokenizeLinesScopeFlatResult,
  RuleStack,
} from './types';

/**
 * Represents a TextMate grammar for syntax highlighting
 */
export class Grammar {
  private native: NativeGrammar;

  /**
   * Creates a Grammar wrapper for a native grammar handle
   * @param module The WASM module
   * @param handle The native grammar handle
   * @internal
   */
  constructor(module: WasmModule, handle: number) {
    this.native = new module.Grammar(handle);
  }

  /**
   * Tokenize a single line of text
   * @param line The line text to tokenize
   * @param prevState The rule stack from the previous line (null for first line)
   * @returns The tokenization result with tokens and rule stack for next line
   */
  tokenizeLine(line: string, prevState: RuleStack = null): TokenizeResult {
    // Build the token objects in JS from a flat numeric buffer + per-line scope dictionary
    // rather than marshalling one object (and a scope-string array) per token across the WASM
    // boundary — same output, but native JS object construction is far cheaper than building
    // `val` objects. The flat buffer is a view into WASM heap memory only valid until the next
    // WASM call, so we consume it fully here within this synchronous call.
    const { tokens, scopeNames, ruleStack } = this.native.tokenizeLineScopeFlat(line, prevState);
    const out: Token[] = [];
    let cursor = 0;
    while (cursor < tokens.length) {
      const startIndex = tokens[cursor++];
      const endIndex = tokens[cursor++];
      const scopeCount = tokens[cursor++];
      const scopes: string[] = new Array(scopeCount);
      for (let s = 0; s < scopeCount; s++) scopes[s] = scopeNames[tokens[cursor++]];
      out.push({ startIndex, endIndex, scopes });
    }
    return { tokens: out, ruleStack };
  }

  /**
   * Tokenize a single line of text with binary format
   * @param line The line text to tokenize
   * @param prevState The rule stack from the previous line (null for first line)
   * @returns The tokenization result with binary tokens and rule stack
   */
  tokenizeLine2(line: string, prevState: RuleStack = null): TokenizeResult2 {
    return this.native.tokenizeLine2(line, prevState);
  }

  /**
   * Tokenize a whole document in a single call.
   *
   * The rule stack is carried across lines inside WASM, so this performs one JS<->WASM
   * crossing for the entire document instead of one per line. Prefer this over looping
   * {@link tokenizeLine} when highlighting static text; use the per-line API for live
   * editing where individual lines change.
   *
   * @param lines The document lines (already split on newlines)
   * @returns An array with one entry per line, each an array of scope tokens
   */
  tokenizeLines(lines: string[]): Token[][] {
    // Build the rich token objects in JS from a flat numeric buffer + interned scope
    // dictionary, rather than marshalling one object (and a scope-string array) per token
    // across the WASM boundary. The embind object path costs ~1.5x more for identical output
    // — native JS object construction is far cheaper than constructing `val` objects. The
    // flat buffers are views into WASM heap memory only valid until the next WASM call, so we
    // consume them fully here within this synchronous call.
    const { tokens, lineTokenCounts, scopeNames } = this.native.tokenizeLinesScopeFlat(lines);
    const out: Token[][] = new Array(lineTokenCounts.length);
    let cursor = 0;
    for (let li = 0; li < lineTokenCounts.length; li++) {
      const n = lineTokenCounts[li];
      const lineTokens: Token[] = new Array(n);
      for (let t = 0; t < n; t++) {
        const startIndex = tokens[cursor++];
        const endIndex = tokens[cursor++];
        const scopeCount = tokens[cursor++];
        const scopes: string[] = new Array(scopeCount);
        for (let s = 0; s < scopeCount; s++) scopes[s] = scopeNames[tokens[cursor++]];
        lineTokens[t] = { startIndex, endIndex, scopes };
      }
      out[li] = lineTokens;
    }
    return out;
  }

  /**
   * Tokenize a whole document in a single call, binary (themed) format.
   *
   * Like {@link tokenizeLines} but returns the compact encoded-token representation that
   * a highlighter consumes. All tokens are packed into one flat `Uint32Array`; walk
   * `lineTokenCounts` to recover per-line ranges.
   *
   * @param lines The document lines (already split on newlines)
   * @returns The packed tokens plus per-line token counts
   */
  tokenizeLines2(lines: string[]): TokenizeLinesResult2 {
    const result = this.native.tokenizeLines2(lines);
    // The native arrays are views into WASM heap memory that are reused on the next
    // call; copy them into JS-owned arrays before returning.
    return {
      tokens: new Uint32Array(result.tokens),
      lineTokenCounts: new Uint32Array(result.lineTokenCounts),
    };
  }

  /**
   * Tokenize a whole document into a flat, dictionary-encoded scope representation.
   *
   * Equivalent scope information to {@link tokenizeLines} but encoded as a single numeric
   * buffer plus an interned scope-name dictionary, avoiding the per-token object marshalling
   * across the WASM boundary. Prefer this over {@link tokenizeLines} when highlighting static
   * text and you can consume the flat layout directly.
   *
   * @param lines The document lines (already split on newlines)
   * @returns The flat token buffer, per-line token counts, and the scope-name dictionary
   */
  tokenizeLinesScopeFlat(lines: string[]): TokenizeLinesScopeFlatResult {
    const result = this.native.tokenizeLinesScopeFlat(lines);
    // The numeric arrays are views into WASM heap memory reused on the next call; copy them
    // into JS-owned arrays before returning. scopeNames is already a JS array.
    return {
      tokens: new Uint32Array(result.tokens),
      lineTokenCounts: new Uint32Array(result.lineTokenCounts),
      scopeNames: result.scopeNames,
    };
  }

  /**
   * Get the scope name of this grammar
   * @returns The scope name (e.g., "source.js")
   */
  getScopeName(): string {
    return this.native.getScopeName();
  }
}
