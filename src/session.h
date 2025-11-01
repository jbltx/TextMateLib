#ifndef VSCODE_TEXTMATE_SESSION_H
#define VSCODE_TEXTMATE_SESSION_H

#include "grammar.h"
#include "types.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <string>
#include <map>

namespace vscode_textmate {

// Forward declaration
class SessionImpl;

// Represents a cached line with tokens and state
struct SessionLine {
    std::string content;                    // Line content
    std::vector<IToken> tokens;             // Cached tokens
    StateStack* state;                      // State at end of line
    uint64_t version;                       // Version for change tracking
    bool cached;                            // Whether this line has been tokenized

    SessionLine()
        : content(""),
          state(nullptr),
          version(0),
          cached(false) {}

    ~SessionLine();
};

// Session metadata for debugging and monitoring
struct SessionMetadata {
    uint64_t createdAtMs;                   // Timestamp of creation
    uint32_t referenceCount;                // Current reference count
    int32_t lineCount;                      // Current line count
    int32_t cachedLineCount;                // Number of cached lines
    uint64_t memoryUsageBytes;              // Approximate memory usage
};

// High-level stateful tokenization interface for text editors
//
// The Session API provides incremental tokenization with automatic state management.
// It caches tokens and state per line, allowing editors to efficiently handle
// incremental edits with early stopping when state stabilizes.
class SessionImpl {
private:
    // Session identity and state
    uint64_t sessionId;                     // Unique session identifier
    std::shared_ptr<IGrammar> grammar;      // Grammar for tokenization
    std::vector<SessionLine> lines;         // Cached lines with tokens and state
    uint32_t referenceCount;                // Reference count for memory management
    uint64_t createdAtMs;                   // Creation time in milliseconds
    uint64_t lastAccessMs;                  // Last access time for expiry
    uint64_t nextVersion;                   // Version counter for cache invalidation

    // Incremental tokenization helpers
    bool isStateEqual(StateStack* state1, StateStack* state2) const;
    void invalidateFrom(int32_t startIndex);
    void retokenizeLines(int32_t startIndex, int32_t endIndex);

public:
    // Lifecycle
    SessionImpl(uint64_t id, std::shared_ptr<IGrammar> gram);
    ~SessionImpl();

    // Reference counting
    void retain();
    void release();
    uint32_t getRefCount() const { return referenceCount; }

    // Queries
    uint64_t getSessionId() const { return sessionId; }
    uint64_t getCreatedAtMs() const { return createdAtMs; }
    uint64_t getLastAccessMs() const { return lastAccessMs; }
    bool isExpired(uint64_t currentTimeMs, uint64_t maxAgeMs) const;

    // State management
    int32_t setLines(const std::vector<std::string>& newLines);
    int32_t getLineCount() const { return static_cast<int32_t>(lines.size()); }

    // Incremental tokenization operations
    int32_t edit(
        const std::vector<std::string>& newLines,
        int32_t startIndex,
        int32_t replaceCount
    );

    int32_t add(
        const std::vector<std::string>& newLines,
        int32_t insertIndex
    );

    int32_t remove(
        int32_t startIndex,
        int32_t removeCount
    );

    // Query operations
    const SessionLine* getLine(int32_t lineIndex) const;
    const std::vector<IToken>* getLineTokens(int32_t lineIndex) const;
    StateStack* getLineState(int32_t lineIndex) const;

    // Batch query
    void getTokensRange(
        int32_t startIndex,
        int32_t endIndex,
        std::vector<SessionLine>& results
    ) const;

    // Maintenance operations
    void invalidateRange(int32_t startIndex, int32_t endIndex);
    void clearCache();
    SessionMetadata getMetadata() const;

private:
    void updateAccessTime();
    uint64_t calculateMemoryUsage() const;
    int32_t countCachedLines() const;
};

// Global session manager with reference counting
class SessionManager {
private:
    static std::map<uint64_t, std::shared_ptr<SessionImpl>> sessions;
    static uint64_t nextSessionId;
    static uint32_t operationCount;
    static const uint32_t CLEANUP_INTERVAL = 100;

public:
    // Session lifecycle
    static uint64_t createSession(std::shared_ptr<IGrammar> grammar);
    static std::shared_ptr<SessionImpl> getSession(uint64_t sessionId);
    static void retainSession(uint64_t sessionId);
    static void releaseSession(uint64_t sessionId);
    static void disposeSession(uint64_t sessionId);

    // Maintenance
    static void cleanupExpired(int32_t maxAgeMs);
    static void triggerPeriodicCleanup(int32_t maxAgeMs);

    // Debug/monitoring
    static size_t getSessionCount();
};

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_SESSION_H
