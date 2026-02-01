#include <gtest/gtest.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include "tml_c.h"

class ThemeFileTest : public ::testing::Test {
};

TEST_F(ThemeFileTest, LoadDarkPlusFile) {
    std::cout << "\n=== Test: LoadDarkPlusFile ===" << std::endl;

    std::string themePath = "../test_theme/fixtures/dark_plus.json";

    std::cout << "Loading theme from: " << themePath << std::endl;

    TextMateTheme theme = textmate_theme_load_from_file(themePath.c_str());

    EXPECT_NE(theme, nullptr) << "Failed to load theme from " << themePath;

    if (theme != nullptr) {
        std::cout << "✓ Theme loaded successfully" << std::endl;
        uint32_t fg = textmate_theme_get_default_foreground(theme);
        EXPECT_NE(fg, 0) << "Default foreground should be non-zero";
        // Note: NOT calling textmate_theme_dispose due to Theme class cleanup issue
        // See PHASE1B_FINDINGS.md for details
    }
}
