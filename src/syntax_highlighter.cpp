#include "syntax_highlighter.h"
#include <chrono>
#include <algorithm>

namespace vscode_textmate {

// ============================================================================
// Utility Functions
// ============================================================================

uint64_t SyntaxHighlighter::getCurrentTimeMs() {
    auto now = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    );
    return static_cast<uint64_t>(ms.count());
}

// ============================================================================
// SyntaxHighlighter - Lifecycle
// ============================================================================

SyntaxHighlighter::SyntaxHighlighter(
    std::shared_ptr<IGrammar> gram,
    Theme* thm,
    bool enableCache
)
    : grammar(gram),
      theme(thm),
      session(nullptr),
      cache(nullptr),
      createdAtMs(getCurrentTimeMs()),
      lastUpdateMs(createdAtMs) {

    if (!grammar) {
        throw std::invalid_argument("Grammar cannot be null");
    }
    if (!theme) {
        throw std::invalid_argument("Theme cannot be null");
    }

    // Create session with the grammar
    // Session handles incremental tokenization with state management
    uint64_t sessionId = SessionManager::createSession(grammar);
    session = SessionManager::getSession(sessionId);

    if (!session) {
        throw std::runtime_error("Failed to create session");
    }

    // Initialize cache if enabled (C++11 compatible)
    if (enableCache) {
        cache.reset(new HighlighterCache());
    }
}

SyntaxHighlighter::~SyntaxHighlighter() {
    // Session is managed by SessionManager with reference counting
    // Just release our reference
    if (session) {
        SessionManager::releaseSession(session->getSessionId());
    }
}

// ============================================================================
// SyntaxHighlighter - Document Management
// ============================================================================

void SyntaxHighlighter::setDocument(const std::vector<std::string>& lines) {
    if (!session) {
        throw std::runtime_error("Session is null");
    }

    session->setLines(lines);
    lastUpdateMs = getCurrentTimeMs();

    // Clear cache when entire document changes
    if (cache) {
        cache->clear();
    }
}

void SyntaxHighlighter::editLine(int lineIndex, const std::string& newContent) {
    if (!session) {
        throw std::runtime_error("Session is null");
    }

    if (lineIndex < 0 || lineIndex >= session->getLineCount()) {
        throw std::out_of_range("Line index out of range");
    }

    // Replace single line using edit operation
    std::vector<std::string> newLines = {newContent};
    session->edit(newLines, lineIndex, 1);
    lastUpdateMs = getCurrentTimeMs();

    // Invalidate cache for affected line and potentially following lines
    if (cache) {
        cache->invalidateLine(lineIndex);
    }
}

void SyntaxHighlighter::insertLines(int startIndex, const std::vector<std::string>& lines) {
    if (!session) {
        throw std::runtime_error("Session is null");
    }

    if (startIndex < 0 || startIndex > session->getLineCount()) {
        throw std::out_of_range("Insert index out of range");
    }

    session->add(lines, startIndex);
    lastUpdateMs = getCurrentTimeMs();

    // Invalidate cache from insertion point onward
    if (cache) {
        cache->invalidateRange(startIndex, session->getLineCount() - 1);
    }
}

void SyntaxHighlighter::removeLines(int startIndex, int count) {
    if (!session) {
        throw std::runtime_error("Session is null");
    }

    if (startIndex < 0 || count < 0 || startIndex + count > session->getLineCount()) {
        throw std::out_of_range("Remove range out of range");
    }

    session->remove(startIndex, count);
    lastUpdateMs = getCurrentTimeMs();

    // Invalidate cache from removal point onward
    if (cache) {
        cache->invalidateRange(startIndex, std::max(0, session->getLineCount() - 1));
    }
}

int SyntaxHighlighter::getLineCount() const {
    if (!session) {
        return 0;
    }
    return session->getLineCount();
}

// ============================================================================
// SyntaxHighlighter - Token to Highlighting Conversion
// ============================================================================

HighlightedToken SyntaxHighlighter::tokenToHighlighted(const IToken& token) {
    HighlightedToken highlighted;
    highlighted.startIndex = token.startIndex;
    highlighted.endIndex = token.endIndex;
    highlighted.scopes = token.scopes;

    // Build scope stack for theme matching
    ScopeStack* scopeStack = buildScopeStack(token.scopes);

    if (scopeStack && theme) {
        // Match scope against theme to get styling
        StyleAttributes* styleAttrs = theme->match(scopeStack);

        if (styleAttrs) {
            // Get color map from theme
            auto colorMap = theme->getColorMap();

            // Resolve foreground color
            if (styleAttrs->foregroundId >= 0 && styleAttrs->foregroundId < static_cast<int>(colorMap.size())) {
                highlighted.foregroundColor = colorMap[styleAttrs->foregroundId];
            }

            // Resolve background color
            if (styleAttrs->backgroundId >= 0 && styleAttrs->backgroundId < static_cast<int>(colorMap.size())) {
                highlighted.backgroundColor = colorMap[styleAttrs->backgroundId];
            }

            // Apply font style
            highlighted.fontStyle = styleAttrs->fontStyle;
        }
    }

    // Clean up scope stack
    if (scopeStack) {
        delete scopeStack;
    }

    // Build debug info
    if (!token.scopes.empty()) {
        highlighted.debugInfo = token.scopes[0];
        for (size_t i = 1; i < token.scopes.size(); ++i) {
            highlighted.debugInfo += " " + token.scopes[i];
        }
    }

    return highlighted;
}

ScopeStack* SyntaxHighlighter::buildScopeStack(const std::vector<std::string>& scopes) {
    if (scopes.empty()) {
        return nullptr;
    }

    // Build scope stack from scope names
    // ScopeStack is a linked-list like structure where each node represents a scope
    ScopeStack* result = ScopeStack::from(scopes);
    return result;
}

// ============================================================================
// SyntaxHighlighter - Querying Highlighted Content
// ============================================================================

HighlightedLine SyntaxHighlighter::getHighlightedLine(int lineIndex) {
    if (!session) {
        throw std::runtime_error("Session is null");
    }

    if (lineIndex < 0 || lineIndex >= session->getLineCount()) {
        throw std::out_of_range("Line index out of range");
    }

    // Check cache first
    uint64_t version = lastUpdateMs;
    if (cache) {
        HighlightedLine* cached = cache->getCachedLine(lineIndex, version);
        if (cached) {
            return *cached;
        }
    }

    HighlightedLine result;
    result.lineIndex = lineIndex;
    result.version = version;

    // Get raw line content and tokens from session
    const SessionLine* sessionLine = session->getLine(lineIndex);
    if (!sessionLine) {
        result.isComplete = false;
        return result;
    }

    result.content = sessionLine->content;
    result.isComplete = sessionLine->cached;

    // Convert each raw token to highlighted token
    result.tokens.reserve(sessionLine->tokens.size());
    for (const auto& token : sessionLine->tokens) {
        result.tokens.push_back(tokenToHighlighted(token));
    }

    // Cache the result
    if (cache) {
        cache->cacheLine(result, version);
    }

    return result;
}

std::vector<HighlightedLine> SyntaxHighlighter::getHighlightedRange(
    int startIndex,
    int endIndex
) {
    if (!session) {
        throw std::runtime_error("Session is null");
    }

    int lineCount = session->getLineCount();
    if (startIndex < 0 || endIndex >= lineCount || startIndex > endIndex) {
        throw std::out_of_range("Range out of bounds");
    }

    std::vector<HighlightedLine> results;
    results.reserve(endIndex - startIndex + 1);

    for (int i = startIndex; i <= endIndex; ++i) {
        results.push_back(getHighlightedLine(i));
    }

    return results;
}

std::vector<IToken> SyntaxHighlighter::getLineTokens(int lineIndex) {
    if (!session) {
        throw std::runtime_error("Session is null");
    }

    if (lineIndex < 0 || lineIndex >= session->getLineCount()) {
        throw std::out_of_range("Line index out of range");
    }

    const std::vector<IToken>* tokens = session->getLineTokens(lineIndex);
    if (!tokens) {
        return std::vector<IToken>();
    }

    return *tokens;
}

// ============================================================================
// SyntaxHighlighter - Theme Management
// ============================================================================

void SyntaxHighlighter::setTheme(Theme* newTheme) {
    if (!newTheme) {
        throw std::invalid_argument("Theme cannot be null");
    }

    theme = newTheme;
    lastUpdateMs = getCurrentTimeMs();

    // Clear cache since all colors/styles are now different
    if (cache) {
        cache->clear();
    }
}

Theme* SyntaxHighlighter::getTheme() const {
    return theme;
}

// ============================================================================
// SyntaxHighlighter - Cache Management
// ============================================================================

void SyntaxHighlighter::clearCache() {
    if (cache) {
        cache->clear();
    }
}

void SyntaxHighlighter::invalidateCacheRange(int startIndex, int endIndex) {
    if (!cache) {
        return;
    }

    if (startIndex < 0 || endIndex < 0 || startIndex > endIndex) {
        throw std::invalid_argument("Invalid cache range");
    }

    cache->invalidateRange(startIndex, endIndex);
}

// ============================================================================
// SyntaxHighlighter - Debugging & Monitoring
// ============================================================================

SyntaxHighlightingMetadata SyntaxHighlighter::getMetadata() const {
    SyntaxHighlightingMetadata metadata;

    if (session) {
        SessionMetadata sessionMeta = session->getMetadata();
        metadata.sessionId = sessionMeta.createdAtMs;  // Using timestamp as unique ID
        metadata.lineCount = sessionMeta.lineCount;
        metadata.cachedLineCount = sessionMeta.cachedLineCount;
    }

    if (theme) {
        auto colorMap = theme->getColorMap();
        metadata.themeColorCount = static_cast<int>(colorMap.size());
    }

    metadata.lastUpdateMs = lastUpdateMs;

    if (cache) {
        metadata.cachedLineCount = cache->getCachedLineCount();
    }

    return metadata;
}

SessionImpl* SyntaxHighlighter::getSession() const {
    return session.get();
}

// ============================================================================
// HighlighterCache Implementation
// ============================================================================

HighlightedLine* HighlighterCache::getCachedLine(int lineIndex, uint64_t version) {
    auto it = lineCache.find(lineIndex);
    if (it != lineCache.end() && it->second.version == version) {
        return &it->second.line;
    }
    return nullptr;
}

void HighlighterCache::cacheLine(const HighlightedLine& line, uint64_t version) {
    lineCache[line.lineIndex] = {line, version};
}

void HighlighterCache::invalidateLine(int lineIndex) {
    lineCache.erase(lineIndex);
}

void HighlighterCache::invalidateRange(int startIndex, int endIndex) {
    auto it = lineCache.begin();
    while (it != lineCache.end()) {
        if (it->first >= startIndex && it->first <= endIndex) {
            it = lineCache.erase(it);
        } else {
            ++it;
        }
    }
}

void HighlighterCache::clear() {
    lineCache.clear();
}

size_t HighlighterCache::getCachedLineCount() const {
    return lineCache.size();
}

} // namespace vscode_textmate
