#ifndef TEXTMATELIB_SIMD_BINDINGS_H
#define TEXTMATELIB_SIMD_BINDINGS_H

#ifdef WASM_2023_FEATURES

#include <cstdint>
#include <string>
#include <vector>

/**
 * SIMD-accelerated helper functions for WASM 2023
 *
 * This module provides SIMD128-optimized operations for:
 * - Character classification and pattern matching
 * - Bulk character property lookups
 * - Fast string scanning for tokenization boundaries
 */

class SIMDHelper {
public:
    /**
     * Vectorized character matching using WASM SIMD 128-bit vectors
     * ~4x faster than scalar character-by-character comparison
     */
    static bool matchCharacterClassSIMD(const std::string& text,
                                       const std::string& pattern,
                                       size_t startPos = 0);

    /**
     * Classify characters in bulk using SIMD operations
     * Returns character type flags (whitespace, digit, letter, etc.)
     */
    static std::vector<uint8_t> classifyCharactersSIMD(const std::string& text);

    /**
     * Find the next token boundary using SIMD vectorized scanning
     * Returns position of next significant character or std::string::npos
     */
    static size_t findNextBoundarySIMD(const std::string& text, size_t startPos);

    /**
     * Scan for pattern match using SIMD prefix scanning
     * More efficient than standard regex for simple patterns
     */
    static std::vector<size_t> findPatternMatchesSIMD(const std::string& text,
                                                     const std::string& pattern);
};

#endif // WASM_2023_FEATURES

#endif // TEXTMATELIB_SIMD_BINDINGS_H
