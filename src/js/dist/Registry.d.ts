import type { WasmModule } from './types';
import { Grammar } from './Grammar';
/**
 * Registry for managing TextMate grammars and themes
 */
export declare class Registry {
    private module;
    private native;
    /**
     * Creates a new Registry
     * @param module The WASM module
     * @internal
     */
    constructor(module: WasmModule);
    /**
     * Load a grammar from JSON content
     * @param scopeName The scope name for the grammar (e.g., "source.js")
     * @param content The grammar JSON content
     * @returns The loaded Grammar or null if loading failed
     */
    loadGrammar(scopeName: string, content: string): Grammar | null;
    /**
     * Set the theme from JSON content
     * @param themeJson The theme JSON content
     * @returns true if the theme was set successfully
     */
    setTheme(themeJson: string): boolean;
    /**
     * Get the color map from the current theme
     * @returns Array of color strings
     */
    getColorMap(): string[];
}
