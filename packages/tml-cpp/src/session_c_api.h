#ifndef TEXTMATELIB_SESSION_C_API_H
#define TEXTMATELIB_SESSION_C_API_H

#include "tml_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// ============================================================================
// Session API - High-Level Stateful Tokenization
// ============================================================================
//
// The Session API provides a stateful, incremental tokenization interface
// designed for text editors. It manages line caching and state tracking
// internally, allowing clients to work with high-level edit operations
// (add, edit, remove) rather than low-level tokenization.
//
// Key Design Goals:
// 1. Incremental tokenization - only retokenize affected lines
// 2. Automatic state cascading - stop when state stabilizes
// 3. Memory safety - reference counting prevents leaks
// 4. Automatic cleanup - expired sessions are cleaned up
//

// Opaque session handle
typedef uint64_t TextMateSession;

// Represents a cached line with tokens and state
typedef struct {
    TextMateToken* tokens;           // Array of tokens for this line
    int32_t tokenCount;              // Number of tokens
    TextMateStateStack state;        // State at end of line
    uint64_t version;                // Version number for change tracking
} TextMateSessionLine;

// Result structure for batch queries
typedef struct {
    TextMateSessionLine* lines;      // Array of line results
    int32_t lineCount;               // Number of lines returned
} TextMateSessionLinesResult;

// ============================================================================
// Session Lifecycle
// ============================================================================

// Create a new session for incremental tokenization
//
// Args:
//   grammar: The grammar to use for tokenization (must not be NULL)
//
// Returns:
//   Session ID on success, 0 on error
//
// Notes:
//   - Session starts with INITIAL state
//   - Reference count is set to 1 (ownership transferred to caller)
//   - Must call textmate_session_dispose() to release
TML_API TextMateSession textmate_session_create(TextMateGrammar grammar);

// Increment reference count for a session
//
// Args:
//   session: Session ID from textmate_session_create()
//
// Notes:
//   - Used when storing session handles in multiple places
//   - Must be paired with textmate_session_release()
TML_API void textmate_session_retain(TextMateSession session);

// Decrement reference count for a session
//
// Args:
//   session: Session ID
//
// Notes:
//   - When reference count reaches 0, session is destroyed
//   - Safe to call multiple times (decrements until 0)
//   - Finalizer safety net: finalizer calls this if not explicitly called
TML_API void textmate_session_release(TextMateSession session);

// Dispose a session explicitly
//
// Args:
//   session: Session ID
//
// Notes:
//   - Shorthand for textmate_session_release() (decrements refcount)
//   - Recommended way to dispose for API clarity
//   - Should be called from IDisposable.Dispose() in C#
TML_API void textmate_session_dispose(TextMateSession session);

// ============================================================================
// Session State Management
// ============================================================================

// Set the complete document lines (initializes session)
//
// Args:
//   session: Session ID
//   lines: Array of line strings
//   lineCount: Number of lines
//
// Returns:
//   0 on success, non-zero on error
//
// Notes:
//   - Clears existing cache and tokenizes all lines
//   - Uses initial state for first line
//   - Cascades state naturally between lines
TML_API int textmate_session_set_lines(
    TextMateSession session,
    const char** lines,
    int32_t lineCount
);

// Get current number of lines in session
TML_API int32_t textmate_session_get_line_count(TextMateSession session);

// ============================================================================
// Incremental Tokenization Operations
// ============================================================================

// Edit (replace) a range of lines and retokenize
//
// Args:
//   session: Session ID
//   lines: New line content array
//   lineCount: Number of lines in array
//   startIndex: 0-based index of first line to replace
//   replaceCount: Number of existing lines to replace
//
// Returns:
//   0 on success, non-zero on error
//
// Behavior:
//   1. Replaces lines[startIndex:startIndex+replaceCount] with new lines
//   2. Invalidates cached tokens from startIndex onwards
//   3. Retokenizes from startIndex, cascading state until stable
//   4. Stops early when state matches expected (incremental optimization)
//
// Example (user edits line 50):
//   textmate_session_edit(session, newLines, 1, 50, 1);
//   // Retokenizes line 50 and cascades forward until state stabilizes
TML_API int textmate_session_edit(
    TextMateSession session,
    const char** lines,
    int32_t lineCount,
    int32_t startIndex,
    int32_t replaceCount
);

// Add (insert) new lines at specified position
//
// Args:
//   session: Session ID
//   lines: New line content array to insert
//   lineCount: Number of lines to insert
//   insertIndex: 0-based position to insert at
//
// Returns:
//   0 on success, non-zero on error
//
// Behavior:
//   1. Shifts all lines at insertIndex and beyond down by lineCount
//   2. Inserts new lines at insertIndex
//   3. Invalidates cache from insertIndex onwards
//   4. Retokenizes new lines, cascading forward
//
// Example (user pastes 5 lines at line 100):
//   textmate_session_add(session, pastedLines, 5, 100);
TML_API int textmate_session_add(
    TextMateSession session,
    const char** lines,
    int32_t lineCount,
    int32_t insertIndex
);

// Remove (delete) a range of lines
//
// Args:
//   session: Session ID
//   startIndex: 0-based index of first line to remove
//   removeCount: Number of lines to remove
//
// Returns:
//   0 on success, non-zero on error
//
// Behavior:
//   1. Removes lines[startIndex:startIndex+removeCount]
//   2. Shifts remaining lines up
//   3. Invalidates cache from startIndex onwards
//   4. Retokenizes forward until state stabilizes
//
// Example (user deletes 3 lines starting at line 50):
//   textmate_session_remove(session, 50, 3);
TML_API int textmate_session_remove(
    TextMateSession session,
    int32_t startIndex,
    int32_t removeCount
);

// ============================================================================
// Query Operations
// ============================================================================

// Get cached tokens for a single line
//
// Args:
//   session: Session ID
//   lineIndex: 0-based line number
//
// Returns:
//   Result structure with tokens, or empty result if line not cached
//
// Notes:
//   - Returns cached tokens without retokenization
//   - Returns empty if line hasn't been tokenized yet
//   - Token pointers are valid until next edit operation
TML_API TextMateTokenizeResult* textmate_session_get_line_tokens(
    TextMateSession session,
    int32_t lineIndex
);

// Get cached state for a single line (state at end of line)
//
// Args:
//   session: Session ID
//   lineIndex: 0-based line number
//
// Returns:
//   State stack, or NULL if line not tokenized
//
// Notes:
//   - Useful for external state analysis
//   - State is valid until next edit operation
TML_API TextMateStateStack textmate_session_get_line_state(
    TextMateSession session,
    int32_t lineIndex
);

// Get tokens for a range of lines in a single call
//
// Args:
//   session: Session ID
//   startIndex: 0-based start line
//   endIndex: 0-based end line (inclusive)
//
// Returns:
//   Result array with all line tokens and states
//
// Notes:
//   - More efficient than multiple get_line_tokens() calls
//   - Useful for rendering screen buffers
//   - Must be freed with textmate_session_free_lines_result()
TML_API TextMateSessionLinesResult* textmate_session_get_tokens_range(
    TextMateSession session,
    int32_t startIndex,
    int32_t endIndex
);

// Free query result
TML_API void textmate_session_free_tokens_result(
    TextMateTokenizeResult* result
);

// Free batch query result
TML_API void textmate_session_free_lines_result(
    TextMateSessionLinesResult* result
);

// ============================================================================
// Maintenance Operations
// ============================================================================

// Invalidate cached tokens for a range of lines
//
// Args:
//   session: Session ID
//   startIndex: 0-based start line
//   endIndex: 0-based end line (inclusive, -1 for end of file)
//
// Notes:
//   - Forces retokenization on next query
//   - Useful if grammar changed or external state modified
//   - Automatically called by edit/add/remove operations
TML_API void textmate_session_invalidate_range(
    TextMateSession session,
    int32_t startIndex,
    int32_t endIndex
);

// Clear entire session cache (but keep lines)
//
// Args:
//   session: Session ID
//
// Notes:
//   - Resets all cached tokens and states
//   - Useful for grammar changes
//   - Document structure preserved
TML_API void textmate_session_clear_cache(TextMateSession session);

// Cleanup expired sessions (automatic memory management)
//
// Args:
//   maxAgeMs: Remove sessions older than this (in milliseconds)
//
// Notes:
//   - Called periodically (e.g., every 100 operations)
//   - Part of automatic cleanup defense layer
//   - Safe to call frequently (low overhead)
//   - Helps prevent leaks from abandoned sessions
TML_API void textmate_session_cleanup_expired(int32_t maxAgeMs);

// Get session metadata
//
// Returns version/age info (useful for debugging, optional)
typedef struct {
    uint64_t createdAtMs;            // Timestamp of creation
    uint32_t referenceCount;         // Current reference count
    int32_t lineCount;               // Current line count
    int32_t cachedLineCount;         // Number of cached lines
    uint64_t memoryUsageBytes;       // Approximate memory usage
} TextMateSessionMetadata;

TML_API TextMateSessionMetadata textmate_session_get_metadata(
    TextMateSession session
);

#ifdef __cplusplus
}
#endif

#endif // TEXTMATELIB_SESSION_C_API_H
