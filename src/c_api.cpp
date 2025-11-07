#include "c_api.h"
#include "main.h"
#include "parseRawGrammar.h"
#include "theme.h"
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

// Parse JSON theme and create Theme object
static Theme* parseJsonTheme(const std::string& jsonContent) {
    try {
        Document doc;
        doc.Parse(jsonContent.c_str());

        if (doc.HasParseError()) {
            return nullptr;
        }

        // Create IRawTheme from JSON
        auto rawTheme = new IRawTheme();

        // Get theme name
        if (doc.HasMember("name") && doc["name"].IsString()) {
            rawTheme->name = new std::string(doc["name"].GetString());
        }

        // Parse settings array
        if (doc.HasMember("settings") && doc["settings"].IsArray()) {
            const auto& settingsArray = doc["settings"];

            for (const auto& settingObj : settingsArray.GetArray()) {
                if (!settingObj.IsObject()) {
                    continue;
                }

                auto setting = new IRawThemeSetting();

                // Get name
                if (settingObj.HasMember("name") && settingObj["name"].IsString()) {
                    setting->name = new std::string(settingObj["name"].GetString());
                }

                // Get scope(s)
                if (settingObj.HasMember("scope")) {
                    const auto& scopeVal = settingObj["scope"];
                    if (scopeVal.IsString()) {
                        setting->scopeString = scopeVal.GetString();
                        auto scopeVec = new std::vector<std::string>();
                        scopeVec->push_back(setting->scopeString);
                        setting->scope = scopeVec;
                    } else if (scopeVal.IsArray()) {
                        setting->scope = new std::vector<std::string>();
                        for (const auto& scopeStr : scopeVal.GetArray()) {
                            if (scopeStr.IsString()) {
                                setting->scope->push_back(scopeStr.GetString());
                            }
                        }
                    }
                }

                // Get settings object
                if (settingObj.HasMember("settings") && settingObj["settings"].IsObject()) {
                    const auto& settingsObj = settingObj["settings"];

                    if (settingsObj.HasMember("fontStyle") && settingsObj["fontStyle"].IsString()) {
                        setting->settings.fontStyle = new std::string(settingsObj["fontStyle"].GetString());
                    }
                    if (settingsObj.HasMember("foreground") && settingsObj["foreground"].IsString()) {
                        setting->settings.foreground = new std::string(settingsObj["foreground"].GetString());
                    }
                    if (settingsObj.HasMember("background") && settingsObj["background"].IsString()) {
                        setting->settings.background = new std::string(settingsObj["background"].GetString());
                    }
                }

                rawTheme->settings.push_back(setting);
            }
        }

        // Create Theme from raw theme
        Theme* theme = Theme::createFromRawTheme(rawTheme);
        delete rawTheme;

        return theme;
    } catch (...) {
        return nullptr;
    }
}

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

        Theme* theme = parseJsonTheme(content);
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
        Theme* theme = parseJsonTheme(jsonContent);
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
    if (!theme || !scopePath || !scopePath[0]) {
        return defaultColor;
    }

    try {
        auto managed = static_cast<ManagedTheme*>(theme);

        // For now, just return the default foreground color
        // TODO: implement proper scope matching with theme trie
        if (managed->defaults) {
            auto colorMap = managed->theme->getColorMap();
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
    if (!theme || !scopePath || !scopePath[0]) {
        return defaultColor;
    }

    try {
        auto managed = static_cast<ManagedTheme*>(theme);

        // For now, just return the default background color
        // TODO: implement proper scope matching with theme trie
        if (managed->defaults) {
            auto colorMap = managed->theme->getColorMap();
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
    if (!theme || !scopePath || !scopePath[0]) {
        return defaultStyle;
    }

    try {
        auto managed = static_cast<ManagedTheme*>(theme);

        // For now, just return the default font style
        // TODO: implement proper scope matching with theme trie
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

// Get scope name from grammar
const char* textmate_grammar_get_scope_name(TextMateGrammar grammar) {
    // Not implemented yet - would require adding a getter method to Grammar class
    // For now, return nullptr
    return nullptr;
}

// Dispose grammar (Note: Grammar lifecycle is managed by Registry in the C++ implementation)
void textmate_grammar_dispose(TextMateGrammar grammar) {
    // In the current C++ implementation, Grammar objects are owned by Registry
    // so we don't delete them here. This function is provided for API consistency
    // but doesn't do anything. If you want independent Grammar lifecycle,
    // you'll need to modify the C++ implementation.
}

// Dispose Oniguruma library
void textmate_oniglib_dispose(TextMateOnigLib onigLib) {
    if (onigLib) {
        IOnigLib* lib = static_cast<IOnigLib*>(onigLib);
        delete lib;
    }
}
