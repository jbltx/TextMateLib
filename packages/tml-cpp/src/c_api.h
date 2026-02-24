#ifndef TEXTMATELIB_C_API_H
#define TEXTMATELIB_C_API_H

/// @file c_api.h
/// @brief C language API for TextMateLib
///
/// This header provides a C FFI (Foreign Function Interface) for TextMateLib,
/// enabling use from C code and language bindings (C#, Python, Node.js, etc.).
///
/// **API Organization:**
/// - **Theme API**: Loading and querying theme colors and styles
/// - **Registry & Grammar API**: Managing grammar definitions and tokenization
/// - **Tokenization API**: Core text processing with stateful line-by-line parsing
///
/// **Typical Workflow:**
/// 1. Initialize: Create registry, load grammars and themes
/// 2. Tokenize: Call tokenize_line() or tokenize_lines() with grammar and text
/// 3. Apply Styles: Use theme colors from returned scopes
/// 4. Cleanup: Dispose resources and results

#include "tml_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/// @defgroup opaque_types Opaque Handle Types
/// @{
/// Opaque pointer types for C API objects. Actual implementations are in C++,
/// handles are meant to be passed directly without inspection.

/// @brief Handle to a theme object containing color schemes
typedef void* TextMateTheme;

/// @brief Handle to a grammar definition for a specific language
typedef void* TextMateGrammar;

/// @brief Handle to a parsing state stack (immutable, used for incremental tokenization)
typedef void* TextMateStateStack;

/// @brief Handle to the Oniguruma regex library instance
typedef void* TextMateOnigLib;

/// @brief Handle to the grammar registry managing loaded grammars and themes
typedef void* TextMateRegistry;

/// @}

/// @defgroup token_structures Token and Result Structures
/// @{

/// @brief Represents a single token in tokenized text
///
/// A token maps a range of text to a scope hierarchy (list of scopes).
/// The scopes determine styling through theme matching.
struct TextMateToken {
    int32_t startIndex;     ///< Start position in the line (0-based)
    int32_t endIndex;       ///< End position (exclusive)
    int32_t scopeDepth;     ///< Number of scopes in the scope hierarchy
    char** scopes;          ///< Array of scope strings (e.g., "keyword.control", "string.quoted.double")
};

/// @brief Result from tokenizing a single line with decoded tokens
///
/// Returned by textmate_tokenize_line(). Contains the tokens and the state
/// needed to continue tokenization on the next line (for incremental updates).
///
/// **Memory Ownership:**
/// Caller must free this structure using textmate_free_tokenize_result().
struct TextMateTokenizeResult {
    TextMateToken* tokens;          ///< Array of tokens found in this line
    int32_t tokenCount;             ///< Number of tokens in the array
    TextMateStateStack ruleStack;   ///< State at end of line (pass to next line's tokenization)
    int32_t stoppedEarly;           ///< Non-zero if tokenization stopped before end (time limit hit)
};

/// @brief Result from tokenizing a single line with encoded tokens
///
/// Alternative to TextMateTokenizeResult that uses compact 32-bit token encoding
/// rather than decoded scopes. Used by textmate_tokenize_line2() for performance.
///
/// **Memory Ownership:**
/// Caller must free this structure using textmate_free_tokenize_result2().
struct TextMateTokenizeResult2 {
    uint32_t* tokens;               ///< Array of encoded tokens
    int32_t tokenCount;             ///< Number of tokens in the array
    TextMateStateStack ruleStack;   ///< State at end of line (pass to next line's tokenization)
    int32_t stoppedEarly;           ///< Non-zero if tokenization stopped before end (time limit hit)
};

/// @brief Result from batch tokenizing multiple lines
///
/// Returned by textmate_tokenize_lines(). Optimized for multi-line tokenization
/// to reduce FFI call overhead in language bindings.
///
/// **Memory Ownership:**
/// Caller must free this structure using textmate_free_tokenize_lines_result().
struct TextMateTokenizeMultiLinesResult {
    TextMateTokenizeResult** lineResults;  ///< Array of results, one per line
    int32_t lineCount;                    ///< Number of lines tokenized
};

/// @}

/// @defgroup theme_api Theme API
/// @{
/// Load and query color schemes (themes) for syntax highlighting.
/// Themes map scope hierarchies to foreground/background colors and font styles.

/// @brief Load a theme from a JSON file
/// @param themePath Path to the theme JSON file (TextMate theme format)
/// @return Opaque theme handle on success, NULL on error (file not found, invalid JSON, etc.)
/// @note The returned theme must be disposed with textmate_theme_dispose()
TML_API TextMateTheme textmate_theme_load_from_file(
    const char* themePath
);

/// @brief Load a theme from a JSON string
/// @param jsonContent Theme JSON content as a null-terminated string
/// @return Opaque theme handle on success, NULL on error (invalid JSON)
/// @note The returned theme must be disposed with textmate_theme_dispose()
TML_API TextMateTheme textmate_theme_load_from_json(
    const char* jsonContent
);

/// @brief Get the foreground color for a scope path
/// @param theme Valid theme handle (from textmate_theme_load_*)
/// @param scopePath Scope path to match (e.g., "source.js keyword.control", "string.quoted.double")
/// @param defaultColor Color to return if scope is not found in theme
/// @return RGBA color value (0xRRGGBBAA format, e.g., 0xFF0000FF for opaque red)
/// @note Scope matching uses TextMate's scope selector rules
/// @see textmate_theme_get_background(), textmate_theme_get_font_style()
TML_API uint32_t textmate_theme_get_foreground(
    TextMateTheme theme,
    const char* scopePath,
    uint32_t defaultColor
);

/// @brief Get the background color for a scope path
/// @param theme Valid theme handle (from textmate_theme_load_*)
/// @param scopePath Scope path to match
/// @param defaultColor Color to return if scope is not found in theme
/// @return RGBA color value (0xRRGGBBAA format)
/// @see textmate_theme_get_foreground()
TML_API uint32_t textmate_theme_get_background(
    TextMateTheme theme,
    const char* scopePath,
    uint32_t defaultColor
);

/// @brief Font style flag constants for textmate_theme_get_font_style()
/// @{
#define TEXTMATE_FONT_STYLE_NONE      0  ///< No special styling
#define TEXTMATE_FONT_STYLE_ITALIC    1  ///< Italic text
#define TEXTMATE_FONT_STYLE_BOLD      2  ///< Bold text
#define TEXTMATE_FONT_STYLE_UNDERLINE 4  ///< Underlined text
/// @}

/// @brief Get the font style flags for a scope path
/// @param theme Valid theme handle (from textmate_theme_load_*)
/// @param scopePath Scope path to match
/// @param defaultStyle Font style flags to return if scope is not found
/// @return Combination of TEXTMATE_FONT_STYLE_* flags
/// @note Flags can be combined with bitwise OR (e.g., BOLD | ITALIC)
/// @see textmate_theme_get_foreground()
TML_API int32_t textmate_theme_get_font_style(
    TextMateTheme theme,
    const char* scopePath,
    int32_t defaultStyle
);

/// @brief Get the default/fallback foreground color for the entire theme
/// @param theme Valid theme handle (from textmate_theme_load_*)
/// @return RGBA color value (0xRRGGBBAA format)
/// @note Used when no matching scope is found in the theme
TML_API uint32_t textmate_theme_get_default_foreground(TextMateTheme theme);

/// @brief Get the default/fallback background color for the entire theme
/// @param theme Valid theme handle (from textmate_theme_load_*)
/// @return RGBA color value (0xRRGGBBAA format)
/// @note Used when no matching scope is found in the theme
TML_API uint32_t textmate_theme_get_default_background(TextMateTheme theme);

/// @brief Free a theme object and release resources
/// @param theme Valid theme handle (from textmate_theme_load_*), or NULL (no-op)
/// @warning Do not use theme after calling this function
/// @note Safe to call with NULL
TML_API void textmate_theme_dispose(TextMateTheme theme);

/// @}

/// @defgroup registry_api Registry and Grammar API
/// @{
/// Manage grammar definitions, handle dependencies, and perform tokenization.
/// The registry is the central component for working with multiple grammars and themes.

/// @brief Initialize the Oniguruma regular expression library
/// @return Opaque Oniguruma library handle on success, NULL on error
/// @note This must be created before creating a registry
/// @note The returned handle must be disposed with textmate_oniglib_dispose()
TML_API TextMateOnigLib textmate_oniglib_create();

/// @brief Create a new grammar registry
/// @param onigLib Valid Oniguruma library handle (from textmate_oniglib_create())
/// @return Registry handle on success, NULL on error
/// @note The registry must be disposed with textmate_registry_dispose()
/// @see textmate_oniglib_create()
TML_API TextMateRegistry textmate_registry_create(TextMateOnigLib onigLib);

/// @brief Free a registry and all its resources
/// @param registry Valid registry handle (from textmate_registry_create()), or NULL (no-op)
/// @warning Do not use registry after calling this function
/// @warning All grammars loaded from this registry become invalid
/// @note Safe to call with NULL
TML_API void textmate_registry_dispose(TextMateRegistry registry);

/// @brief Register a grammar from a JSON file
/// @param registry Valid registry handle
/// @param grammarPath Path to the grammar JSON file (TextMate grammar format)
/// @return Non-zero on success, 0 on error (file not found, invalid JSON, etc.)
/// @note Grammars must be registered before they can be loaded with textmate_registry_load_grammar()
/// @see textmate_registry_add_grammar_from_json(), textmate_registry_load_grammar()
TML_API int textmate_registry_add_grammar_from_file(
    TextMateRegistry registry,
    const char* grammarPath
);

/// @brief Register a grammar from a JSON string
/// @param registry Valid registry handle
/// @param jsonContent Grammar JSON content as a null-terminated string (TextMate grammar format)
/// @return Non-zero on success, 0 on error (invalid JSON, etc.)
/// @note Grammars must be registered before they can be loaded
/// @see textmate_registry_add_grammar_from_file(), textmate_registry_load_grammar()
TML_API int textmate_registry_add_grammar_from_json(
    TextMateRegistry registry,
    const char* jsonContent
);

/// @brief Set grammar injection rules for a scope
/// @param registry Valid registry handle
/// @param scopeName Scope to inject grammars into (e.g., "source.js string.quoted.single")
/// @param injections Array of grammar scope names to inject
/// @param injectionCount Number of injections in the array
/// @note Call before loading the target grammar to take effect
/// @note Allows embedding one grammar within another (e.g., regex highlighting in string literals)
TML_API void textmate_registry_set_injections(
    TextMateRegistry registry,
    const char* scopeName,
    const char** injections,
    int32_t injectionCount
);

/// @brief Load a grammar by scope name
/// @param registry Valid registry handle
/// @param scopeName Scope name of the grammar to load (e.g., "source.javascript", "text.html.markdown")
/// @return Grammar handle on success, NULL if grammar not found or registration failed
/// @note Automatically resolves grammar dependencies and includes
/// @note The grammar must have been previously registered with textmate_registry_add_grammar_*()
/// @see textmate_registry_add_grammar_from_file(), textmate_registry_add_grammar_from_json()
TML_API TextMateGrammar textmate_registry_load_grammar(
    TextMateRegistry registry,
    const char* scopeName
);

/// @}

/// @defgroup tokenization_api Tokenization API
/// @{
/// Tokenize text using a grammar, handling stateful line-by-line parsing.

/// @brief Get the initial parsing state
/// @return The INITIAL state stack (first line of a document)
/// @note This is used as the prevState parameter for the first line
/// @note The returned state is read-only and should not be freed
TML_API TextMateStateStack textmate_get_initial_state();

/// @brief Tokenize a single line of text with decoded scopes
/// @param grammar Valid grammar handle (from textmate_registry_load_grammar())
/// @param lineText The text to tokenize (should not include newline)
/// @param prevState The state from the previous line (or initial state for first line)
/// @return Pointer to tokenization result on success, NULL on error
/// @note The returned result must be freed with textmate_free_tokenize_result()
/// @note Use the ruleStack from the result as prevState for the next line
/// @see textmate_tokenize_line2() for encoded token format (more efficient)
/// @see textmate_get_initial_state()
TML_API TextMateTokenizeResult* textmate_tokenize_line(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
);

/// @brief Tokenize a single line of text with encoded tokens (more efficient)
/// @param grammar Valid grammar handle (from textmate_registry_load_grammar())
/// @param lineText The text to tokenize (should not include newline)
/// @param prevState The state from the previous line (or initial state for first line)
/// @return Pointer to tokenization result on success, NULL on error
/// @note The returned result must be freed with textmate_free_tokenize_result2()
/// @note Tokens are encoded as 32-bit values for better performance
/// @note Prefer this over textmate_tokenize_line() for performance-critical code
TML_API TextMateTokenizeResult2* textmate_tokenize_line2(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
);

/// @brief Tokenize multiple lines in a single call
/// @param grammar Valid grammar handle
/// @param lines Array of line strings (none should include newline)
/// @param lineCount Number of lines in the array
/// @param initialState The state to start with (typically INITIAL or from Session API)
/// @return Pointer to batch result on success, NULL on error
/// @note The returned result must be freed with textmate_free_tokenize_lines_result()
/// @note Reduces FFI call overhead when tokenizing multiple lines (important for language bindings)
/// @note Each result's ruleStack is automatically passed to the next line
/// @see textmate_free_tokenize_lines_result()
TML_API TextMateTokenizeMultiLinesResult* textmate_tokenize_lines(
    TextMateGrammar grammar,
    const char** lines,
    int32_t lineCount,
    TextMateStateStack initialState
);

/// @brief Free a line tokenization result
/// @param result Valid result pointer (from textmate_tokenize_line()), or NULL (no-op)
/// @warning Do not use result after calling this function
/// @note Safe to call with NULL
TML_API void textmate_free_tokenize_result(TextMateTokenizeResult* result);

/// @brief Free an encoded line tokenization result
/// @param result Valid result pointer (from textmate_tokenize_line2()), or NULL (no-op)
/// @warning Do not use result after calling this function
/// @note Safe to call with NULL
TML_API void textmate_free_tokenize_result2(TextMateTokenizeResult2* result);

/// @brief Free a batch tokenization result
/// @param result Valid result pointer (from textmate_tokenize_lines()), or NULL (no-op)
/// @warning Do not use result after calling this function
/// @note Safe to call with NULL
TML_API void textmate_free_tokenize_lines_result(TextMateTokenizeMultiLinesResult* result);

/// @defgroup tokenization_utf16_api UTF-16 Tokenization API
/// @{
/// Tokenize text and return indices as UTF-16 code unit offsets.
/// Use these from language bindings where strings are UTF-16 encoded (C#, JavaScript).
/// The original functions above return UTF-8 byte offsets which are correct for C/C++.

/// @brief Tokenize a single line with decoded scopes, returning UTF-16 indices
/// @param grammar Valid grammar handle (from textmate_registry_load_grammar())
/// @param lineText The text to tokenize (UTF-8, null-terminated)
/// @param prevState The state from the previous line (or initial state for first line)
/// @return Pointer to tokenization result on success, NULL on error
/// @note Token startIndex/endIndex are UTF-16 code unit offsets
/// @note The returned result must be freed with textmate_free_tokenize_result()
TML_API TextMateTokenizeResult* textmate_tokenize_line_utf16(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
);

/// @brief Tokenize a single line with encoded tokens, returning UTF-16 indices
/// @param grammar Valid grammar handle (from textmate_registry_load_grammar())
/// @param lineText The text to tokenize (UTF-8, null-terminated)
/// @param prevState The state from the previous line (or initial state for first line)
/// @return Pointer to tokenization result on success, NULL on error
/// @note Start offsets in the encoded tokens are UTF-16 code unit offsets
/// @note The returned result must be freed with textmate_free_tokenize_result2()
TML_API TextMateTokenizeResult2* textmate_tokenize_line2_utf16(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
);

/// @brief Tokenize multiple lines in a single call, returning UTF-16 indices
/// @param grammar Valid grammar handle
/// @param lines Array of line strings (UTF-8, null-terminated, none should include newline)
/// @param lineCount Number of lines in the array
/// @param initialState The state to start with (typically INITIAL or from Session API)
/// @return Pointer to batch result on success, NULL on error
/// @note Token startIndex/endIndex are UTF-16 code unit offsets
/// @note The returned result must be freed with textmate_free_tokenize_lines_result()
TML_API TextMateTokenizeMultiLinesResult* textmate_tokenize_lines_utf16(
    TextMateGrammar grammar,
    const char** lines,
    int32_t lineCount,
    TextMateStateStack initialState
);

/// @}

/// @brief Get the scope name (language identifier) of a grammar
/// @param grammar Valid grammar handle (from textmate_registry_load_grammar())
/// @return Scope name string (e.g., "source.javascript"), valid for lifetime of grammar
/// @return NULL if grammar is invalid
TML_API const char* textmate_grammar_get_scope_name(TextMateGrammar grammar);

/// @brief Free the Oniguruma library
/// @param onigLib Valid Oniguruma handle (from textmate_oniglib_create()), or NULL (no-op)
/// @warning Do not use onigLib after calling this function
/// @warning All registries and grammars created with this lib become invalid
/// @note Safe to call with NULL
TML_API void textmate_oniglib_dispose(TextMateOnigLib onigLib);

/// @}

#ifdef __cplusplus
}
#endif

#endif // TEXTMATELIB_C_API_H
