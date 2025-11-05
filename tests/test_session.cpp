#include "../src/session.h"
#include "../src/main.h"
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

using namespace tml;
using namespace rapidjson;

// ============================================================================
// Helper Functions
// ============================================================================

// Read file content
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Parse JSON file
Document parseJSONFile(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    IStreamWrapper isw(ifs);
    Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError()) {
        throw std::runtime_error("JSON parse error in file: " + filename);
    }
    return doc;
}

// Grammar holder for test setup
class TestGrammarHolder {
private:
    std::string basePath;
    std::map<std::string, IRawGrammar*> grammarByScope;

public:
    TestGrammarHolder(const std::string& path) : basePath(path) {}

    ~TestGrammarHolder() {
        for (auto& pair : grammarByScope) {
            delete pair.second;
        }
        grammarByScope.clear();
    }

    std::string loadGrammar(const std::string& grammarPath) {
        std::string fullPath = basePath + "/" + grammarPath + ".json";
        try {
            std::string content = readFile(fullPath);
            auto rawGrammar = parseRawGrammar(content, &fullPath);

            if (rawGrammar) {
                std::string scopeName = rawGrammar->scopeName;
                grammarByScope[scopeName] = rawGrammar;
                return scopeName;
            }
            return "";
        } catch (...) {
            return "";
        }
    }

    IRawGrammar* getGrammarByScope(const std::string& scopeName) {
        auto it = grammarByScope.find(scopeName);
        if (it != grammarByScope.end()) {
            return it->second;
        }
        return nullptr;
    }
};

// ============================================================================
// Fixture for Session Tests
// ============================================================================

class SessionTest : public ::testing::Test {
protected:
    IOnigLib* onigLib = nullptr;
    std::shared_ptr<IGrammar> testGrammar;

    void SetUp() override {
        // Initialize Oniguruma
        onigLib = new DefaultOnigLib();
        ASSERT_TRUE(onigLib != nullptr);

        // Create a simple test grammar
        try {
            // Try to load test case from multiple possible locations
            std::vector<std::string> possiblePaths = {
                "test-cases/first-mate/tests.json",
                "../../test-cases/first-mate/tests.json",
                "../test-cases/first-mate/tests.json"
            };

            Document doc;
            bool loaded = false;

            for (const auto& path : possiblePaths) {
                try {
                    doc = parseJSONFile(path);
                    loaded = true;
                    break;
                } catch (...) {
                    continue;
                }
            }

            if (loaded && doc.IsArray() && doc.Size() > 0) {
                const Value& firstTest = doc[0];
                if (firstTest.HasMember("grammars")) {
                    const Value& grammars = firstTest["grammars"];
                    if (grammars.Size() > 0) {
                        std::string grammarPath = grammars[0].GetString();

                        // Try to load grammar from multiple possible locations
                        std::vector<std::string> possibleBasePaths = {
                            "test-cases/first-mate/fixtures",
                            "../../test-cases/first-mate/fixtures",
                            "../test-cases/first-mate/fixtures"
                        };

                        TestGrammarHolder holder(possibleBasePaths[0]);
                        std::string scopeName = holder.loadGrammar(grammarPath);

                        RegistryOptions options;
                        options.onigLib = onigLib;
                        options.loadGrammar = [&holder](const ScopeName& scopeName) -> IRawGrammar* {
                            return holder.getGrammarByScope(scopeName);
                        };

                        Registry registry(options);
                        auto grammar = registry.loadGrammar(scopeName);
                        if (grammar) {
                            testGrammar = testGrammar;
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to set up test grammar: " << e.what() << std::endl;
            // Continue with tests - some tests don't need grammar
        }
    }

    void TearDown() override {
        testGrammar.reset();
        if (onigLib) {
            delete onigLib;
            onigLib = nullptr;
        }
    }
};

// ============================================================================
// Test Cases: Session Lifecycle
// ============================================================================

TEST_F(SessionTest, CreateSessionReturnsValidHandle) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    EXPECT_NE(sessionId, 0);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, CreateSessionWithNullGrammarReturnsZero) {
    auto sessionId = SessionManager::createSession(nullptr);
    EXPECT_EQ(sessionId, 0);
}

TEST_F(SessionTest, GetSessionReturnsValidPointer) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    EXPECT_TRUE(session != nullptr);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, GetSessionWithInvalidIdReturnsNull) {
    auto session = SessionManager::getSession(99999);
    EXPECT_TRUE(session == nullptr);
}

TEST_F(SessionTest, ReferenceCountingWorks) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    EXPECT_EQ(session->getRefCount(), 1);

    SessionManager::retainSession(sessionId);
    EXPECT_EQ(session->getRefCount(), 2);

    SessionManager::releaseSession(sessionId);
    EXPECT_EQ(session->getRefCount(), 1);

    SessionManager::disposeSession(sessionId);
}

// ============================================================================
// Test Cases: State Management
// ============================================================================

TEST_F(SessionTest, SetLinesInitializesDocument) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2", "line 3"};
    int result = session->setLines(lines);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 3);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, SetLinesWithEmptyDocumentWorks) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines;
    int result = session->setLines(lines);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 0);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, SetLinesCachesTokens) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"package main"};
    session->setLines(lines);

    const auto* tokens = session->getLineTokens(0);
    EXPECT_TRUE(tokens != nullptr);
    EXPECT_GT(tokens->size(), 0);

    SessionManager::disposeSession(sessionId);
}

// ============================================================================
// Test Cases: Edit Operations
// ============================================================================

TEST_F(SessionTest, EditLineReplacesContent) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2", "line 3"};
    session->setLines(lines);

    std::vector<std::string> newLine = {"modified line 1"};
    int result = session->edit(newLine, 0, 1);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 3);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, EditMultipleLinesWorks) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2", "line 3"};
    session->setLines(lines);

    std::vector<std::string> newLines = {"new 1", "new 2"};
    int result = session->edit(newLines, 0, 2);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 3);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, EditOutOfBoundsReturnsError) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2"};
    session->setLines(lines);

    std::vector<std::string> newLine = {"new"};
    int result = session->edit(newLine, 10, 1);
    EXPECT_NE(result, 0);

    SessionManager::disposeSession(sessionId);
}

// ============================================================================
// Test Cases: Add Operations
// ============================================================================

TEST_F(SessionTest, AddLinesIncreasesLineCount) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2"};
    session->setLines(lines);
    EXPECT_EQ(session->getLineCount(), 2);

    std::vector<std::string> newLines = {"new 1", "new 2"};
    int result = session->add(newLines, 1);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 4);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, AddAtBeginningWorks) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2"};
    session->setLines(lines);

    std::vector<std::string> newLines = {"new 1"};
    int result = session->add(newLines, 0);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 3);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, AddAtEndWorks) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2"};
    session->setLines(lines);

    std::vector<std::string> newLines = {"new 1"};
    int result = session->add(newLines, 2);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 3);

    SessionManager::disposeSession(sessionId);
}

// ============================================================================
// Test Cases: Remove Operations
// ============================================================================

TEST_F(SessionTest, RemoveLinesDecreasesLineCount) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2", "line 3"};
    session->setLines(lines);
    EXPECT_EQ(session->getLineCount(), 3);

    int result = session->remove(1, 1);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 2);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, RemoveMultipleLinesWorks) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2", "line 3", "line 4"};
    session->setLines(lines);

    int result = session->remove(1, 2);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 2);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, RemoveFirstLineWorks) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2", "line 3"};
    session->setLines(lines);

    int result = session->remove(0, 1);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 2);

    SessionManager::disposeSession(sessionId);
}

// ============================================================================
// Test Cases: Query Operations
// ============================================================================

TEST_F(SessionTest, GetLineTokensReturnsValidTokens) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"package main"};
    session->setLines(lines);

    const auto* tokens = session->getLineTokens(0);
    EXPECT_TRUE(tokens != nullptr);
    EXPECT_GT(tokens->size(), 0);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, GetLineTokensForUncachedLineReturnsNull) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1"};
    session->setLines(lines);

    const auto* tokens = session->getLineTokens(10);
    EXPECT_TRUE(tokens == nullptr);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, GetLineStateReturnsValidState) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1"};
    session->setLines(lines);

    auto state = session->getLineState(0);
    EXPECT_TRUE(state != nullptr);

    SessionManager::disposeSession(sessionId);
}

// ============================================================================
// Test Cases: Maintenance Operations
// ============================================================================

TEST_F(SessionTest, InvalidateRangeInvalidatesCache) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2", "line 3"};
    session->setLines(lines);

    auto tokensBeforeInvalidate = session->getLineTokens(0);
    EXPECT_TRUE(tokensBeforeInvalidate != nullptr);

    session->invalidateRange(0, 1);

    // After invalidation and retokenization, tokens should still be valid
    // but the version should have changed
    auto tokensAfterInvalidate = session->getLineTokens(0);
    EXPECT_TRUE(tokensAfterInvalidate != nullptr);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, ClearCacheClearsAllTokens) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2", "line 3"};
    session->setLines(lines);

    session->clearCache();

    // After clear cache, getting tokens will retokenize
    auto tokens = session->getLineTokens(0);
    EXPECT_TRUE(tokens != nullptr);

    SessionManager::disposeSession(sessionId);
}

// ============================================================================
// Test Cases: Metadata
// ============================================================================

TEST_F(SessionTest, GetMetadataReturnsValidInfo) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2"};
    session->setLines(lines);

    auto metadata = session->getMetadata();
    EXPECT_EQ(metadata.lineCount, 2);
    EXPECT_GT(metadata.referenceCount, 0);
    EXPECT_GT(metadata.createdAtMs, 0);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, MetadataReportsCachedLineCount) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2", "line 3"};
    session->setLines(lines);

    auto metadata = session->getMetadata();
    EXPECT_EQ(metadata.cachedLineCount, 3);

    SessionManager::disposeSession(sessionId);
}

// ============================================================================
// Test Cases: Complex Scenarios
// ============================================================================

TEST_F(SessionTest, SequenceOfEditsWorks) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"line 1", "line 2", "line 3"};
    session->setLines(lines);
    EXPECT_EQ(session->getLineCount(), 3);

    // Edit line 0
    std::vector<std::string> edit1 = {"edited 1"};
    session->edit(edit1, 0, 1);
    EXPECT_EQ(session->getLineCount(), 3);

    // Add lines
    std::vector<std::string> add1 = {"new 1", "new 2"};
    session->add(add1, 2);
    EXPECT_EQ(session->getLineCount(), 5);

    // Remove lines
    session->remove(1, 2);
    EXPECT_EQ(session->getLineCount(), 3);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, LargeDocumentHandling) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    // Create a large document
    std::vector<std::string> lines;
    for (int i = 0; i < 1000; i++) {
        lines.push_back("line " + std::to_string(i));
    }

    int result = session->setLines(lines);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 1000);

    // Edit a line in the middle
    std::vector<std::string> edit = {"edited middle"};
    result = session->edit(edit, 500, 1);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 1000);

    SessionManager::disposeSession(sessionId);
}

TEST_F(SessionTest, EmptyLinesAreHandled) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    std::vector<std::string> lines = {"", "line 2", "", "line 4"};
    int result = session->setLines(lines);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session->getLineCount(), 4);

    // Query empty line
    const auto* tokens = session->getLineTokens(0);
    EXPECT_TRUE(tokens != nullptr);

    SessionManager::disposeSession(sessionId);
}

// ============================================================================
// Test Cases: Memory Management
// ============================================================================

TEST_F(SessionTest, SessionCleanupReleasesProperly) {
    if (!testGrammar) { return; }

    size_t initialCount = SessionManager::getSessionCount();

    {
        auto sessionId = SessionManager::createSession(
            testGrammar
        );
        EXPECT_EQ(SessionManager::getSessionCount(), initialCount + 1);

        SessionManager::disposeSession(sessionId);
        EXPECT_EQ(SessionManager::getSessionCount(), initialCount);
    }
}

TEST_F(SessionTest, MultipleSessionsCanCoexist) {
    if (!testGrammar) { return; }

    auto sessionId1 = SessionManager::createSession(
        testGrammar
    );
    auto sessionId2 = SessionManager::createSession(
        testGrammar
    );

    EXPECT_NE(sessionId1, 0);
    EXPECT_NE(sessionId2, 0);
    EXPECT_NE(sessionId1, sessionId2);

    auto session1 = SessionManager::getSession(sessionId1);
    auto session2 = SessionManager::getSession(sessionId2);

    EXPECT_TRUE(session1 != nullptr);
    EXPECT_TRUE(session2 != nullptr);

    SessionManager::disposeSession(sessionId1);
    SessionManager::disposeSession(sessionId2);
}

TEST_F(SessionTest, SessionExpiryWorks) {
    if (!testGrammar) { return; }

    auto sessionId = SessionManager::createSession(
        testGrammar
    );
    ASSERT_NE(sessionId, 0);

    auto session = SessionManager::getSession(sessionId);
    ASSERT_TRUE(session != nullptr);

    // Check that session is not expired with large maxAge
    EXPECT_FALSE(session->isExpired(session->getCreatedAtMs() + 1000, 100));

    // Check that session would be expired with small maxAge
    EXPECT_TRUE(session->isExpired(session->getCreatedAtMs() + 100000, 10000));

    SessionManager::disposeSession(sessionId);
}
