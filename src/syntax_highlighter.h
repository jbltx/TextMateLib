#ifndef TEXTMATELIB_SYNTAX_HIGHLIGHTER_H
#define TEXTMATELIB_SYNTAX_HIGHLIGHTER_H

#include "types.h"
#include "session.h"
#include "theme.h"
#include "grammar.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <map>

namespace tml {

// Forward declarations
class SyntaxHighlighter;
class HighlighterCache;

// HighlightedToken represents a single token with complete styling information
struct HighlightedToken {
    int startIndex;                        // Character position in line where token starts
    int endIndex;                          // Character position in line where token ends
    std::vector<std::string> scopes;       // Scope path (e.g., "source.js string.quoted.double")

    // Applied styling from theme
    std::string foregroundColor;           // Hex color (#RRGGBB) or color name
    std::string backgroundColor;           // Hex color or empty string
    int fontStyle;                         // Combination of FontStyle flags (bold, italic, underline, strikethrough)

    // Metadata
    StandardTokenType tokenType;           // Token classification: Comment, String, RegEx, Other
    std::string debugInfo;                 // Debug information (scope chain, etc.)

    HighlightedToken()
        : startIndex(0),
          endIndex(0),
          fontStyle(static_cast<int>(FontStyle::None)),
          tokenType(StandardTokenType::Other) {}
};

// HighlightedLine represents complete highlighting information for a single line
struct HighlightedLine {
    int lineIndex;                         // Line number in document
    std::string content;                   // The actual line content
    std::vector<HighlightedToken> tokens;  // Tokens with applied styling
    bool isComplete;                       // Whether tokenization completed without timeout
    uint64_t version;                      // Version number for cache invalidation

    HighlightedLine()
        : lineIndex(-1),
          isComplete(true),
          version(0) {}
};

// SyntaxHighlightingMetadata provides debugging and performance information
struct SyntaxHighlightingMetadata {
    uint64_t sessionId;                    // Unique session identifier
    int lineCount;                         // Total lines in document
    int cachedLineCount;                   // Number of cached highlighted lines

    // Performance metrics
    double averageLineTokenizationMs;      // Average tokenization time per line
    int64_t lastUpdateMs;                  // Timestamp of last update

    // Theme information
    std::string themeName;                 // Name of active theme
    int themeColorCount;                   // Number of colors in theme

    SyntaxHighlightingMetadata()
        : sessionId(0),
          lineCount(0),
          cachedLineCount(0),
          averageLineTokenizationMs(0.0),
          lastUpdateMs(0),
          themeColorCount(0) {}
};

// Main SyntaxHighlighter class
// Combines Session API (incremental tokenization) with Theme system (color/style application)
// Provides convenient high-level interface for text editors to get syntax-highlighted content
class SyntaxHighlighter {
private:
    // Core components
    std::shared_ptr<IGrammar> grammar;                  // Grammar for tokenization
    Theme* theme;                                       // Theme for styling (referenced, not owned)
    std::shared_ptr<SessionImpl> session;                // Session for incremental tokenization with state management
    std::unique_ptr<HighlighterCache> cache;            // Optional cache for highlighted lines

    // Performance tracking
    uint64_t createdAtMs;
    uint64_t lastUpdateMs;

    // Helper methods

    /// Convert a single IToken to HighlightedToken by applying theme styling
    /// @param token Raw token from tokenization
    /// @return HighlightedToken with resolved colors and styles
    HighlightedToken tokenToHighlighted(const IToken& token);

    /// Build scope stack from token scopes
    /// @param scopes Vector of scope names
    /// @return ScopeStack for theme matching
    ScopeStack* buildScopeStack(const std::vector<std::string>& scopes);

    /// Get current timestamp in milliseconds
    static uint64_t getCurrentTimeMs();

public:
    // ============================================================================
    // Lifecycle
    // ============================================================================

    /// Create a new SyntaxHighlighter
    /// @param gram IGrammar for tokenization
    /// @param thm Theme for coloring and styling
    /// @param enableCache Enable optional line caching (default: true)
    SyntaxHighlighter(
        std::shared_ptr<IGrammar> gram,
        Theme* thm,
        bool enableCache = true
    );

    ~SyntaxHighlighter();

    // Delete copy operations (session has internal state)
    SyntaxHighlighter(const SyntaxHighlighter&) = delete;
    SyntaxHighlighter& operator=(const SyntaxHighlighter&) = delete;

    // Allow move operations
    SyntaxHighlighter(SyntaxHighlighter&&) noexcept = default;
    SyntaxHighlighter& operator=(SyntaxHighlighter&&) noexcept = default;

    // ============================================================================
    // Document Management
    // ============================================================================

    /// Load a complete document
    /// @param lines Document lines
    void setDocument(const std::vector<std::string>& lines);

    /// Edit a single line
    /// @param lineIndex Line to edit
    /// @param newContent New content for the line
    void editLine(int lineIndex, const std::string& newContent);

    /// Insert lines at specified position
    /// @param startIndex Insert position
    /// @param lines Lines to insert
    void insertLines(int startIndex, const std::vector<std::string>& lines);

    /// Remove lines
    /// @param startIndex Start of removal
    /// @param count Number of lines to remove
    void removeLines(int startIndex, int count);

    /// Get current line count
    int getLineCount() const;

    // ============================================================================
    // Querying Highlighted Content
    // ============================================================================

    /// Get syntax-highlighted version of a single line
    /// Returns fully resolved colors, fonts, and scope information
    /// @param lineIndex Line to highlight
    /// @return HighlightedLine with complete styling
    HighlightedLine getHighlightedLine(int lineIndex);

    /// Get syntax-highlighted version of a line range (batch query)
    /// More efficient than calling getHighlightedLine multiple times
    /// @param startIndex Start line (inclusive)
    /// @param endIndex End line (inclusive)
    /// @return Vector of HighlightedLine structs
    std::vector<HighlightedLine> getHighlightedRange(int startIndex, int endIndex);

    /// Get raw tokens for a line (without theme styling)
    /// Useful for lower-level access or custom rendering
    /// @param lineIndex Line to tokenize
    /// @return Vector of IToken with scope information
    std::vector<IToken> getLineTokens(int lineIndex);

    // ============================================================================
    // Theme Management
    // ============================================================================

    /// Switch to a different theme
    /// Invalidates all cached highlighting (will be recomputed on next query)
    /// @param newTheme New Theme to apply
    void setTheme(Theme* newTheme);

    /// Get the currently active theme
    /// @return Pointer to active Theme
    Theme* getTheme() const;

    // ============================================================================
    // Cache Management
    // ============================================================================

    /// Clear all cached highlighted lines
    void clearCache();

    /// Invalidate cached highlighting for a specific line range
    /// Forces recomputation on next query
    /// @param startIndex Start line
    /// @param endIndex End line
    void invalidateCacheRange(int startIndex, int endIndex);

    // ============================================================================
    // Debugging & Monitoring
    // ============================================================================

    /// Get debugging and performance metadata
    /// @return SyntaxHighlightingMetadata with statistics
    SyntaxHighlightingMetadata getMetadata() const;

    /// Get the underlying SessionImpl (for advanced usage)
    /// @return Pointer to internal SessionImpl
    SessionImpl* getSession() const;
};

// Optional cache class for highlighted lines
// Improves performance when repeatedly querying the same lines
class HighlighterCache {
private:
    struct CachedEntry {
        HighlightedLine line;
        uint64_t version;
    };

    std::map<int, CachedEntry> lineCache;

public:
    HighlighterCache() = default;
    ~HighlighterCache() = default;

    // Delete copy operations
    HighlighterCache(const HighlighterCache&) = delete;
    HighlighterCache& operator=(const HighlighterCache&) = delete;

    /// Check if a line is cached
    /// @param lineIndex Line to check
    /// @param version Current version number (for validation)
    /// @return Cached HighlightedLine or nullptr if not cached/invalid
    HighlightedLine* getCachedLine(int lineIndex, uint64_t version);

    /// Cache a highlighted line
    /// @param line HighlightedLine to cache
    /// @param version Version number
    void cacheLine(const HighlightedLine& line, uint64_t version);

    /// Invalidate cache for a specific line
    /// @param lineIndex Line to invalidate
    void invalidateLine(int lineIndex);

    /// Invalidate cache for a range
    /// @param startIndex Start line
    /// @param endIndex End line
    void invalidateRange(int startIndex, int endIndex);

    /// Clear entire cache
    void clear();

    /// Get cache statistics
    size_t getCachedLineCount() const;
};

} // namespace tml

#endif // TEXTMATELIB_SYNTAX_HIGHLIGHTER_H
