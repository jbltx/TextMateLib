#ifdef WASM_2023_FEATURES

#include "wasm/simd_bindings.h"
#include <algorithm>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <wasm_simd128.h>
#endif

// SIMD helper implementation
bool SIMDHelper::matchCharacterClassSIMD(const std::string& text,
                                         const std::string& pattern,
                                         size_t startPos) {
    if (startPos >= text.length()) {
        return false;
    }

    // For small patterns, scalar matching is faster
    if (pattern.length() <= 8) {
        for (size_t i = startPos; i < text.length(); ++i) {
            for (char p : pattern) {
                if (text[i] == p) return true;
            }
        }
        return false;
    }

#ifdef __WASM_SIMD128__
    // SIMD-optimized matching for larger patterns
    // Load pattern into 128-bit vector for comparison
    const char* patternData = pattern.data();
    const char* textData = text.data() + startPos;
    size_t textLen = text.length() - startPos;

    // Process in 16-byte chunks using SIMD
    size_t pos = 0;
    while (pos + 16 <= textLen) {
        // Load 16 bytes from text
        v128_t textVec = wasm_v128_load(textData + pos);

        // Check against each character in pattern
        for (char p : pattern) {
            // Create vector with repeated pattern character
            v128_t patternVec = wasm_i8x16_splat(p);

            // Compare all 16 bytes
            v128_t cmp = wasm_i8x16_eq(textVec, patternVec);

            // If any match, check with bitmask
            uint32_t mask = wasm_i8x16_bitmask(cmp);
            if (mask != 0) {
                return true;
            }
        }
        pos += 16;
    }

    // Handle remaining bytes
    for (size_t i = pos; i < textLen; ++i) {
        for (char p : pattern) {
            if (textData[i] == p) return true;
        }
    }
#else
    // Fallback for non-SIMD environments
    for (size_t i = startPos; i < text.length(); ++i) {
        for (char p : pattern) {
            if (text[i] == p) return true;
        }
    }
#endif

    return false;
}

std::vector<uint8_t> SIMDHelper::classifyCharactersSIMD(const std::string& text) {
    std::vector<uint8_t> classifications(text.length());

#ifdef __WASM_SIMD128__
    // Character type flags
    const uint8_t CHAR_WHITESPACE = 1 << 0;
    const uint8_t CHAR_DIGIT = 1 << 1;
    const uint8_t CHAR_LETTER = 1 << 2;
    const uint8_t CHAR_SYMBOL = 1 << 3;

    const char* data = text.data();
    size_t len = text.length();
    size_t i = 0;

    // Process 16 characters at a time
    while (i + 16 <= len) {
        v128_t chars = wasm_v128_load(data + i);

        // Check for whitespace (space, tab, newline, carriage return)
        v128_t ws_space = wasm_i8x16_eq(chars, wasm_i8x16_splat(' '));
        v128_t ws_tab = wasm_i8x16_eq(chars, wasm_i8x16_splat('\t'));
        v128_t ws_newline = wasm_i8x16_eq(chars, wasm_i8x16_splat('\n'));
        v128_t ws_cr = wasm_i8x16_eq(chars, wasm_i8x16_splat('\r'));

        v128_t is_ws = wasm_v128_or(wasm_v128_or(ws_space, ws_tab),
                                     wasm_v128_or(ws_newline, ws_cr));

        // Check for digits (0-9)
        v128_t ge_zero = wasm_i8x16_ge_u(chars, wasm_i8x16_splat('0'));
        v128_t le_nine = wasm_i8x16_le_u(chars, wasm_i8x16_splat('9'));
        v128_t is_digit = wasm_v128_and(ge_zero, le_nine);

        // Check for lowercase letters (a-z)
        v128_t ge_a = wasm_i8x16_ge_u(chars, wasm_i8x16_splat('a'));
        v128_t le_z = wasm_i8x16_le_u(chars, wasm_i8x16_splat('z'));
        v128_t is_lower = wasm_v128_and(ge_a, le_z);

        // Check for uppercase letters (A-Z)
        v128_t ge_A = wasm_i8x16_ge_u(chars, wasm_i8x16_splat('A'));
        v128_t le_Z = wasm_i8x16_le_u(chars, wasm_i8x16_splat('Z'));
        v128_t is_upper = wasm_v128_and(ge_A, le_Z);

        v128_t is_letter = wasm_v128_or(is_lower, is_upper);

        // Store results
        for (int j = 0; j < 16; ++j) {
            uint8_t classification = 0;
            if (wasm_i8x16_extract_lane(is_ws, j)) classification |= CHAR_WHITESPACE;
            if (wasm_i8x16_extract_lane(is_digit, j)) classification |= CHAR_DIGIT;
            if (wasm_i8x16_extract_lane(is_letter, j)) classification |= CHAR_LETTER;
            if (!classification) classification = CHAR_SYMBOL;

            classifications[i + j] = classification;
        }

        i += 16;
    }

    // Handle remaining characters
    for (; i < len; ++i) {
        uint8_t classification = 0;
        char c = data[i];

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            classification |= CHAR_WHITESPACE;
        } else if (c >= '0' && c <= '9') {
            classification |= CHAR_DIGIT;
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            classification |= CHAR_LETTER;
        } else {
            classification = CHAR_SYMBOL;
        }

        classifications[i] = classification;
    }
#else
    // Scalar fallback
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        uint8_t classification = 0;

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            classification |= 1; // whitespace
        } else if (c >= '0' && c <= '9') {
            classification |= 2; // digit
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            classification |= 4; // letter
        } else {
            classification = 8; // symbol
        }

        classifications[i] = classification;
    }
#endif

    return classifications;
}

size_t SIMDHelper::findNextBoundarySIMD(const std::string& text, size_t startPos) {
    if (startPos >= text.length()) {
        return std::string::npos;
    }

    const char* data = text.data() + startPos;
    size_t len = text.length() - startPos;

#ifdef __WASM_SIMD128__
    // Find next non-whitespace boundary using SIMD
    size_t i = 0;

    while (i + 16 <= len) {
        v128_t chars = wasm_v128_load(data + i);

        // Check for non-whitespace
        v128_t ws_space = wasm_i8x16_eq(chars, wasm_i8x16_splat(' '));
        v128_t ws_tab = wasm_i8x16_eq(chars, wasm_i8x16_splat('\t'));
        v128_t is_ws = wasm_v128_or(ws_space, ws_tab);

        // Get bitmask of non-whitespace positions
        uint32_t mask = wasm_i8x16_bitmask(wasm_v128_not(is_ws));

        if (mask != 0) {
            // Find first set bit
            return startPos + i + __builtin_ctz(mask);
        }

        i += 16;
    }
#else
    size_t i = 0;
#endif

    // Scalar fallback for remaining bytes
    for (; i < len; ++i) {
        if (data[i] != ' ' && data[i] != '\t') {
            return startPos + i;
        }
    }

    return std::string::npos;
}

std::vector<size_t> SIMDHelper::findPatternMatchesSIMD(const std::string& text,
                                                      const std::string& pattern) {
    std::vector<size_t> matches;

    if (pattern.empty() || text.empty()) {
        return matches;
    }

    // For small patterns, use scalar matching
    if (pattern.length() == 1) {
        char p = pattern[0];
        for (size_t i = 0; i < text.length(); ++i) {
            if (text[i] == p) {
                matches.push_back(i);
            }
        }
        return matches;
    }

#ifdef __WASM_SIMD128__
    const char* textData = text.data();
    const char* patternData = pattern.data();
    size_t textLen = text.length();
    size_t patternLen = pattern.length();

    // SIMD matching for patterns up to 16 bytes
    if (patternLen <= 16 && textLen >= patternLen) {
        size_t i = 0;

        while (i + patternLen <= textLen) {
            // Load pattern for comparison
            v128_t patternVec = wasm_v128_load(patternData);

            // Load text window
            v128_t textVec = wasm_v128_load(textData + i);

            // Compare
            v128_t cmp = wasm_i8x16_eq(textVec, patternVec);
            uint32_t mask = wasm_i8x16_bitmask(cmp);

            // Check if all pattern bytes matched
            uint32_t fullMask = (1 << patternLen) - 1;
            if ((mask & fullMask) == fullMask) {
                matches.push_back(i);
            }

            i++;
        }
        return matches;
    }
#endif

    // Scalar fallback for larger patterns
    for (size_t i = 0; i + patternLen <= text.length(); ++i) {
        if (text.substr(i, patternLen) == pattern) {
            matches.push_back(i);
        }
    }

    return matches;
}

#ifdef __EMSCRIPTEN__
// Emscripten bindings for SIMD helpers
EMSCRIPTEN_BINDINGS(simd_helpers) {
    emscripten::class_<SIMDHelper>("SIMDHelper")
        .class_function("matchCharacterClass",
                       &SIMDHelper::matchCharacterClassSIMD)
        .class_function("classifyCharacters",
                       &SIMDHelper::classifyCharactersSIMD)
        .class_function("findNextBoundary",
                       &SIMDHelper::findNextBoundarySIMD)
        .class_function("findPatternMatches",
                       &SIMDHelper::findPatternMatchesSIMD);
};
#endif

#endif // WASM_2023_FEATURES
