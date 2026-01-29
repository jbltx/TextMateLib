import type { WasmModule, NativeRegistry } from './types';
import { Grammar } from './Grammar';

/**
 * Registry for managing TextMate grammars and themes
 */
export class Registry {
  private module: WasmModule;
  private native: NativeRegistry;

  /**
   * Creates a new Registry
   * @param module The WASM module
   * @internal
   */
  constructor(module: WasmModule) {
    this.module = module;
    this.native = new module.Registry();
  }

  /**
   * Load a grammar from JSON content
   * @param scopeName The scope name for the grammar (e.g., "source.js")
   * @param content The grammar JSON content
   * @returns The loaded Grammar or null if loading failed
   */
  loadGrammar(scopeName: string, content: string): Grammar | null {
    const handle = this.native.loadGrammarFromContent(content, scopeName);
    if (handle === null || handle === 0) {
      return null;
    }
    return new Grammar(this.module, handle);
  }

  /**
   * Set the theme from JSON content
   * @param themeJson The theme JSON content
   * @returns true if the theme was set successfully
   */
  setTheme(themeJson: string): boolean {
    return this.native.setTheme(themeJson);
  }

  /**
   * Get the color map from the current theme
   * @returns Array of color strings
   */
  getColorMap(): string[] {
    return this.native.getColorMap();
  }
}
