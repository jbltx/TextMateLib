import type { WasmModule, NativeGrammar, TokenizeResult, TokenizeResult2, RuleStack } from './types';

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
    return this.native.tokenizeLine(line, prevState);
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
   * Get the scope name of this grammar
   * @returns The scope name (e.g., "source.js")
   */
  getScopeName(): string {
    return this.native.getScopeName();
  }
}
