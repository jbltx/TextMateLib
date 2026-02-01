'use strict';

/**
 * Represents a TextMate grammar for syntax highlighting
 */
class Grammar {
    /**
     * Creates a Grammar wrapper for a native grammar handle
     * @param module The WASM module
     * @param handle The native grammar handle
     * @internal
     */
    constructor(module, handle) {
        this.native = new module.Grammar(handle);
    }
    /**
     * Tokenize a single line of text
     * @param line The line text to tokenize
     * @param prevState The rule stack from the previous line (null for first line)
     * @returns The tokenization result with tokens and rule stack for next line
     */
    tokenizeLine(line, prevState = null) {
        return this.native.tokenizeLine(line, prevState);
    }
    /**
     * Tokenize a single line of text with binary format
     * @param line The line text to tokenize
     * @param prevState The rule stack from the previous line (null for first line)
     * @returns The tokenization result with binary tokens and rule stack
     */
    tokenizeLine2(line, prevState = null) {
        return this.native.tokenizeLine2(line, prevState);
    }
    /**
     * Get the scope name of this grammar
     * @returns The scope name (e.g., "source.js")
     */
    getScopeName() {
        return this.native.getScopeName();
    }
}

/**
 * Registry for managing TextMate grammars and themes
 */
class Registry {
    /**
     * Creates a new Registry
     * @param module The WASM module
     * @internal
     */
    constructor(module) {
        this.module = module;
        this.native = new module.Registry();
    }
    /**
     * Load a grammar from JSON content
     * @param scopeName The scope name for the grammar (e.g., "source.js")
     * @param content The grammar JSON content
     * @returns The loaded Grammar or null if loading failed
     */
    loadGrammar(scopeName, content) {
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
    setTheme(themeJson) {
        return this.native.setTheme(themeJson);
    }
    /**
     * Get the color map from the current theme
     * @returns Array of color strings
     */
    getColorMap() {
        return this.native.getColorMap();
    }
}

/**
 * Main entry point for TextMate syntax highlighting
 */
class TextMate {
    /**
     * Private constructor - use TextMate.create() instead
     * @param module The initialized WASM module
     */
    constructor(module) {
        this.module = module;
        this.registry = new Registry(module);
    }
    /**
     * Create a new TextMate instance
     * @returns A promise that resolves to an initialized TextMate instance
     */
    static async create() {
        // Dynamic import of the WASM module
        const createModule = await import('../wasm/tml-standard.js');
        const module = await createModule.default();
        return new TextMate(module);
    }
    /**
     * Get the grammar registry
     * @returns The Registry instance
     */
    getRegistry() {
        return this.registry;
    }
    /**
     * Load a grammar from JSON content
     * @param scopeName The scope name for the grammar (e.g., "source.js")
     * @param content The grammar JSON content
     * @returns The loaded Grammar or null if loading failed
     */
    async loadGrammar(scopeName, content) {
        return this.registry.loadGrammar(scopeName, content);
    }
    /**
     * Set the theme from JSON content
     * @param themeJson The theme JSON content
     * @returns true if the theme was set successfully
     */
    setTheme(themeJson) {
        return this.registry.setTheme(themeJson);
    }
    /**
     * Get the color map from the current theme
     * @returns Array of color strings
     */
    getColorMap() {
        return this.registry.getColorMap();
    }
}

exports.Grammar = Grammar;
exports.Registry = Registry;
exports.TextMate = TextMate;
//# sourceMappingURL=index.cjs.map
