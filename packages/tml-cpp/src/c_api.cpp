#include "c_api.h"
#include "main.h"
#include "parseRawGrammar.h"
#include "parseRawTheme.h"
#include "theme.h"
#include "utf16_utils.h"
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cctype>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

using namespace tml;
using namespace rapidjson;

// Helper function to read file contents
static std::string readFileContents(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to convert std::string to C string (caller must free)
static char* stringToCString(const std::string& str) {
    char* cstr = new char[str.length() + 1];
    std::strcpy(cstr, str.c_str());
    return cstr;
}

// ============================================================================
// Theme Helper Functions
// ============================================================================

// Convert hex color string (#RRGGBB or #RRGGBBAA) to uint32_t (0xRRGGBBAA)
static uint32_t hexColorToUint32(const std::string& hexColor) {
    if (hexColor.empty() || hexColor[0] != '#') {
        return 0;
    }

    std::string hex = hexColor.substr(1);

    // Handle 6-char (#RRGGBB) - add full opacity
    if (hex.length() == 6) {
        hex += "FF";
    }
    // Handle 8-char (#RRGGBBAA) - convert to RGBA format
    else if (hex.length() != 8) {
        return 0;
    }

    try {
        uint32_t value = std::stoul(hex, nullptr, 16);
        // Convert from #RRGGBBAA to 0xRRGGBBAA
        return value;
    } catch (...) {
        return 0;
    }
}

// Convert font style string to flags
static int32_t fontStyleStringToFlags(const std::string& fontStyle) {
    int32_t flags = TEXTMATE_FONT_STYLE_NONE;

    if (fontStyle.find("italic") != std::string::npos) {
        flags |= TEXTMATE_FONT_STYLE_ITALIC;
    }
    if (fontStyle.find("bold") != std::string::npos) {
        flags |= TEXTMATE_FONT_STYLE_BOLD;
    }
    if (fontStyle.find("underline") != std::string::npos) {
        flags |= TEXTMATE_FONT_STYLE_UNDERLINE;
    }

    return flags;
}

// Helper to free ScopeStack linked list created by ScopeStack::from()
static void freeScopeStack(ScopeStack* stack) {
    while (stack) {
        ScopeStack* parent = stack->parent;
        delete stack;
        stack = parent;
    }
}

// Parse space-separated scope path into vector of scope names
static std::vector<ScopeName> parseScopePath(const char* scopePath) {
    std::vector<ScopeName> scopes;
    if (!scopePath || !scopePath[0]) {
        return scopes;
    }

    std::istringstream iss(scopePath);
    std::string scope;
    while (iss >> scope) {
        scopes.push_back(scope);
    }
    return scopes;
}

// Match all scopes in the stack against the theme and merge results
// This iterates from outermost to innermost scope, with inner scopes overwriting outer ones
static StyleAttributes* matchAllScopes(Theme* theme, const std::vector<ScopeName>& scopes) {
    if (scopes.empty()) {
        return nullptr;
    }

    StyleAttributes* merged = nullptr;

    // Iterate through all scopes from outermost to innermost
    // Build a progressively deeper scope stack for each match
    for (size_t i = 0; i < scopes.size(); i++) {
        // Create a scope stack up to this scope
        ScopeStack* scopeStack = nullptr;
        for (size_t j = 0; j <= i; j++) {
            scopeStack = new ScopeStack(scopeStack, scopes[j]);
        }

        // Match against theme
        StyleAttributes* attrs = theme->match(scopeStack);

        // Clean up scope stack
        freeScopeStack(scopeStack);

        // Merge results
        if (attrs) {
            if (!merged) {
                merged = attrs;
            } else {
                // Inner scope attributes override outer ones (if set)
                if (attrs->fontStyle != static_cast<int>(FontStyle::NotSet)) {
                    merged->fontStyle = attrs->fontStyle;
                }
                if (attrs->foregroundId != 0) {
                    merged->foregroundId = attrs->foregroundId;
                }
                if (attrs->backgroundId != 0) {
                    merged->backgroundId = attrs->backgroundId;
                }
                delete attrs;
            }
        }
    }

    return merged;
}

// Parse JSON theme and create Theme object
// Use the shared parseJSONTheme from parseRawTheme.h
// (Implementation moved to parseRawTheme.cpp to avoid duplication)

// ============================================================================
// Theme C API Implementation
// ============================================================================

TextMateTheme textmate_theme_load_from_file(const char* themePath) {
    if (!themePath) {
        return nullptr;
    }

    try {
        std::string content = readFileContents(themePath);
        if (content.empty()) {
            return nullptr;
        }

        Theme* theme = parseJSONTheme(content);
        if (!theme) {
            return nullptr;
        }

        StyleAttributes* defaults = theme->getDefaults();
        if (!defaults) {
            delete theme;
            return nullptr;
        }

        auto managed = new ManagedTheme(theme, defaults);
        return static_cast<TextMateTheme>(managed);
    } catch (...) {
        return nullptr;
    }
}

TextMateTheme textmate_theme_load_from_json(const char* jsonContent) {
    if (!jsonContent) {
        return nullptr;
    }

    try {
        Theme* theme = parseJSONTheme(jsonContent);
        if (!theme) {
            return nullptr;
        }

        StyleAttributes* defaults = theme->getDefaults();
        if (!defaults) {
            delete theme;
            return nullptr;
        }

        auto managed = new ManagedTheme(theme, defaults);
        return static_cast<TextMateTheme>(managed);
    } catch (...) {
        return nullptr;
    }
}

uint32_t textmate_theme_get_foreground(
    TextMateTheme theme,
    const char* scopePath,
    uint32_t defaultColor
) {
    if (!theme) {
        return defaultColor;
    }

    try {
        auto managed = static_cast<ManagedTheme*>(theme);
        auto colorMap = managed->theme->getColorMap();

        // If no scope path provided, return default foreground
        if (!scopePath || !scopePath[0]) {
            if (managed->defaults) {
                int fgId = managed->defaults->foregroundId;
                if (fgId > 0 && fgId < static_cast<int>(colorMap.size())) {
                    return hexColorToUint32(colorMap[fgId]);
                }
            }
            return defaultColor;
        }

        // Parse scope path
        std::vector<ScopeName> scopes = parseScopePath(scopePath);
        if (scopes.empty()) {
            if (managed->defaults) {
                int fgId = managed->defaults->foregroundId;
                if (fgId > 0 && fgId < static_cast<int>(colorMap.size())) {
                    return hexColorToUint32(colorMap[fgId]);
                }
            }
            return defaultColor;
        }

        // Match all scopes against theme and merge results
        StyleAttributes* attrs = matchAllScopes(managed->theme, scopes);

        // Get color from matched attributes
        if (attrs) {
            int fgId = attrs->foregroundId;
            delete attrs;

            if (fgId > 0 && fgId < static_cast<int>(colorMap.size())) {
                return hexColorToUint32(colorMap[fgId]);
            }
        }

        // Fall back to default foreground
        if (managed->defaults) {
            int fgId = managed->defaults->foregroundId;
            if (fgId > 0 && fgId < static_cast<int>(colorMap.size())) {
                return hexColorToUint32(colorMap[fgId]);
            }
        }

        return defaultColor;
    } catch (...) {
        return defaultColor;
    }
}

uint32_t textmate_theme_get_background(
    TextMateTheme theme,
    const char* scopePath,
    uint32_t defaultColor
) {
    if (!theme) {
        return defaultColor;
    }

    try {
        auto managed = static_cast<ManagedTheme*>(theme);
        auto colorMap = managed->theme->getColorMap();

        // If no scope path provided, return default background
        if (!scopePath || !scopePath[0]) {
            if (managed->defaults) {
                int bgId = managed->defaults->backgroundId;
                if (bgId > 0 && bgId < static_cast<int>(colorMap.size())) {
                    return hexColorToUint32(colorMap[bgId]);
                }
            }
            return defaultColor;
        }

        // Parse scope path
        std::vector<ScopeName> scopes = parseScopePath(scopePath);
        if (scopes.empty()) {
            if (managed->defaults) {
                int bgId = managed->defaults->backgroundId;
                if (bgId > 0 && bgId < static_cast<int>(colorMap.size())) {
                    return hexColorToUint32(colorMap[bgId]);
                }
            }
            return defaultColor;
        }

        // Match all scopes against theme and merge results
        StyleAttributes* attrs = matchAllScopes(managed->theme, scopes);

        // Get color from matched attributes
        if (attrs) {
            int bgId = attrs->backgroundId;
            delete attrs;

            if (bgId > 0 && bgId < static_cast<int>(colorMap.size())) {
                return hexColorToUint32(colorMap[bgId]);
            }
        }

        // Fall back to default background
        if (managed->defaults) {
            int bgId = managed->defaults->backgroundId;
            if (bgId > 0 && bgId < static_cast<int>(colorMap.size())) {
                return hexColorToUint32(colorMap[bgId]);
            }
        }

        return defaultColor;
    } catch (...) {
        return defaultColor;
    }
}

int32_t textmate_theme_get_font_style(
    TextMateTheme theme,
    const char* scopePath,
    int32_t defaultStyle
) {
    if (!theme) {
        return defaultStyle;
    }

    try {
        auto managed = static_cast<ManagedTheme*>(theme);

        // If no scope path provided, return default font style
        if (!scopePath || !scopePath[0]) {
            if (managed->defaults) {
                return managed->defaults->fontStyle;
            }
            return defaultStyle;
        }

        // Parse scope path
        std::vector<ScopeName> scopes = parseScopePath(scopePath);
        if (scopes.empty()) {
            if (managed->defaults) {
                return managed->defaults->fontStyle;
            }
            return defaultStyle;
        }

        // Match all scopes against theme and merge results
        StyleAttributes* attrs = matchAllScopes(managed->theme, scopes);

        // Get font style from matched attributes
        if (attrs) {
            int fontStyle = attrs->fontStyle;
            delete attrs;

            // FontStyle::NotSet is -1, only return if explicitly set
            if (fontStyle >= 0) {
                return fontStyle;
            }
        }

        // Fall back to default font style
        if (managed->defaults) {
            return managed->defaults->fontStyle;
        }

        return defaultStyle;
    } catch (...) {
        return defaultStyle;
    }
}

uint32_t textmate_theme_get_default_foreground(TextMateTheme theme) {
    if (!theme) {
        return 0xFFFFFFFF;  // White
    }

    try {
        auto managed = static_cast<ManagedTheme*>(theme);
        if (managed->defaults) {
            auto colorMap = managed->theme->getColorMap();
            int fgId = managed->defaults->foregroundId;

            if (fgId > 0 && fgId < static_cast<int>(colorMap.size())) {
                return hexColorToUint32(colorMap[fgId]);
            }
        }
        return 0xFFFFFFFF;  // White fallback
    } catch (...) {
        return 0xFFFFFFFF;
    }
}

uint32_t textmate_theme_get_default_background(TextMateTheme theme) {
    if (!theme) {
        return 0x000000FF;  // Black
    }

    try {
        auto managed = static_cast<ManagedTheme*>(theme);
        if (managed->defaults) {
            auto colorMap = managed->theme->getColorMap();
            int bgId = managed->defaults->backgroundId;

            if (bgId > 0 && bgId < static_cast<int>(colorMap.size())) {
                return hexColorToUint32(colorMap[bgId]);
            }
        }
        return 0x000000FF;  // Black fallback
    } catch (...) {
        return 0x000000FF;
    }
}

void textmate_theme_dispose(TextMateTheme theme) {
    if (theme) {
        auto managed = static_cast<ManagedTheme*>(theme);
        delete managed;
    }
}

// Initialize Oniguruma library
TextMateOnigLib textmate_oniglib_create() {
    try {
        IOnigLib* onigLib = new DefaultOnigLib();
        return static_cast<TextMateOnigLib>(onigLib);
    } catch (...) {
        return nullptr;
    }
}

// Helper class to manage registry with internal grammar storage
class ManagedRegistry {
public:
    Registry* registry;
    std::map<std::string, IRawGrammar*> preloadedGrammars;
    std::map<std::string, std::vector<std::string>> injections;

    ManagedRegistry(IOnigLib* onigLib) {
        RegistryOptions options;
        options.onigLib = onigLib;

        // Set up loadGrammar callback to return from preloaded grammars
        options.loadGrammar = [this](const ScopeName& scopeName) -> IRawGrammar* {
            auto it = preloadedGrammars.find(scopeName);
            if (it != preloadedGrammars.end()) {
                return it->second;
            }
            return nullptr;
        };

        // Set up getInjections callback to return configured injections
        options.getInjections = [this](const ScopeName& scopeName) -> std::vector<ScopeName> {
            auto it = injections.find(scopeName);
            if (it != injections.end()) {
                return it->second;
            }
            return std::vector<ScopeName>();
        };

        registry = new Registry(options);
    }

    ~ManagedRegistry() {
        if (registry) {
            delete registry;
        }
        // Note: Don't delete preloaded grammars as they're owned by the registry now
    }
};

// Create registry with Oniguruma library
TextMateRegistry textmate_registry_create(TextMateOnigLib onigLib) {
    try {
        ManagedRegistry* managed = new ManagedRegistry(static_cast<IOnigLib*>(onigLib));
        return static_cast<TextMateRegistry>(managed);
    } catch (...) {
        return nullptr;
    }
}

// Dispose registry
void textmate_registry_dispose(TextMateRegistry registry) {
    if (registry) {
        ManagedRegistry* managed = static_cast<ManagedRegistry*>(registry);
        delete managed;
    }
}

// Add grammar to registry from JSON file (does not return Grammar, just registers it)
int textmate_registry_add_grammar_from_file(
    TextMateRegistry registry,
    const char* grammarPath
) {
    if (!registry || !grammarPath) {
        return 0;
    }

    try {
        std::string content = readFileContents(grammarPath);
        if (content.empty()) {
            return 0;
        }

        std::string pathStr = grammarPath;
        IRawGrammar* rawGrammar = parseRawGrammar(content, &pathStr);
        if (!rawGrammar) {
            return 0;
        }

        ManagedRegistry* managed = static_cast<ManagedRegistry*>(registry);
        managed->preloadedGrammars[rawGrammar->scopeName] = rawGrammar;

        return 1; // Success
    } catch (...) {
        return 0;
    }
}

// Add grammar to registry from JSON string (does not return Grammar, just registers it)
int textmate_registry_add_grammar_from_json(
    TextMateRegistry registry,
    const char* jsonContent
) {
    if (!registry || !jsonContent) {
        return 0;
    }

    try {
        IRawGrammar* rawGrammar = parseRawGrammar(jsonContent, nullptr);
        if (!rawGrammar) {
            return 0;
        }

        ManagedRegistry* managed = static_cast<ManagedRegistry*>(registry);
        managed->preloadedGrammars[rawGrammar->scopeName] = rawGrammar;

        return 1; // Success
    } catch (...) {
        return 0;
    }
}

// Set grammar injections for a scope (call before loading the grammar)
void textmate_registry_set_injections(
    TextMateRegistry registry,
    const char* scopeName,
    const char** injections,
    int32_t injectionCount
) {
    if (!registry || !scopeName || !injections) {
        return;
    }

    try {
        ManagedRegistry* managed = static_cast<ManagedRegistry*>(registry);
        std::vector<std::string> injectionsList;
        for (int32_t i = 0; i < injectionCount; i++) {
            if (injections[i]) {
                injectionsList.push_back(injections[i]);
            }
        }
        managed->injections[scopeName] = injectionsList;
    } catch (...) {
        // Ignore errors
    }
}

// Load grammar by scope name (after grammars have been added to registry)
TextMateGrammar textmate_registry_load_grammar(
    TextMateRegistry registry,
    const char* scopeName
) {
    if (!registry || !scopeName) {
        return nullptr;
    }

    try {
        ManagedRegistry* managed = static_cast<ManagedRegistry*>(registry);
        Grammar* grammar = managed->registry->loadGrammar(scopeName);
        return static_cast<TextMateGrammar>(grammar);
    } catch (...) {
        return nullptr;
    }
}

// Get INITIAL state
TextMateStateStack textmate_get_initial_state() {
    return static_cast<TextMateStateStack>(const_cast<StateStack*>(INITIAL));
}

// Tokenize a line of text
TextMateTokenizeResult* textmate_tokenize_line(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
) {
    if (!grammar || !lineText) {
        return nullptr;
    }

    try {
        Grammar* gram = static_cast<Grammar*>(grammar);
        StateStack* state = static_cast<StateStack*>(prevState);

        ITokenizeLineResult result = gram->tokenizeLine(lineText, state);

        // Allocate result structure
        TextMateTokenizeResult* cResult = new TextMateTokenizeResult();
        cResult->tokenCount = result.tokens.size();
        cResult->tokens = new TextMateToken[cResult->tokenCount];
        cResult->ruleStack = static_cast<TextMateStateStack>(result.ruleStack);
        cResult->stoppedEarly = result.stoppedEarly ? 1 : 0;

        // Convert tokens
        for (int i = 0; i < cResult->tokenCount; i++) {
            const IToken& token = result.tokens[i];
            cResult->tokens[i].startIndex = token.startIndex;
            cResult->tokens[i].endIndex = token.endIndex;
            cResult->tokens[i].scopeDepth = token.scopes.size();

            // Allocate scope strings
            cResult->tokens[i].scopes = new char*[token.scopes.size()];
            for (size_t j = 0; j < token.scopes.size(); j++) {
                cResult->tokens[i].scopes[j] = stringToCString(token.scopes[j]);
            }
        }

        return cResult;
    } catch (...) {
        return nullptr;
    }
}

// Tokenize a line of text with encoded tokens
TextMateTokenizeResult2* textmate_tokenize_line2(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
) {
    if (!grammar || !lineText) {
        return nullptr;
    }

    try {
        Grammar* gram = static_cast<Grammar*>(grammar);
        StateStack* state = static_cast<StateStack*>(prevState);

        ITokenizeLineResult2 result = gram->tokenizeLine2(lineText, state);

        // Allocate result structure
        TextMateTokenizeResult2* cResult = new TextMateTokenizeResult2();
        cResult->tokenCount = result.tokens.size();
        cResult->tokens = new uint32_t[cResult->tokenCount];
        cResult->ruleStack = static_cast<TextMateStateStack>(result.ruleStack);
        cResult->stoppedEarly = result.stoppedEarly ? 1 : 0;

        // Copy tokens
        for (int i = 0; i < cResult->tokenCount; i++) {
            cResult->tokens[i] = result.tokens[i];
        }

        return cResult;
    } catch (...) {
        return nullptr;
    }
}

// Free tokenize result
void textmate_free_tokenize_result(TextMateTokenizeResult* result) {
    if (result) {
        if (result->tokens) {
            for (int i = 0; i < result->tokenCount; i++) {
                if (result->tokens[i].scopes) {
                    for (int j = 0; j < result->tokens[i].scopeDepth; j++) {
                        delete[] result->tokens[i].scopes[j];
                    }
                    delete[] result->tokens[i].scopes;
                }
            }
            delete[] result->tokens;
        }
        delete result;
    }
}

// Free tokenize result2
void textmate_free_tokenize_result2(TextMateTokenizeResult2* result) {
    if (result) {
        if (result->tokens) {
            delete[] result->tokens;
        }
        delete result;
    }
}

// Batch tokenize multiple lines (Phase 2 optimization)
TextMateTokenizeMultiLinesResult* textmate_tokenize_lines(
    TextMateGrammar grammar,
    const char** lines,
    int32_t lineCount,
    TextMateStateStack initialState
) {
    if (!grammar || !lines || lineCount <= 0) {
        return nullptr;
    }

    try {
        Grammar* g = static_cast<Grammar*>(grammar);
        StateStack* state = static_cast<StateStack*>(initialState);

        // Allocate result structure
        TextMateTokenizeMultiLinesResult* batchResult = new TextMateTokenizeMultiLinesResult();
        batchResult->lineCount = lineCount;
        batchResult->lineResults = new TextMateTokenizeResult*[lineCount];

        // Tokenize each line, propagating state
        for (int32_t i = 0; i < lineCount; i++) {
            std::string lineText(lines[i]);
            auto result = g->tokenizeLine(lineText, state);

            // Update state for next line
            state = result.ruleStack;

            // Allocate result for this line
            TextMateTokenizeResult* lineResult = new TextMateTokenizeResult();
            lineResult->tokenCount = result.tokens.size();
            lineResult->stoppedEarly = result.stoppedEarly ? 1 : 0;
            lineResult->ruleStack = static_cast<TextMateStateStack>(result.ruleStack);

            // Allocate and populate tokens
            lineResult->tokens = new TextMateToken[lineResult->tokenCount];
            for (size_t j = 0; j < result.tokens.size(); j++) {
                const auto& token = result.tokens[j];
                lineResult->tokens[j].startIndex = token.startIndex;
                lineResult->tokens[j].endIndex = token.endIndex;
                lineResult->tokens[j].scopeDepth = token.scopes.size();

                // Allocate scope array
                lineResult->tokens[j].scopes = new char*[token.scopes.size()];
                for (size_t k = 0; k < token.scopes.size(); k++) {
                    lineResult->tokens[j].scopes[k] = stringToCString(token.scopes[k]);
                }
            }

            batchResult->lineResults[i] = lineResult;
        }

        return batchResult;
    } catch (...) {
        return nullptr;
    }
}

// Free batch tokenize result
void textmate_free_tokenize_lines_result(TextMateTokenizeMultiLinesResult* result) {
    if (result) {
        // Free each line result
        for (int32_t i = 0; i < result->lineCount; i++) {
            textmate_free_tokenize_result(result->lineResults[i]);
        }
        delete[] result->lineResults;
        delete result;
    }
}

// ============================================================================
// UTF-16 Tokenization API
// ============================================================================

// Tokenize a line of text with UTF-16 code unit indices
TextMateTokenizeResult* textmate_tokenize_line_utf16(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
) {
    if (!grammar || !lineText) {
        return nullptr;
    }

    try {
        Grammar* gram = static_cast<Grammar*>(grammar);
        StateStack* state = static_cast<StateStack*>(prevState);

        ITokenizeLineResult result = gram->tokenizeLine(lineText, state);

        // Build byte-offset to UTF-16 index map
        auto map = tml::buildByteToUtf16Map(lineText, std::strlen(lineText));

        // Allocate result structure
        TextMateTokenizeResult* cResult = new TextMateTokenizeResult();
        cResult->tokenCount = result.tokens.size();
        cResult->tokens = new TextMateToken[cResult->tokenCount];
        cResult->ruleStack = static_cast<TextMateStateStack>(result.ruleStack);
        cResult->stoppedEarly = result.stoppedEarly ? 1 : 0;

        // Convert tokens with UTF-16 indices
        // Note: the tokenizer may internally append '\n', so token indices
        // can exceed strlen(lineText). Use mapByteToUtf16 for safe lookup.
        for (int i = 0; i < cResult->tokenCount; i++) {
            const IToken& token = result.tokens[i];
            cResult->tokens[i].startIndex = tml::mapByteToUtf16(map, token.startIndex);
            cResult->tokens[i].endIndex = tml::mapByteToUtf16(map, token.endIndex);
            cResult->tokens[i].scopeDepth = token.scopes.size();

            // Allocate scope strings
            cResult->tokens[i].scopes = new char*[token.scopes.size()];
            for (size_t j = 0; j < token.scopes.size(); j++) {
                cResult->tokens[i].scopes[j] = stringToCString(token.scopes[j]);
            }
        }

        return cResult;
    } catch (...) {
        return nullptr;
    }
}

// Tokenize a line of text with encoded tokens and UTF-16 indices
TextMateTokenizeResult2* textmate_tokenize_line2_utf16(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
) {
    if (!grammar || !lineText) {
        return nullptr;
    }

    try {
        Grammar* gram = static_cast<Grammar*>(grammar);
        StateStack* state = static_cast<StateStack*>(prevState);

        ITokenizeLineResult2 result = gram->tokenizeLine2(lineText, state);

        // Build byte-offset to UTF-16 index map
        auto map = tml::buildByteToUtf16Map(lineText, std::strlen(lineText));

        // Allocate result structure
        TextMateTokenizeResult2* cResult = new TextMateTokenizeResult2();
        cResult->tokenCount = result.tokens.size();
        cResult->tokens = new uint32_t[cResult->tokenCount];
        cResult->ruleStack = static_cast<TextMateStateStack>(result.ruleStack);
        cResult->stoppedEarly = result.stoppedEarly ? 1 : 0;

        // Copy tokens, converting start offsets from UTF-8 byte to UTF-16
        // Encoded tokens are pairs: [startIndex, metadata, startIndex, metadata, ...]
        for (int i = 0; i < cResult->tokenCount; i++) {
            if (i % 2 == 0) {
                // Even indices are start offsets
                cResult->tokens[i] = tml::mapByteToUtf16(map, result.tokens[i]);
            } else {
                // Odd indices are metadata — pass through
                cResult->tokens[i] = result.tokens[i];
            }
        }

        return cResult;
    } catch (...) {
        return nullptr;
    }
}

// Batch tokenize multiple lines with UTF-16 indices
TextMateTokenizeMultiLinesResult* textmate_tokenize_lines_utf16(
    TextMateGrammar grammar,
    const char** lines,
    int32_t lineCount,
    TextMateStateStack initialState
) {
    if (!grammar || !lines || lineCount <= 0) {
        return nullptr;
    }

    try {
        Grammar* g = static_cast<Grammar*>(grammar);
        StateStack* state = static_cast<StateStack*>(initialState);

        // Allocate result structure
        TextMateTokenizeMultiLinesResult* batchResult = new TextMateTokenizeMultiLinesResult();
        batchResult->lineCount = lineCount;
        batchResult->lineResults = new TextMateTokenizeResult*[lineCount];

        // Tokenize each line, propagating state
        for (int32_t i = 0; i < lineCount; i++) {
            std::string lineText(lines[i]);
            auto result = g->tokenizeLine(lineText, state);

            // Update state for next line
            state = result.ruleStack;

            // Build byte-offset to UTF-16 index map for this line
            auto map = tml::buildByteToUtf16Map(lineText.c_str(), lineText.size());

            // Allocate result for this line
            TextMateTokenizeResult* lineResult = new TextMateTokenizeResult();
            lineResult->tokenCount = result.tokens.size();
            lineResult->stoppedEarly = result.stoppedEarly ? 1 : 0;
            lineResult->ruleStack = static_cast<TextMateStateStack>(result.ruleStack);

            // Allocate and populate tokens with UTF-16 indices
            lineResult->tokens = new TextMateToken[lineResult->tokenCount];
            for (size_t j = 0; j < result.tokens.size(); j++) {
                const auto& token = result.tokens[j];
                lineResult->tokens[j].startIndex = tml::mapByteToUtf16(map, token.startIndex);
                lineResult->tokens[j].endIndex = tml::mapByteToUtf16(map, token.endIndex);
                lineResult->tokens[j].scopeDepth = token.scopes.size();

                // Allocate scope array
                lineResult->tokens[j].scopes = new char*[token.scopes.size()];
                for (size_t k = 0; k < token.scopes.size(); k++) {
                    lineResult->tokens[j].scopes[k] = stringToCString(token.scopes[k]);
                }
            }

            batchResult->lineResults[i] = lineResult;
        }

        return batchResult;
    } catch (...) {
        return nullptr;
    }
}

// Get scope name from grammar
const char* textmate_grammar_get_scope_name(TextMateGrammar grammar) {
    if (!grammar) {
        return nullptr;
    }

    try {
        Grammar* gram = static_cast<Grammar*>(grammar);
        static thread_local std::string scopeName;
        scopeName = gram->getScopeName();
        return scopeName.c_str();
    } catch (...) {
        return nullptr;
    }
}

// Dispose Oniguruma library
void textmate_oniglib_dispose(TextMateOnigLib onigLib) {
    if (onigLib) {
        IOnigLib* lib = static_cast<IOnigLib*>(onigLib);
        delete lib;
    }
}
