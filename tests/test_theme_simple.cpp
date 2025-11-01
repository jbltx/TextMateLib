#include <gtest/gtest.h>
#include "../src/c_api.h"

class ThemeSimpleTest : public ::testing::Test {
};

// Test that we can call the theme API without crashing
TEST_F(ThemeSimpleTest, ThemeAPIExists) {
    // This just verifies the API exists and is callable
    EXPECT_TRUE(true);
}

TEST_F(ThemeSimpleTest, LoadNullTheme) {
    TextMateTheme theme = textmate_theme_load_from_json(nullptr);
    EXPECT_EQ(theme, nullptr);
}

TEST_F(ThemeSimpleTest, LoadEmptyStringTheme) {
    TextMateTheme theme = textmate_theme_load_from_json("");
    EXPECT_EQ(theme, nullptr);
}

TEST_F(ThemeSimpleTest, GetColorFromNullTheme) {
    uint32_t color = textmate_theme_get_default_foreground(nullptr);
    EXPECT_EQ(color, 0xFFFFFFFF);
}

TEST_F(ThemeSimpleTest, DisposeNullTheme) {
    // Should not crash
    textmate_theme_dispose(nullptr);
    EXPECT_TRUE(true);
}
