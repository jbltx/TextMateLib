import type { WasmModule } from './types';
import { Registry } from './Registry';
import { Grammar } from './Grammar';

/**
 * Main entry point for TextMate syntax highlighting
 */
export class TextMate {
  private module: WasmModule;
  private registry: Registry;

  /**
   * Private constructor - use TextMate.create() instead
   * @param module The initialized WASM module
   */
  private constructor(module: WasmModule) {
    this.module = module;
    this.registry = new Registry(module);
  }

  /**
   * Create a new TextMate instance
   * @returns A promise that resolves to an initialized TextMate instance
   */
  static async create(): Promise<TextMate> {
    // Dynamic import of the WASM module
    const createModule = await import('../../wasm/tml-standard.js');
    const module = await createModule.default() as WasmModule;
    return new TextMate(module);
  }

  /**
   * Get the grammar registry
   * @returns The Registry instance
   */
  getRegistry(): Registry {
    return this.registry;
  }

  /**
   * Load a grammar from JSON content
   * @param scopeName The scope name for the grammar (e.g., "source.js")
   * @param content The grammar JSON content
   * @returns The loaded Grammar or null if loading failed
   */
  async loadGrammar(scopeName: string, content: string): Promise<Grammar | null> {
    return this.registry.loadGrammar(scopeName, content);
  }

  /**
   * Set the theme from JSON content
   * @param themeJson The theme JSON content
   * @returns true if the theme was set successfully
   */
  setTheme(themeJson: string): boolean {
    return this.registry.setTheme(themeJson);
  }

  /**
   * Get the color map from the current theme
   * @returns Array of color strings
   */
  getColorMap(): string[] {
    return this.registry.getColorMap();
  }
}
