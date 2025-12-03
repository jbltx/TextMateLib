/**
 * High-level convenience API for TextMateLib
 * Combines grammar, theme, and tokenization into a simple interface
 * Use this for most common use cases
 */

import { Registry, Grammar } from './low-level';
import { loadTextMateModule } from './loader';
import type {
  ISyntaxHighlighter,
  HighlightedToken,
  SyntaxHighlighterOptions,
  RuleStack,
} from './types';

/**
 * Simple theme color lookup structure
 */
interface ThemeColor {
  foreground?: string;
  background?: string;
  fontStyle?: string;
}

/**
 * High-level SyntaxHighlighter API
 * Provides simple highlight() method for full document highlighting
 */
export class SyntaxHighlighter implements ISyntaxHighlighter {
  private registry: Registry;
  private grammar: Grammar;
  private colorMap: string[] = [];
  private themeScopeRules: Array<{
    scope: string[];
    settings: ThemeColor;
  }> = [];

  /**
   * Create a new SyntaxHighlighter instance
   * @param registry - The Registry instance
   * @param grammar - The Grammar instance for tokenization
   */
  constructor(registry: Registry, grammar: Grammar) {
    this.registry = registry;
    this.grammar = grammar;
  }

  /**
   * Set the active theme from JSON content
   * This updates the color information used for styling tokens
   * @param themeContent - Raw JSON string of the theme definition
   */
  setTheme(themeContent: string): void {
    try {
      this.registry.setTheme(themeContent);
      this.colorMap = this.registry.getColorMap();

      // Parse theme to build scope->color mapping
      const themeObj = JSON.parse(themeContent);
      this.themeScopeRules = [];

      if (themeObj.tokenColors) {
        for (const rule of themeObj.tokenColors) {
          const scope = rule.scope;
          const settings = rule.settings || {};

          // Normalize scope to array
          const scopes = Array.isArray(scope) ? scope : [scope];

          for (const s of scopes) {
            this.themeScopeRules.push({
              scope: s.split('.'),
              settings: {
                foreground: settings.foreground,
                background: settings.background,
                fontStyle: settings.fontStyle,
              },
            });
          }
        }
      }
    } catch (err) {
      throw new Error(`Failed to set theme: ${err}`);
    }
  }

  /**
   * Change the active grammar
   * @param grammarContent - Raw JSON string of the new grammar
   * @param scopeName - Scope name for the grammar
   */
  setGrammar(grammarContent: string, scopeName: string): void {
    try {
      this.grammar = this.registry.loadGrammarFromContent(grammarContent, scopeName);
    } catch (err) {
      throw new Error(`Failed to set grammar: ${err}`);
    }
  }

  /**
   * Highlight code using the current grammar and theme
   * Tokenizes all lines and applies theme colors to each token
   *
   * @param code - Source code to highlight (can be multiple lines)
   * @returns 2D array where [lineIndex][tokenIndex] is a HighlightedToken with colors
   */
  highlight(code: string): HighlightedToken[][] {
    const lines = code.split('\n');
    const result: HighlightedToken[][] = [];
    let ruleStack: RuleStack | null = null;

    for (const line of lines) {
      const lineTokens = this.highlightLine(line, ruleStack);
      result.push(lineTokens.tokens);
      ruleStack = lineTokens.ruleStack;
    }

    return result;
  }

  /**
   * Highlight a single line
   * Internal method that maintains state for multi-line highlighting
   */
  private highlightLine(
    lineText: string,
    ruleStack: RuleStack | null
  ): { tokens: HighlightedToken[]; ruleStack: RuleStack } {
    const tokenizeResult = this.grammar.tokenizeLine(lineText, ruleStack);
    const highlightedTokens: HighlightedToken[] = [];

    for (const token of tokenizeResult.tokens) {
      const highlighted: HighlightedToken = {
        startIndex: token.startIndex,
        endIndex: token.endIndex,
        scopes: token.scopes,
      };

      // Apply theme colors based on scopes
      const color = this.getColorForScopes(token.scopes);
      if (color.foreground) highlighted.foreground = color.foreground;
      if (color.background) highlighted.background = color.background;
      if (color.fontStyle) highlighted.fontStyle = color.fontStyle;

      highlightedTokens.push(highlighted);
    }

    return {
      tokens: highlightedTokens,
      ruleStack: tokenizeResult.ruleStack,
    };
  }

  /**
   * Find the best matching theme color for a scope path
   * Prefers more specific matches (longer scope paths)
   */
  private getColorForScopes(scopes: string[]): ThemeColor {
    let bestMatch: ThemeColor | null = null;
    let bestMatchDepth = 0;

    for (const rule of this.themeScopeRules) {
      // Check if this rule matches the scope path
      const depth = this.scopeMatchDepth(scopes, rule.scope);
      if (depth > bestMatchDepth) {
        bestMatch = rule.settings;
        bestMatchDepth = depth;
      }
    }

    return bestMatch || {};
  }

  /**
   * Calculate match depth for scope matching
   * Returns the number of matching scope components
   * Returns 0 if no match
   */
  private scopeMatchDepth(tokenScopes: string[], ruleScope: string[]): number {
    // Simple matching: check if token scopes contain rule scope
    // More specific matches (longer matching chains) rank higher
    let depth = 0;

    for (const ruleComponent of ruleScope) {
      let found = false;
      for (const tokenComponent of tokenScopes) {
        if (tokenComponent === ruleComponent || tokenComponent.startsWith(ruleComponent + '.')) {
          found = true;
          depth++;
          break;
        }
      }
      if (!found) {
        break;
      }
    }

    return depth;
  }
}

/**
 * Create a SyntaxHighlighter instance with grammar and theme
 * This is the main entry point for the high-level API
 *
 * @param options - Configuration with grammar, theme, and scope name
 * @returns Promise resolving to an initialized SyntaxHighlighter
 */
export async function createSyntaxHighlighter(
  options: SyntaxHighlighterOptions
): Promise<SyntaxHighlighter> {
  // Load WASM module if needed
  await loadTextMateModule();

  // Create registry and load grammar
  const registry = new Registry((await loadTextMateModule()).Registry);
  const grammar = registry.loadGrammarFromContent(options.grammarContent, options.scopeName);

  // Create highlighter
  const highlighter = new SyntaxHighlighter(registry, grammar);

  // Set theme
  highlighter.setTheme(options.themeContent);

  return highlighter;
}
