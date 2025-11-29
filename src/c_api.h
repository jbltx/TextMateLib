#ifndef TEXTMATELIB_C_API_H
#define TEXTMATELIB_C_API_H

#include "tml_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Opaque handle types
typedef void* TextMateRegistry;
typedef void* TextMateGrammar;
typedef void* TextMateStateStack;
typedef void* TextMateOnigLib;
typedef void* TextMateTheme;

// Token structure for marshalling
typedef struct {
    int32_t startIndex;
    int32_t endIndex;
    int32_t scopeDepth;
    char** scopes;  // Array of scope strings
} TextMateToken;

// Tokenize result structure
typedef struct {
    TextMateToken* tokens;
    int32_t tokenCount;
    TextMateStateStack ruleStack;
    int32_t stoppedEarly;
} TextMateTokenizeResult;

// Tokenize result for encoded tokens (tokenizeLine2)
typedef struct {
    uint32_t* tokens;  // Encoded token array
    int32_t tokenCount;
    TextMateStateStack ruleStack;
    int32_t stoppedEarly;
} TextMateTokenizeResult2;

// ============================================================================
// Theme API
// ============================================================================

// Load theme from JSON file
// Returns nullptr on error
TML_API TextMateTheme textmate_theme_load_from_file(
    const char* themePath
);

// Load theme from JSON string
// Returns nullptr on error
TML_API TextMateTheme textmate_theme_load_from_json(
    const char* jsonContent
);

// Get foreground color for a scope path
// Returns defaultColor if scope not found
// Color format: 0xRRGGBBAA (e.g., 0xFF0000FF for opaque red)
TML_API uint32_t textmate_theme_get_foreground(
    TextMateTheme theme,
    const char* scopePath,
    uint32_t defaultColor
);

// Get background color for a scope path
// Returns defaultColor if scope not found
TML_API uint32_t textmate_theme_get_background(
    TextMateTheme theme,
    const char* scopePath,
    uint32_t defaultColor
);

// Get font style flags for a scope
// Returns defaultStyle if scope not found
// Font style constants:
#define TEXTMATE_FONT_STYLE_NONE      0
#define TEXTMATE_FONT_STYLE_ITALIC    1
#define TEXTMATE_FONT_STYLE_BOLD      2
#define TEXTMATE_FONT_STYLE_UNDERLINE 4

TML_API int32_t textmate_theme_get_font_style(
    TextMateTheme theme,
    const char* scopePath,
    int32_t defaultStyle
);

// Get default foreground color for the theme
TML_API uint32_t textmate_theme_get_default_foreground(TextMateTheme theme);

// Get default background color for the theme
TML_API uint32_t textmate_theme_get_default_background(TextMateTheme theme);

// Dispose theme
TML_API void textmate_theme_dispose(TextMateTheme theme);

// ============================================================================
// Registry and Grammar API
// ============================================================================

// Initialize Oniguruma library
TML_API TextMateOnigLib textmate_oniglib_create();

// Create registry with Oniguruma library
TML_API TextMateRegistry textmate_registry_create(TextMateOnigLib onigLib);

// Dispose registry
TML_API void textmate_registry_dispose(TextMateRegistry registry);

// Add grammar to registry from JSON file (does not return Grammar, just registers it)
TML_API int textmate_registry_add_grammar_from_file(
    TextMateRegistry registry,
    const char* grammarPath
);

// Add grammar to registry from JSON string (does not return Grammar, just registers it)
TML_API int textmate_registry_add_grammar_from_json(
    TextMateRegistry registry,
    const char* jsonContent
);

// Set grammar injections for a scope (call before loading the grammar)
TML_API void textmate_registry_set_injections(
    TextMateRegistry registry,
    const char* scopeName,
    const char** injections,
    int32_t injectionCount
);

// Load grammar by scope name (after grammars have been added to registry)
// This properly resolves dependencies and includes
TML_API TextMateGrammar textmate_registry_load_grammar(
    TextMateRegistry registry,
    const char* scopeName
);

// Get INITIAL state
TML_API TextMateStateStack textmate_get_initial_state();

// Tokenize a line of text
TML_API TextMateTokenizeResult* textmate_tokenize_line(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
);

// Tokenize a line of text with encoded tokens
TML_API TextMateTokenizeResult2* textmate_tokenize_line2(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
);

// Free tokenize result
TML_API void textmate_free_tokenize_result(TextMateTokenizeResult* result);

// Free tokenize result2
TML_API void textmate_free_tokenize_result2(TextMateTokenizeResult2* result);

// Batch tokenize multiple lines (Phase 2 optimization)
typedef struct {
    TextMateTokenizeResult** lineResults;  // Array of results per line
    int32_t lineCount;
} TextMateTokenizeMultiLinesResult;

// Tokenize multiple lines in a single call (reduces PInvoke overhead)
TML_API TextMateTokenizeMultiLinesResult* textmate_tokenize_lines(
    TextMateGrammar grammar,
    const char** lines,          // Array of line strings
    int32_t lineCount,           // Number of lines
    TextMateStateStack initialState
);

// Free batch tokenize result
TML_API void textmate_free_tokenize_lines_result(TextMateTokenizeMultiLinesResult* result);

// Get scope name from grammar
TML_API const char* textmate_grammar_get_scope_name(TextMateGrammar grammar);

// Dispose Oniguruma library
TML_API void textmate_oniglib_dispose(TextMateOnigLib onigLib);

#ifdef __cplusplus
}
#endif

#endif // TEXTMATELIB_C_API_H
