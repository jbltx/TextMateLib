import type { WasmModule, TokenizeResult, TokenizeResult2, RuleStack } from './types';
/**
 * Represents a TextMate grammar for syntax highlighting
 */
export declare class Grammar {
    private native;
    /**
     * Creates a Grammar wrapper for a native grammar handle
     * @param module The WASM module
     * @param handle The native grammar handle
     * @internal
     */
    constructor(module: WasmModule, handle: number);
    /**
     * Tokenize a single line of text
     * @param line The line text to tokenize
     * @param prevState The rule stack from the previous line (null for first line)
     * @returns The tokenization result with tokens and rule stack for next line
     */
    tokenizeLine(line: string, prevState?: RuleStack): TokenizeResult;
    /**
     * Tokenize a single line of text with binary format
     * @param line The line text to tokenize
     * @param prevState The rule stack from the previous line (null for first line)
     * @returns The tokenization result with binary tokens and rule stack
     */
    tokenizeLine2(line: string, prevState?: RuleStack): TokenizeResult2;
    /**
     * Get the scope name of this grammar
     * @returns The scope name (e.g., "source.js")
     */
    getScopeName(): string;
}
