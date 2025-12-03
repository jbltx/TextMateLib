/**
 * Low-level API for TextMateLib
 * Direct wrappers around Emscripten-compiled C++ classes
 * Use this for advanced use cases where you need fine-grained control
 */

import { loadTextMateModule } from './loader';
import type { IRegistry, IGrammar, Token, TokenizeResult, RuleStack } from './types';

/**
 * Low-level Registry API
 * Manages grammar and theme registration
 */
export class Registry implements IRegistry {
  private nativeRegistry: any;

  /**
   * Create a new Registry instance
   * Automatically initializes the WASM module if needed
   */
  constructor(nativeRegistry: any) {
    this.nativeRegistry = nativeRegistry;
  }

  /**
   * Load a grammar from JSON content
   * @param content - Raw JSON string of the grammar
   * @param scopeName - Scope name (e.g., "source.javascript")
   * @returns Grammar object for tokenizing with this grammar
   */
  loadGrammarFromContent(content: string, scopeName: string): Grammar {
    try {
      const nativeGrammar = this.nativeRegistry.loadGrammarFromContent(content, scopeName);
      return new Grammar(nativeGrammar);
    } catch (err) {
      throw new Error(`Failed to load grammar for scope "${scopeName}": ${err}`);
    }
  }

  /**
   * Set the active theme for color queries
   * @param themeContent - Raw JSON string of the theme
   */
  setTheme(themeContent: string): void {
    try {
      this.nativeRegistry.setTheme(themeContent);
    } catch (err) {
      throw new Error(`Failed to set theme: ${err}`);
    }
  }

  /**
   * Get the color map from the current theme
   * Returns the color palette used by the theme
   * @returns Array of color strings in hex format
   */
  getColorMap(): string[] {
    try {
      return this.nativeRegistry.getColorMap();
    } catch (err) {
      throw new Error(`Failed to get color map: ${err}`);
    }
  }
}

/**
 * Low-level Grammar API
 * Handles tokenization for a specific language grammar
 */
export class Grammar implements IGrammar {
  private nativeGrammar: any;

  constructor(nativeGrammar: any) {
    this.nativeGrammar = nativeGrammar;
  }

  /**
   * Get the scope name of this grammar
   * @returns The scope name (e.g., "source.javascript")
   */
  getScopeName(): string {
    try {
      return this.nativeGrammar.getScopeName();
    } catch (err) {
      throw new Error(`Failed to get grammar scope name: ${err}`);
    }
  }

  /**
   * Tokenize a single line of code
   * This is a low-level function that returns decoded token information
   * Use this for detailed token information including scopes
   *
   * @param lineText - The source code line to tokenize
   * @param ruleStack - The rule stack from the previous line (pass null for first line)
   * @returns TokenizeResult with tokens and the next line's rule stack
   */
  tokenizeLine(lineText: string, ruleStack: RuleStack | null = null): TokenizeResult {
    try {
      const result = this.nativeGrammar.tokenizeLine(lineText, ruleStack);
      return this.convertTokenizeResult(result);
    } catch (err) {
      throw new Error(`Failed to tokenize line: ${err}`);
    }
  }

  /**
   * Tokenize a line using compact binary format
   * More efficient than tokenizeLine for performance-critical code
   * Returns token data in a compact 32-bit format
   *
   * @param lineText - The source code line to tokenize
   * @param ruleStack - The rule stack from the previous line
   * @returns Object with compact token format and next rule stack
   */
  tokenizeLine2(lineText: string, ruleStack: RuleStack | null = null): any {
    try {
      return this.nativeGrammar.tokenizeLine2(lineText, ruleStack);
    } catch (err) {
      throw new Error(`Failed to tokenize line (compact format): ${err}`);
    }
  }

  /**
   * Internal: Convert native tokenize result to TypeScript interface
   */
  private convertTokenizeResult(nativeResult: any): TokenizeResult {
    const tokens: Token[] = [];

    if (nativeResult.tokens) {
      for (const nativeToken of nativeResult.tokens) {
        tokens.push({
          startIndex: nativeToken.startIndex,
          endIndex: nativeToken.endIndex,
          scopes: nativeToken.scopes || [],
        });
      }
    }

    return {
      tokens,
      ruleStack: nativeResult.ruleStack,
      stoppedEarly: nativeResult.stoppedEarly || false,
    };
  }
}

/**
 * Create a new Registry instance
 * Automatically loads the WASM module if not already loaded
 *
 * @returns Promise resolving to a new Registry instance
 */
export async function createRegistry(): Promise<Registry> {
  const module = await loadTextMateModule();
  const nativeRegistry = new module.Registry();
  return new Registry(nativeRegistry);
}

/**
 * Create a Grammar from an existing registry
 * @param registry - The Registry instance
 * @param grammarContent - JSON string of the grammar
 * @param scopeName - Scope name for the grammar
 * @returns Grammar instance
 */
export function createGrammar(
  registry: Registry,
  grammarContent: string,
  scopeName: string
): Grammar {
  return registry.loadGrammarFromContent(grammarContent, scopeName);
}
