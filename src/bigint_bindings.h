#ifndef TEXTMATELIB_BIGINT_BINDINGS_H
#define TEXTMATELIB_BIGINT_BINDINGS_H

#ifdef WASM_2023_FEATURES

#include <cstdint>
#include <cstddef>

/**
 * BigInt support for WASM 2023
 *
 * Provides 64-bit token metadata without precision loss using
 * JavaScript BigInt64Array and BigUint64Array for direct access.
 */

/**
 * 64-bit token metadata structure
 * Maps directly to BigInt64Array for efficient JavaScript access
 */
struct TokenMetadata64 {
    int64_t tokenId;        // Unique token identifier
    int64_t timestamp;      // Last modified timestamp (milliseconds)
    uint64_t styleHash;     // Cached style information hash
    uint32_t scopeDepth;    // Nesting depth for scope tracking
    uint32_t flags;         // Additional flags for token properties
};

/**
 * Helper class for BigInt operations
 */
class BigIntHelper {
public:
    /**
     * Create metadata for a token with full precision
     */
    static TokenMetadata64 createTokenMetadata(
        int64_t tokenId,
        int64_t timestamp,
        uint64_t styleHash,
        uint32_t scopeDepth,
        uint32_t flags);

    /**
     * Extract timestamp with millisecond precision
     */
    static int64_t extractTimestamp(const TokenMetadata64& metadata);

    /**
     * Extract style hash for efficient theme switching
     */
    static uint64_t extractStyleHash(const TokenMetadata64& metadata);

    /**
     * Pack two 32-bit values into a 64-bit BigInt
     */
    static int64_t packInt32Pair(int32_t high, int32_t low);

    /**
     * Unpack a 64-bit BigInt into two 32-bit values
     */
    static void unpackInt32Pair(int64_t value, int32_t& high, int32_t& low);

    /**
     * Encode RGB color as 64-bit value (with alpha and precision info)
     */
    static uint64_t encodeColorBigInt(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    /**
     * Decode RGB color from 64-bit value
     */
    static void decodeColorBigInt(uint64_t value, uint8_t& r, uint8_t& g,
                                 uint8_t& b, uint8_t& a);
};

#endif // WASM_2023_FEATURES

#endif // TEXTMATELIB_BIGINT_BINDINGS_H
