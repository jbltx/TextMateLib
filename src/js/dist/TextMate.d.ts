import { Registry } from './Registry';
import { Grammar } from './Grammar';
/**
 * Main entry point for TextMate syntax highlighting
 */
export declare class TextMate {
    private module;
    private registry;
    /**
     * Private constructor - use TextMate.create() instead
     * @param module The initialized WASM module
     */
    private constructor();
    /**
     * Create a new TextMate instance
     * @returns A promise that resolves to an initialized TextMate instance
     */
    static create(): Promise<TextMate>;
    /**
     * Get the grammar registry
     * @returns The Registry instance
     */
    getRegistry(): Registry;
    /**
     * Load a grammar from JSON content
     * @param scopeName The scope name for the grammar (e.g., "source.js")
     * @param content The grammar JSON content
     * @returns The loaded Grammar or null if loading failed
     */
    loadGrammar(scopeName: string, content: string): Promise<Grammar | null>;
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
