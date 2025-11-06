#include <gtest/gtest.h>
#include "../src/syntax_highlighter.h"
#include "../src/main.h"
#include <memory>

namespace tml {

class SyntaxHighlighterTest : public ::testing::Test {
protected:
    std::shared_ptr<Registry> registry;
    std::shared_ptr<IGrammar> jsGrammar;
    Theme* theme;

    void SetUp() override {
        // Initialize registry with async oniguruma
        // This test assumes oniguruma is already loaded
        // In production, you'd need to load the wasm module first
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// Test basic construction and destruction
TEST_F(SyntaxHighlighterTest, DISABLED_ConstructionAndDestruction) {
    // This test is disabled because it requires full setup
    // In a real scenario, you'd load a grammar and theme first

    // Example of how to use:
    // auto highlighter = std::make_shared<SyntaxHighlighter>(jsGrammar, theme);
    // EXPECT_NE(highlighter, nullptr);
}

// Test document management
TEST_F(SyntaxHighlighterTest, DISABLED_DocumentManagement) {
    // Example test structure for document management
    // auto highlighter = std::make_shared<SyntaxHighlighter>(jsGrammar, theme);
    // std::vector<std::string> lines = {"const x = 5;", "console.log(x);"};
    // highlighter->setDocument(lines);
    // EXPECT_EQ(highlighter->getLineCount(), 2);
}

// Test single line highlighting
TEST_F(SyntaxHighlighterTest, DISABLED_SingleLineHighlighting) {
    // Example test for single line highlighting
    // auto highlighter = std::make_shared<SyntaxHighlighter>(jsGrammar, theme);
    // std::vector<std::string> lines = {"const x = 5;"};
    // highlighter->setDocument(lines);
    // auto highlighted = highlighter->getHighlightedLine(0);
    // EXPECT_EQ(highlighted.lineIndex, 0);
    // EXPECT_FALSE(highlighted.tokens.empty());
}

// Test incremental edits
TEST_F(SyntaxHighlighterTest, DISABLED_IncrementalEdits) {
    // Example test for incremental editing
    // auto highlighter = std::make_shared<SyntaxHighlighter>(jsGrammar, theme);
    // std::vector<std::string> lines = {"const x = 5;"};
    // highlighter->setDocument(lines);
    // highlighter->editLine(0, "const y = 10;");
    // EXPECT_EQ(highlighter->getLineCount(), 1);
}

// Test theme switching
TEST_F(SyntaxHighlighterTest, DISABLED_ThemeSwitching) {
    // Example test for theme switching
    // auto highlighter = std::make_shared<SyntaxHighlighter>(jsGrammar, theme);
    // Theme* newTheme = nullptr;  // Would load a different theme
    // highlighter->setTheme(newTheme);
    // EXPECT_EQ(highlighter->getTheme(), newTheme);
}

// Test cache functionality
TEST_F(SyntaxHighlighterTest, DISABLED_CacheFunctionality) {
    // Example test for caching
    // auto highlighter = std::make_shared<SyntaxHighlighter>(jsGrammar, theme, true);
    // std::vector<std::string> lines = {"const x = 5;"};
    // highlighter->setDocument(lines);
    // auto line1 = highlighter->getHighlightedLine(0);
    // highlighter->clearCache();
    // auto line2 = highlighter->getHighlightedLine(0);
    // EXPECT_EQ(line1.content, line2.content);
}

// Test HighlighterCache
TEST(HighlighterCacheTest, CacheInsertAndRetrieve) {
    HighlighterCache cache;
    HighlightedLine line;
    line.lineIndex = 0;
    line.content = "test";
    line.isComplete = true;

    cache.cacheLine(line, 1);
    HighlightedLine* cached = cache.getCachedLine(0, 1);

    EXPECT_NE(cached, nullptr);
    EXPECT_EQ(cached->lineIndex, 0);
    EXPECT_EQ(cached->content, "test");
}

TEST(HighlighterCacheTest, CacheVersionValidation) {
    HighlighterCache cache;
    HighlightedLine line;
    line.lineIndex = 0;
    line.content = "test";

    cache.cacheLine(line, 1);

    // Should find with correct version
    EXPECT_NE(cache.getCachedLine(0, 1), nullptr);

    // Should not find with wrong version
    EXPECT_EQ(cache.getCachedLine(0, 2), nullptr);
}

TEST(HighlighterCacheTest, CacheInvalidation) {
    HighlighterCache cache;
    HighlightedLine line;
    line.lineIndex = 0;
    line.content = "test";

    cache.cacheLine(line, 1);
    EXPECT_NE(cache.getCachedLine(0, 1), nullptr);

    cache.invalidateLine(0);
    EXPECT_EQ(cache.getCachedLine(0, 1), nullptr);
}

TEST(HighlighterCacheTest, CacheRangeInvalidation) {
    HighlighterCache cache;

    for (int i = 0; i < 5; ++i) {
        HighlightedLine line;
        line.lineIndex = i;
        line.content = "test" + std::to_string(i);
        cache.cacheLine(line, 1);
    }

    cache.invalidateRange(1, 3);

    // Lines 0 and 4 should still be cached
    EXPECT_NE(cache.getCachedLine(0, 1), nullptr);
    EXPECT_NE(cache.getCachedLine(4, 1), nullptr);

    // Lines 1, 2, 3 should be invalidated
    EXPECT_EQ(cache.getCachedLine(1, 1), nullptr);
    EXPECT_EQ(cache.getCachedLine(2, 1), nullptr);
    EXPECT_EQ(cache.getCachedLine(3, 1), nullptr);
}

TEST(HighlighterCacheTest, CacheClear) {
    HighlighterCache cache;

    for (int i = 0; i < 5; ++i) {
        HighlightedLine line;
        line.lineIndex = i;
        line.content = "test" + std::to_string(i);
        cache.cacheLine(line, 1);
    }

    EXPECT_EQ(cache.getCachedLineCount(), 5);

    cache.clear();
    EXPECT_EQ(cache.getCachedLineCount(), 0);
}

TEST(HighlightedTokenTest, BasicStructure) {
    HighlightedToken token;
    token.startIndex = 0;
    token.endIndex = 5;
    token.scopes.push_back("source.js");
    token.foregroundColor = "#FF0000";
    token.fontStyle = static_cast<int>(FontStyle::Bold);
    token.tokenType = StandardTokenType::String;

    EXPECT_EQ(token.startIndex, 0);
    EXPECT_EQ(token.endIndex, 5);
    EXPECT_EQ(token.scopes.size(), 1);
    EXPECT_EQ(token.scopes[0], "source.js");
    EXPECT_EQ(token.foregroundColor, "#FF0000");
    EXPECT_EQ(token.fontStyle, static_cast<int>(FontStyle::Bold));
    EXPECT_EQ(token.tokenType, StandardTokenType::String);
}

TEST(HighlightedLineTest, BasicStructure) {
    HighlightedLine line;
    line.lineIndex = 5;
    line.content = "const x = 5;";
    line.isComplete = true;
    line.version = 1;

    EXPECT_EQ(line.lineIndex, 5);
    EXPECT_EQ(line.content, "const x = 5;");
    EXPECT_TRUE(line.isComplete);
    EXPECT_EQ(line.version, 1);
}

} // namespace tml
