#include "session.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <vector>
#include <stdint.h>

namespace vscode_textmate {

// ============================================================================
// SessionLine Implementation
// ============================================================================

SessionLine::~SessionLine() {
    if (state != nullptr) {
        // State stacks are managed by the grammar/tokenizer
        // They should be released through the proper mechanism
        state = nullptr;
    }
}

// ============================================================================
// TextMateSession Implementation
// ============================================================================

SessionImpl::SessionImpl(uint64_t id, std::shared_ptr<IGrammar> gram)
    : sessionId(id),
      grammar(gram),
      referenceCount(1),
      nextVersion(1) {
    auto now = std::chrono::system_clock::now();
    createdAtMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    lastAccessMs = createdAtMs;
}

SessionImpl::~SessionImpl() {
    clearCache();
}

void SessionImpl::retain() {
    referenceCount++;
}

void SessionImpl::release() {
    if (referenceCount > 0) {
        referenceCount--;
    }
}

void SessionImpl::updateAccessTime() {
    auto now = std::chrono::system_clock::now();
    lastAccessMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
}

bool SessionImpl::isExpired(uint64_t currentTimeMs, uint64_t maxAgeMs) const {
    return (currentTimeMs - createdAtMs) > maxAgeMs;
}

bool SessionImpl::isStateEqual(StateStack* state1, StateStack* state2) const {
    if (state1 == state2) {
        return true;
    }
    if (state1 == nullptr || state2 == nullptr) {
        return false;
    }
    // Compare state stacks by converting to string representation
    // This is a simple comparison; more sophisticated comparison may be needed
    return std::string(reinterpret_cast<char*>(state1)) ==
           std::string(reinterpret_cast<char*>(state2));
}

void SessionImpl::invalidateFrom(int32_t startIndex) {
    if (startIndex < 0 || startIndex >= static_cast<int32_t>(lines.size())) {
        return;
    }

    for (int32_t i = startIndex; i < static_cast<int32_t>(lines.size()); i++) {
        lines[i].cached = false;
        lines[i].tokens.clear();
        if (lines[i].state != nullptr) {
            lines[i].state = nullptr;
        }
        lines[i].version = nextVersion;
    }
    nextVersion++;
}

int32_t SessionImpl::setLines(const std::vector<std::string>& newLines) {
    updateAccessTime();

    // Clear existing lines
    clearCache();
    lines.clear();

    // Initialize new lines
    lines.resize(newLines.size());
    for (size_t i = 0; i < newLines.size(); i++) {
        lines[i].content = newLines[i];
        lines[i].cached = false;
        lines[i].state = nullptr;
        lines[i].version = nextVersion;
    }

    // Tokenize all lines with initial state
    retokenizeLines(0, static_cast<int32_t>(lines.size()) - 1);

    return 0;
}

void SessionImpl::retokenizeLines(int32_t startIndex, int32_t endIndex) {
    if (!grammar || startIndex < 0 || startIndex > endIndex) {
        return;
    }

    endIndex = std::min(endIndex, static_cast<int32_t>(lines.size()) - 1);

    // Get initial state for first line to retokenize
    StateStack* state = nullptr;
    if (startIndex > 0 && lines[startIndex - 1].cached && lines[startIndex - 1].state) {
        state = lines[startIndex - 1].state;
    }

    // Retokenize with early stopping when state stabilizes
    for (int32_t i = startIndex; i <= endIndex; i++) {
        ITokenizeLineResult result = grammar->tokenizeLine(lines[i].content, state);

        lines[i].tokens = result.tokens;
        lines[i].state = result.ruleStack;
        lines[i].cached = true;
        lines[i].version = nextVersion;

        // Check if we should stop cascading (state has stabilized)
        if (i < static_cast<int32_t>(lines.size()) - 1) {
            if (lines[i + 1].cached &&
                isStateEqual(result.ruleStack, lines[i + 1].state)) {
                // State matches expected, can stop here
                break;
            }
        }

        state = result.ruleStack;
    }
}

int32_t SessionImpl::edit(
    const std::vector<std::string>& newLines,
    int32_t startIndex,
    int32_t replaceCount
) {
    updateAccessTime();

    // Validate parameters
    if (startIndex < 0 || startIndex > static_cast<int32_t>(lines.size())) {
        return 1;  // Error
    }

    int32_t endIndex = startIndex + replaceCount - 1;
    if (endIndex >= static_cast<int32_t>(lines.size())) {
        endIndex = static_cast<int32_t>(lines.size()) - 1;
    }

    // Replace lines in the buffer
    if (replaceCount > 0 && startIndex < static_cast<int32_t>(lines.size())) {
        for (int32_t i = 0; i < static_cast<int32_t>(newLines.size()); i++) {
            int32_t targetIdx = startIndex + i;
            if (targetIdx < static_cast<int32_t>(lines.size())) {
                lines[targetIdx].content = newLines[i];
            }
        }
    }

    // Invalidate cache from start
    invalidateFrom(startIndex);

    // Retokenize starting from the edited line
    if (startIndex < static_cast<int32_t>(lines.size())) {
        retokenizeLines(startIndex, static_cast<int32_t>(lines.size()) - 1);
    }

    return 0;
}

int32_t SessionImpl::add(
    const std::vector<std::string>& newLines,
    int32_t insertIndex
) {
    updateAccessTime();

    // Validate parameters
    if (insertIndex < 0 || insertIndex > static_cast<int32_t>(lines.size())) {
        return 1;  // Error
    }

    // Insert new lines at the specified position
    lines.insert(
        lines.begin() + insertIndex,
        SessionLine()
    );

    for (size_t i = 0; i < newLines.size(); i++) {
        int32_t targetIdx = insertIndex + i;
        if (targetIdx < static_cast<int32_t>(lines.size())) {
            lines[targetIdx].content = newLines[i];
            lines[targetIdx].cached = false;
            lines[targetIdx].state = nullptr;
        }
    }

    // Invalidate cache from insertion point
    invalidateFrom(insertIndex);

    // Retokenize from insertion point
    if (insertIndex < static_cast<int32_t>(lines.size())) {
        retokenizeLines(insertIndex, static_cast<int32_t>(lines.size()) - 1);
    }

    return 0;
}

int32_t SessionImpl::remove(
    int32_t startIndex,
    int32_t removeCount
) {
    updateAccessTime();

    // Validate parameters
    if (startIndex < 0 || startIndex >= static_cast<int32_t>(lines.size()) ||
        removeCount <= 0) {
        return 1;  // Error
    }

    int32_t endIndex = std::min(
        static_cast<int32_t>(lines.size()),
        startIndex + removeCount
    );

    // Remove lines from buffer
    lines.erase(lines.begin() + startIndex, lines.begin() + endIndex);

    // Invalidate cache from removal point
    if (startIndex < static_cast<int32_t>(lines.size())) {
        invalidateFrom(startIndex);

        // Retokenize from removal point
        retokenizeLines(startIndex, static_cast<int32_t>(lines.size()) - 1);
    }

    return 0;
}

const SessionLine* SessionImpl::getLine(int32_t lineIndex) const {
    if (lineIndex < 0 || lineIndex >= static_cast<int32_t>(lines.size())) {
        return nullptr;
    }
    return &lines[lineIndex];
}

const std::vector<IToken>* SessionImpl::getLineTokens(int32_t lineIndex) const {
    const SessionLine* line = getLine(lineIndex);
    if (line == nullptr || !line->cached) {
        return nullptr;
    }
    return &line->tokens;
}

StateStack* SessionImpl::getLineState(int32_t lineIndex) const {
    const SessionLine* line = getLine(lineIndex);
    if (line == nullptr || !line->cached) {
        return nullptr;
    }
    return line->state;
}

void SessionImpl::getTokensRange(
    int32_t startIndex,
    int32_t endIndex,
    std::vector<SessionLine>& results
) const {
    results.clear();

    if (startIndex < 0 || endIndex >= static_cast<int32_t>(lines.size()) ||
        startIndex > endIndex) {
        return;
    }

    for (int32_t i = startIndex; i <= endIndex; i++) {
        if (i < static_cast<int32_t>(lines.size())) {
            results.push_back(lines[i]);
        }
    }
}

void SessionImpl::invalidateRange(int32_t startIndex, int32_t endIndex) {
    updateAccessTime();

    if (startIndex < 0 || startIndex >= static_cast<int32_t>(lines.size())) {
        return;
    }

    if (endIndex < 0 || endIndex >= static_cast<int32_t>(lines.size())) {
        endIndex = static_cast<int32_t>(lines.size()) - 1;
    }

    for (int32_t i = startIndex; i <= endIndex; i++) {
        lines[i].cached = false;
        lines[i].tokens.clear();
        if (lines[i].state != nullptr) {
            lines[i].state = nullptr;
        }
        lines[i].version = nextVersion;
    }
    nextVersion++;

    // Retokenize the range
    if (startIndex < static_cast<int32_t>(lines.size())) {
        retokenizeLines(startIndex, endIndex);
    }
}

void SessionImpl::clearCache() {
    for (auto& line : lines) {
        line.cached = false;
        line.tokens.clear();
        if (line.state != nullptr) {
            line.state = nullptr;
        }
    }
    nextVersion++;
}

uint64_t SessionImpl::calculateMemoryUsage() const {
    uint64_t usage = sizeof(SessionImpl);

    // Account for lines vector
    usage += lines.capacity() * sizeof(SessionLine);

    // Account for token data
    for (const auto& line : lines) {
        usage += line.content.capacity();
        usage += line.tokens.capacity() * sizeof(IToken);
        for (const auto& token : line.tokens) {
            for (const auto& scope : token.scopes) {
                usage += scope.capacity();
            }
        }
    }

    return usage;
}

int32_t SessionImpl::countCachedLines() const {
    int32_t count = 0;
    for (const auto& line : lines) {
        if (line.cached) {
            count++;
        }
    }
    return count;
}

SessionMetadata SessionImpl::getMetadata() const {
    SessionMetadata metadata;
    metadata.createdAtMs = createdAtMs;
    metadata.referenceCount = referenceCount;
    metadata.lineCount = static_cast<int32_t>(lines.size());
    metadata.cachedLineCount = countCachedLines();
    metadata.memoryUsageBytes = calculateMemoryUsage();
    return metadata;
}

// ============================================================================
// SessionManager Implementation
// ============================================================================

std::map<uint64_t, std::shared_ptr<SessionImpl>> SessionManager::sessions;
uint64_t SessionManager::nextSessionId = 1;
uint32_t SessionManager::operationCount = 0;

uint64_t SessionManager::createSession(std::shared_ptr<IGrammar> grammar) {
    if (!grammar) {
        return 0;
    }

    uint64_t sessionId = nextSessionId++;
    auto session = std::make_shared<SessionImpl>(sessionId, grammar);
    sessions[sessionId] = session;

    // Periodic cleanup
    triggerPeriodicCleanup(60000);  // 60 seconds

    return sessionId;
}

std::shared_ptr<SessionImpl> SessionManager::getSession(uint64_t sessionId) {
    auto it = sessions.find(sessionId);
    if (it != sessions.end()) {
        return it->second;
    }
    return nullptr;
}

void SessionManager::retainSession(uint64_t sessionId) {
    auto session = getSession(sessionId);
    if (session) {
        session->retain();
    }
}

void SessionManager::releaseSession(uint64_t sessionId) {
    auto session = getSession(sessionId);
    if (session) {
        session->release();
    }
}

void SessionManager::disposeSession(uint64_t sessionId) {
    auto it = sessions.find(sessionId);
    if (it != sessions.end()) {
        it->second->release();
        if (it->second->getRefCount() == 0) {
            sessions.erase(it);
        }
    }
}

void SessionManager::cleanupExpired(int32_t maxAgeMs) {
    auto now = std::chrono::system_clock::now();
    uint64_t currentTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    for (auto it = sessions.begin(); it != sessions.end();) {
        if (it->second->isExpired(currentTimeMs, maxAgeMs)) {
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
}

void SessionManager::triggerPeriodicCleanup(int32_t maxAgeMs) {
    operationCount++;
    if (operationCount % CLEANUP_INTERVAL == 0) {
        cleanupExpired(maxAgeMs);
    }
}

size_t SessionManager::getSessionCount() {
    return sessions.size();
}

} // namespace vscode_textmate

// ============================================================================
// C API Wrapper Layer (extern "C")
// ============================================================================

// Forward declare the TEXTMATE_API macro and opaque C types from c_api.h
#ifdef _WIN32
    #ifdef TEXTMATE_EXPORTS
        #define TEXTMATE_API __declspec(dllexport)
    #else
        #define TEXTMATE_API __declspec(dllimport)
    #endif
#else
    #define TEXTMATE_API __attribute__((visibility("default")))
#endif

// Opaque types from c_api.h (not defining TextMateSession yet)
typedef void* TextMateGrammar;
typedef void* TextMateStateStack;
typedef void* TextMateOnigLib;
typedef void* TextMateTheme;

// Token structure
typedef struct {
    int32_t startIndex;
    int32_t endIndex;
    int32_t scopeDepth;
    char** scopes;
} TextMateToken;

// Tokenize result structure
typedef struct {
    TextMateToken* tokens;
    int32_t tokenCount;
    TextMateStateStack ruleStack;
    int32_t stoppedEarly;
} TextMateTokenizeResult;

// Session opaque handle
typedef uint64_t TextMateSession;

// Session line representation
typedef struct {
    TextMateToken* tokens;
    int32_t tokenCount;
    TextMateStateStack state;
    uint64_t version;
} TextMateSessionLine;

// Session lines result
typedef struct {
    TextMateSessionLine* lines;
    int32_t lineCount;
} TextMateSessionLinesResult;

// Session metadata
typedef struct {
    uint64_t createdAtMs;
    uint32_t referenceCount;
    int32_t lineCount;
    int32_t cachedLineCount;
    uint64_t memoryUsageBytes;
} TextMateSessionMetadata;

extern "C" {

using namespace vscode_textmate;

// ============================================================================
// Session Lifecycle (from session_c_api.h)
// ============================================================================

TEXTMATE_API TextMateSession textmate_session_create(TextMateGrammar grammar) {
    if (!grammar) {
        return 0;
    }

    // Grammar is a void* that we need to cast to IGrammar*
    auto grammarPtr = static_cast<IGrammar*>(grammar);
    auto grammarSharedPtr = std::shared_ptr<IGrammar>(grammarPtr, [](IGrammar*) {
        // Don't delete - ownership remains with the registry
    });

    return SessionManager::createSession(grammarSharedPtr);
}

TEXTMATE_API void textmate_session_retain(TextMateSession session) {
    if (session == 0) return;
    SessionManager::retainSession(session);
}

TEXTMATE_API void textmate_session_release(TextMateSession session) {
    if (session == 0) return;
    SessionManager::releaseSession(session);
}

TEXTMATE_API void textmate_session_dispose(TextMateSession session) {
    if (session == 0) return;
    SessionManager::disposeSession(session);
}

// ============================================================================
// Session State Management
// ============================================================================

TEXTMATE_API int textmate_session_set_lines(
    TextMateSession session,
    const char** lines,
    int32_t lineCount
) {
    if (session == 0 || !lines || lineCount < 0) {
        return 1;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return 1;
    }

    std::vector<std::string> lineVec;
    for (int32_t i = 0; i < lineCount; i++) {
        lineVec.push_back(lines[i] ? std::string(lines[i]) : std::string(""));
    }

    return sessionPtr->setLines(lineVec);
}

TEXTMATE_API int32_t textmate_session_get_line_count(TextMateSession session) {
    if (session == 0) {
        return 0;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return 0;
    }

    return sessionPtr->getLineCount();
}

// ============================================================================
// Incremental Tokenization Operations
// ============================================================================

TEXTMATE_API int textmate_session_edit(
    TextMateSession session,
    const char** lines,
    int32_t lineCount,
    int32_t startIndex,
    int32_t replaceCount
) {
    if (session == 0 || !lines || lineCount < 0) {
        return 1;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return 1;
    }

    std::vector<std::string> lineVec;
    for (int32_t i = 0; i < lineCount; i++) {
        lineVec.push_back(lines[i] ? std::string(lines[i]) : std::string(""));
    }

    return sessionPtr->edit(lineVec, startIndex, replaceCount);
}

TEXTMATE_API int textmate_session_add(
    TextMateSession session,
    const char** lines,
    int32_t lineCount,
    int32_t insertIndex
) {
    if (session == 0 || !lines || lineCount < 0) {
        return 1;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return 1;
    }

    std::vector<std::string> lineVec;
    for (int32_t i = 0; i < lineCount; i++) {
        lineVec.push_back(lines[i] ? std::string(lines[i]) : std::string(""));
    }

    return sessionPtr->add(lineVec, insertIndex);
}

TEXTMATE_API int textmate_session_remove(
    TextMateSession session,
    int32_t startIndex,
    int32_t removeCount
) {
    if (session == 0) {
        return 1;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return 1;
    }

    return sessionPtr->remove(startIndex, removeCount);
}

// ============================================================================
// Query Operations
// ============================================================================

TEXTMATE_API TextMateTokenizeResult* textmate_session_get_line_tokens(
    TextMateSession session,
    int32_t lineIndex
) {
    if (session == 0) {
        return nullptr;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return nullptr;
    }

    const auto* tokens = sessionPtr->getLineTokens(lineIndex);
    if (!tokens) {
        return nullptr;
    }

    // Allocate result structure
    auto* result = new TextMateTokenizeResult();
    result->tokenCount = static_cast<int32_t>(tokens->size());

    if (result->tokenCount > 0) {
        result->tokens = new TextMateToken[result->tokenCount];

        for (int32_t i = 0; i < result->tokenCount; i++) {
            const auto& token = (*tokens)[i];
            result->tokens[i].startIndex = token.startIndex;
            result->tokens[i].endIndex = token.endIndex;
            result->tokens[i].scopeDepth = static_cast<int32_t>(token.scopes.size());

            // Allocate scope array
            result->tokens[i].scopes = new char*[token.scopes.size()];
            for (size_t j = 0; j < token.scopes.size(); j++) {
                size_t scopeLen = token.scopes[j].length();
                result->tokens[i].scopes[j] = new char[scopeLen + 1];
                std::strcpy(result->tokens[i].scopes[j], token.scopes[j].c_str());
            }
        }
    } else {
        result->tokens = nullptr;
    }

    result->ruleStack = sessionPtr->getLineState(lineIndex);
    result->stoppedEarly = 0;

    return result;
}

TEXTMATE_API TextMateStateStack textmate_session_get_line_state(
    TextMateSession session,
    int32_t lineIndex
) {
    if (session == 0) {
        return nullptr;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return nullptr;
    }

    return sessionPtr->getLineState(lineIndex);
}

TEXTMATE_API TextMateSessionLinesResult* textmate_session_get_tokens_range(
    TextMateSession session,
    int32_t startIndex,
    int32_t endIndex
) {
    if (session == 0) {
        return nullptr;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return nullptr;
    }

    auto* result = new TextMateSessionLinesResult();
    std::vector<SessionLine> lines;
    sessionPtr->getTokensRange(startIndex, endIndex, lines);

    result->lineCount = static_cast<int32_t>(lines.size());

    if (result->lineCount > 0) {
        result->lines = new TextMateSessionLine[result->lineCount];

        for (int32_t i = 0; i < result->lineCount; i++) {
            const auto& line = lines[i];
            result->lines[i].tokenCount = static_cast<int32_t>(line.tokens.size());
            result->lines[i].state = line.state;
            result->lines[i].version = line.version;

            if (result->lines[i].tokenCount > 0) {
                result->lines[i].tokens = new TextMateToken[result->lines[i].tokenCount];

                for (int32_t j = 0; j < result->lines[i].tokenCount; j++) {
                    const auto& token = line.tokens[j];
                    result->lines[i].tokens[j].startIndex = token.startIndex;
                    result->lines[i].tokens[j].endIndex = token.endIndex;
                    result->lines[i].tokens[j].scopeDepth = static_cast<int32_t>(token.scopes.size());

                    result->lines[i].tokens[j].scopes = new char*[token.scopes.size()];
                    for (size_t k = 0; k < token.scopes.size(); k++) {
                        size_t scopeLen = token.scopes[k].length();
                        result->lines[i].tokens[j].scopes[k] = new char[scopeLen + 1];
                        std::strcpy(result->lines[i].tokens[j].scopes[k], token.scopes[k].c_str());
                    }
                }
            } else {
                result->lines[i].tokens = nullptr;
            }
        }
    } else {
        result->lines = nullptr;
    }

    return result;
}

TEXTMATE_API void textmate_session_free_tokens_result(
    TextMateTokenizeResult* result
) {
    if (!result) {
        return;
    }

    if (result->tokens) {
        for (int32_t i = 0; i < result->tokenCount; i++) {
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

TEXTMATE_API void textmate_session_free_lines_result(
    TextMateSessionLinesResult* result
) {
    if (!result) {
        return;
    }

    if (result->lines) {
        for (int32_t i = 0; i < result->lineCount; i++) {
            if (result->lines[i].tokens) {
                for (int32_t j = 0; j < result->lines[i].tokenCount; j++) {
                    if (result->lines[i].tokens[j].scopes) {
                        for (int k = 0; k < result->lines[i].tokens[j].scopeDepth; k++) {
                            delete[] result->lines[i].tokens[j].scopes[k];
                        }
                        delete[] result->lines[i].tokens[j].scopes;
                    }
                }
                delete[] result->lines[i].tokens;
            }
        }
        delete[] result->lines;
    }

    delete result;
}

// ============================================================================
// Maintenance Operations
// ============================================================================

TEXTMATE_API void textmate_session_invalidate_range(
    TextMateSession session,
    int32_t startIndex,
    int32_t endIndex
) {
    if (session == 0) {
        return;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return;
    }

    sessionPtr->invalidateRange(startIndex, endIndex);
}

TEXTMATE_API void textmate_session_clear_cache(TextMateSession session) {
    if (session == 0) {
        return;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return;
    }

    sessionPtr->clearCache();
}

TEXTMATE_API void textmate_session_cleanup_expired(int32_t maxAgeMs) {
    SessionManager::cleanupExpired(maxAgeMs);
}

TEXTMATE_API TextMateSessionMetadata textmate_session_get_metadata(
    TextMateSession session
) {
    TextMateSessionMetadata metadata = {0, 0, 0, 0, 0};

    if (session == 0) {
        return metadata;
    }

    auto sessionPtr = SessionManager::getSession(session);
    if (!sessionPtr) {
        return metadata;
    }

    auto cppMetadata = sessionPtr->getMetadata();
    metadata.createdAtMs = cppMetadata.createdAtMs;
    metadata.referenceCount = cppMetadata.referenceCount;
    metadata.lineCount = cppMetadata.lineCount;
    metadata.cachedLineCount = cppMetadata.cachedLineCount;
    metadata.memoryUsageBytes = cppMetadata.memoryUsageBytes;

    return metadata;
}

} // extern "C"
