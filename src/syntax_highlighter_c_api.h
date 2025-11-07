#ifndef TEXTMATELIB_SYNTAX_HIGHLIGHTER_C_API_H
#define TEXTMATELIB_SYNTAX_HIGHLIGHTER_C_API_H

#include "tml_export.h"
#include "c_api.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Opaque Handle Types
// ============================================================================

typedef void* textmate_syntax_highlighter_t;
typedef void* textmate_highlighted_token_t;
typedef void* textmate_highlighted_line_t;

// ============================================================================
// C Structures Matching C++ Data
// ============================================================================

/// Corresponds to HighlightedToken in C++
typedef struct {
    int32_t startIndex;
    int32_t endIndex;
    const char** scopes;
    int32_t scopeCount;

    // Applied styling
    const char* foregroundColor;           // Hex color like "#FF0000"
    const char* backgroundColor;           // Hex color or NULL
    int32_t fontStyle;                     // Bit flags: 1=italic, 2=bold, 4=underline, 8=strikethrough

    // Metadata
    int32_t tokenType;                     // StandardTokenType: 0=Other, 1=Comment, 2=String, 3=RegEx
    const char* debugInfo;
} textmate_highlighted_token_c;

/// Corresponds to HighlightedLine in C++
typedef struct {
    int32_t lineIndex;
    const char* content;
    const textmate_highlighted_token_c* tokens;
    int32_t tokenCount;
    int32_t isComplete;                    // 0 = false, 1 = true
    uint64_t version;
} textmate_highlighted_line_c;

/// Corresponds to SyntaxHighlightingMetadata in C++
typedef struct {
    uint64_t sessionId;
    int32_t lineCount;
    int32_t cachedLineCount;
    double averageLineTokenizationMs;
    int64_t lastUpdateMs;
    const char* themeName;
    int32_t themeColorCount;
} textmate_syntax_highlighting_metadata_c;

// ============================================================================
// Lifecycle API
// ============================================================================

/// Create a new syntax highlighter
/// @param grammar Grammar for tokenization (textmate_grammar_t)
/// @param theme Theme for styling (textmate_theme_t)
/// @return Opaque handle to highlighter, or NULL on error
/// @note Caller must call textmate_syntax_highlighter_dispose() to free memory
TML_API textmate_syntax_highlighter_t textmate_syntax_highlighter_create(
    textmate_grammar_t grammar,
    textmate_theme_t theme
);

/// Create a syntax highlighter with optional cache
/// @param grammar Grammar for tokenization
/// @param theme Theme for styling
/// @param enableCache 1 to enable caching, 0 to disable
/// @return Opaque handle to highlighter, or NULL on error
TML_API textmate_syntax_highlighter_t textmate_syntax_highlighter_create_with_cache(
    textmate_grammar_t grammar,
    textmate_theme_t theme,
    int32_t enableCache
);

/// Dispose of a syntax highlighter and free associated memory
/// @param highlighter Highlighter to dispose
TML_API void textmate_syntax_highlighter_dispose(textmate_syntax_highlighter_t highlighter);

// ============================================================================
// Document Management API
// ============================================================================

/// Set the entire document content
/// @param highlighter Highlighter instance
/// @param lines Array of line strings
/// @param lineCount Number of lines
TML_API void textmate_syntax_highlighter_set_document(
    textmate_syntax_highlighter_t highlighter,
    const char** lines,
    int32_t lineCount
);

/// Edit a single line
/// @param highlighter Highlighter instance
/// @param lineIndex Line to edit (0-based)
/// @param newContent New content for the line
TML_API void textmate_syntax_highlighter_edit_line(
    textmate_syntax_highlighter_t highlighter,
    int32_t lineIndex,
    const char* newContent
);

/// Insert lines at specified position
/// @param highlighter Highlighter instance
/// @param startIndex Position to insert (0-based)
/// @param lines Array of lines to insert
/// @param lineCount Number of lines to insert
TML_API void textmate_syntax_highlighter_insert_lines(
    textmate_syntax_highlighter_t highlighter,
    int32_t startIndex,
    const char** lines,
    int32_t lineCount
);

/// Remove lines
/// @param highlighter Highlighter instance
/// @param startIndex Start of removal (0-based)
/// @param count Number of lines to remove
TML_API void textmate_syntax_highlighter_remove_lines(
    textmate_syntax_highlighter_t highlighter,
    int32_t startIndex,
    int32_t count
);

/// Get current line count
/// @param highlighter Highlighter instance
/// @return Number of lines in document
TML_API int32_t textmate_syntax_highlighter_get_line_count(
    textmate_syntax_highlighter_t highlighter
);

// ============================================================================
// Query API - Get Highlighted Content
// ============================================================================

/// Get syntax-highlighted version of a single line
/// Returns fully resolved colors, fonts, and scope information
/// @param highlighter Highlighter instance
/// @param lineIndex Line to highlight (0-based)
/// @return Opaque handle to HighlightedLine, or NULL on error
/// @note Caller must call textmate_highlighted_line_dispose() to free memory
TML_API textmate_highlighted_line_t textmate_syntax_highlighter_get_highlighted_line(
    textmate_syntax_highlighter_t highlighter,
    int32_t lineIndex
);

/// Get syntax-highlighted version of a line range (batch query)
/// More efficient than calling get_highlighted_line multiple times
/// @param highlighter Highlighter instance
/// @param startIndex Start line (0-based, inclusive)
/// @param endIndex End line (0-based, inclusive)
/// @param outResults Array to store results (caller allocates)
/// @param outResultCount Output: number of results returned
/// @return Number of highlighted lines retrieved, or -1 on error
TML_API int32_t textmate_syntax_highlighter_get_highlighted_range(
    textmate_syntax_highlighter_t highlighter,
    int32_t startIndex,
    int32_t endIndex,
    textmate_highlighted_line_t* outResults,
    int32_t* outResultCount
);

/// Get raw tokens for a line (without theme styling)
/// Useful for lower-level access or custom rendering
/// @param highlighter Highlighter instance
/// @param lineIndex Line to tokenize (0-based)
/// @param outTokens Output array for tokens (caller allocates)
/// @param outTokenCount Output: number of tokens
/// @return Number of tokens, or -1 on error
/// @note Token scopes are owned by the highlighter, do not free
TML_API int32_t textmate_syntax_highlighter_get_line_tokens(
    textmate_syntax_highlighter_t highlighter,
    int32_t lineIndex,
    textmate_token_t* outTokens,
    int32_t* outTokenCount
);

// ============================================================================
// Theme Management API
// ============================================================================

/// Switch to a different theme
/// Invalidates all cached highlighting
/// @param highlighter Highlighter instance
/// @param theme New theme to apply
TML_API void textmate_syntax_highlighter_set_theme(
    textmate_syntax_highlighter_t highlighter,
    textmate_theme_t theme
);

/// Get the currently active theme
/// @param highlighter Highlighter instance
/// @return Theme handle (do not dispose, owned by highlighter)
TML_API textmate_theme_t textmate_syntax_highlighter_get_theme(
    textmate_syntax_highlighter_t highlighter
);

// ============================================================================
// Cache Management API
// ============================================================================

/// Clear all cached highlighted lines
/// Forces recomputation on next query
/// @param highlighter Highlighter instance
TML_API void textmate_syntax_highlighter_clear_cache(
    textmate_syntax_highlighter_t highlighter
);

/// Invalidate cached highlighting for a line range
/// @param highlighter Highlighter instance
/// @param startIndex Start line (0-based)
/// @param endIndex End line (0-based)
TML_API void textmate_syntax_highlighter_invalidate_cache_range(
    textmate_syntax_highlighter_t highlighter,
    int32_t startIndex,
    int32_t endIndex
);

// ============================================================================
// Debugging & Monitoring API
// ============================================================================

/// Get debugging and performance metadata
/// @param highlighter Highlighter instance
/// @param outMetadata Output metadata structure (caller allocates)
/// @return 0 on success, -1 on error
TML_API int32_t textmate_syntax_highlighter_get_metadata(
    textmate_syntax_highlighter_t highlighter,
    textmate_syntax_highlighting_metadata_c* outMetadata
);

// ============================================================================
// Highlighted Line API (Accessors)
// ============================================================================

/// Get line index from highlighted line
/// @param line Highlighted line handle
/// @return Line index (0-based)
TML_API int32_t textmate_highlighted_line_get_index(textmate_highlighted_line_t line);

/// Get line content from highlighted line
/// @param line Highlighted line handle
/// @return Null-terminated string (owned by line, do not free)
TML_API const char* textmate_highlighted_line_get_content(textmate_highlighted_line_t line);

/// Get token count for highlighted line
/// @param line Highlighted line handle
/// @return Number of tokens
TML_API int32_t textmate_highlighted_line_get_token_count(textmate_highlighted_line_t line);

/// Get token at index from highlighted line
/// @param line Highlighted line handle
/// @param tokenIndex Token index (0-based)
/// @return Token data (owned by line, do not free)
TML_API const textmate_highlighted_token_c* textmate_highlighted_line_get_token(
    textmate_highlighted_line_t line,
    int32_t tokenIndex
);

/// Get completion status for highlighted line
/// @param line Highlighted line handle
/// @return 1 if tokenization completed, 0 if stopped early
TML_API int32_t textmate_highlighted_line_is_complete(textmate_highlighted_line_t line);

/// Get version of highlighted line
/// @param line Highlighted line handle
/// @return Version number for cache tracking
TML_API uint64_t textmate_highlighted_line_get_version(textmate_highlighted_line_t line);

/// Dispose of a highlighted line and free associated memory
/// @param line Highlighted line to dispose
TML_API void textmate_highlighted_line_dispose(textmate_highlighted_line_t line);

// ============================================================================
// Highlighted Token API (Accessors)
// ============================================================================

/// Get start index from token
/// @param token Token handle
/// @return Character position in line where token starts
TML_API int32_t textmate_highlighted_token_get_start_index(textmate_highlighted_token_t token);

/// Get end index from token
/// @param token Token handle
/// @return Character position in line where token ends
TML_API int32_t textmate_highlighted_token_get_end_index(textmate_highlighted_token_t token);

/// Get scope count for token
/// @param token Token handle
/// @return Number of scopes in the scope path
TML_API int32_t textmate_highlighted_token_get_scope_count(textmate_highlighted_token_t token);

/// Get scope at index from token
/// @param token Token handle
/// @param scopeIndex Scope index (0-based)
/// @return Scope name (owned by token, do not free)
TML_API const char* textmate_highlighted_token_get_scope(
    textmate_highlighted_token_t token,
    int32_t scopeIndex
);

/// Get foreground color from token
/// @param token Token handle
/// @return Hex color string like "#FF0000" or NULL if not set (owned by token, do not free)
TML_API const char* textmate_highlighted_token_get_foreground_color(
    textmate_highlighted_token_t token
);

/// Get background color from token
/// @param token Token handle
/// @return Hex color string or NULL if not set (owned by token, do not free)
TML_API const char* textmate_highlighted_token_get_background_color(
    textmate_highlighted_token_t token
);

/// Get font style from token
/// @param token Token handle
/// @return Bit flags: 1=italic, 2=bold, 4=underline, 8=strikethrough
TML_API int32_t textmate_highlighted_token_get_font_style(textmate_highlighted_token_t token);

/// Get token type from token
/// @param token Token handle
/// @return StandardTokenType: 0=Other, 1=Comment, 2=String, 3=RegEx
TML_API int32_t textmate_highlighted_token_get_type(textmate_highlighted_token_t token);

/// Get debug info from token
/// @param token Token handle
/// @return Debug string (owned by token, do not free)
TML_API const char* textmate_highlighted_token_get_debug_info(textmate_highlighted_token_t token);

/// Dispose of a highlighted token and free associated memory
/// @param token Token to dispose
TML_API void textmate_highlighted_token_dispose(textmate_highlighted_token_t token);

#ifdef __cplusplus
}
#endif

#endif // TEXTMATELIB_SYNTAX_HIGHLIGHTER_C_API_H
