#include <gtest/gtest.h>
#include <iostream>
#include <chrono>
#include "../src/c_api.h"

class ThemeDebugTest : public ::testing::Test {
};

// Simple JSON with minimal settings
TEST_F(ThemeDebugTest, LoadMinimalJSON) {
    std::cout << "\n=== Test: LoadMinimalJSON ===" << std::endl;

    const char* jsonTheme = R"({
        "name": "Minimal",
        "settings": []
    })";

    std::cout << "Parsing JSON..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    TextMateTheme theme = textmate_theme_load_from_json(jsonTheme);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "JSON parsing took: " << duration.count() << " ms" << std::endl;

    if (theme != nullptr) {
        std::cout << "✓ Theme loaded successfully" << std::endl;
        textmate_theme_dispose(theme);
    } else {
        std::cout << "✗ Failed to load theme" << std::endl;
    }

    ASSERT_NE(theme, nullptr);
}

// JSON with one simple setting
TEST_F(ThemeDebugTest, LoadOneSettingJSON) {
    std::cout << "\n=== Test: LoadOneSettingJSON ===" << std::endl;

    const char* jsonTheme = R"({
        "name": "One Setting",
        "settings": [
            {
                "settings": {
                    "foreground": "#FFFFFF"
                }
            }
        ]
    })";

    std::cout << "Parsing JSON with one setting..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    TextMateTheme theme = textmate_theme_load_from_json(jsonTheme);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Theme creation took: " << duration.count() << " ms" << std::endl;

    if (theme != nullptr) {
        std::cout << "✓ Theme loaded successfully" << std::endl;

        uint32_t fg = textmate_theme_get_default_foreground(theme);
        std::cout << "Default foreground: 0x" << std::hex << fg << std::dec << std::endl;

        textmate_theme_dispose(theme);
    } else {
        std::cout << "✗ Failed to load theme" << std::endl;
    }

    ASSERT_NE(theme, nullptr);
}

// JSON with scope setting
TEST_F(ThemeDebugTest, LoadScopeSettingJSON) {
    std::cout << "\n=== Test: LoadScopeSettingJSON ===" << std::endl;

    const char* jsonTheme = R"({
        "name": "With Scope",
        "settings": [
            {
                "scope": "comment",
                "settings": {
                    "foreground": "#888888"
                }
            }
        ]
    })";

    std::cout << "Parsing JSON with scope..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    TextMateTheme theme = textmate_theme_load_from_json(jsonTheme);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Theme creation took: " << duration.count() << " ms" << std::endl;

    if (theme != nullptr) {
        std::cout << "✓ Theme loaded successfully" << std::endl;
        textmate_theme_dispose(theme);
    } else {
        std::cout << "✗ Failed to load theme" << std::endl;
    }

    ASSERT_NE(theme, nullptr);
}

// JSON with multiple scopes
TEST_F(ThemeDebugTest, LoadMultipleScopesJSON) {
    std::cout << "\n=== Test: LoadMultipleScopesJSON ===" << std::endl;

    const char* jsonTheme = R"({
        "name": "Multiple Scopes",
        "settings": [
            {
                "scope": ["comment", "string"],
                "settings": {
                    "foreground": "#888888"
                }
            },
            {
                "scope": "keyword",
                "settings": {
                    "foreground": "#0066FF"
                }
            }
        ]
    })";

    std::cout << "Parsing JSON with multiple scopes..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    TextMateTheme theme = textmate_theme_load_from_json(jsonTheme);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Theme creation took: " << duration.count() << " ms" << std::endl;

    if (theme != nullptr) {
        std::cout << "✓ Theme loaded successfully" << std::endl;
        textmate_theme_dispose(theme);
    } else {
        std::cout << "✗ Failed to load theme" << std::endl;
    }

    ASSERT_NE(theme, nullptr);
}

// JSON with nested scopes
TEST_F(ThemeDebugTest, LoadNestedScopesJSON) {
    std::cout << "\n=== Test: LoadNestedScopesJSON ===" << std::endl;

    const char* jsonTheme = R"({
        "name": "Nested Scopes",
        "settings": [
            {
                "scope": "source.js string",
                "settings": {
                    "foreground": "#00FF00"
                }
            },
            {
                "scope": "source.js string.quoted.double",
                "settings": {
                    "foreground": "#00AA00"
                }
            }
        ]
    })";

    std::cout << "Parsing JSON with nested scopes..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    TextMateTheme theme = textmate_theme_load_from_json(jsonTheme);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Theme creation took: " << duration.count() << " ms" << std::endl;

    if (theme != nullptr) {
        std::cout << "✓ Theme loaded successfully" << std::endl;
        textmate_theme_dispose(theme);
    } else {
        std::cout << "✗ Failed to load theme" << std::endl;
    }

    ASSERT_NE(theme, nullptr);
}
