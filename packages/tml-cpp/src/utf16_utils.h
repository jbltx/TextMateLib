#ifndef TEXTMATELIB_UTF16_UTILS_H
#define TEXTMATELIB_UTF16_UTILS_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tml {

/// Builds a lookup table mapping UTF-8 byte offsets to UTF-16 code unit indices.
///
/// For ASCII text, map[i] == i (identity).
/// For multi-byte UTF-8 sequences:
///   - 2-byte (U+0080..U+07FF)  -> 1 UTF-16 code unit
///   - 3-byte (U+0800..U+FFFF)  -> 1 UTF-16 code unit
///   - 4-byte (U+10000..U+10FFFF) -> 2 UTF-16 code units (surrogate pair)
///
/// The returned vector has byteLen+1 entries so that map[byteLen] gives the
/// total UTF-16 length (useful for end-of-string indices).
inline std::vector<int32_t> buildByteToUtf16Map(const char* utf8, size_t byteLen) {
    std::vector<int32_t> map(byteLen + 1);
    int32_t utf16Index = 0;
    size_t i = 0;

    while (i < byteLen) {
        map[i] = utf16Index;

        unsigned char ch = static_cast<unsigned char>(utf8[i]);
        size_t seqLen;
        int32_t utf16Units;

        if (ch < 0x80) {
            seqLen = 1;
            utf16Units = 1;
        } else if ((ch & 0xE0) == 0xC0) {
            seqLen = 2;
            utf16Units = 1;
        } else if ((ch & 0xF0) == 0xE0) {
            seqLen = 3;
            utf16Units = 1;
        } else if ((ch & 0xF8) == 0xF0) {
            seqLen = 4;
            utf16Units = 2; // surrogate pair in UTF-16
        } else {
            // Invalid leading byte — treat as 1 byte, 1 unit
            seqLen = 1;
            utf16Units = 1;
        }

        // Fill continuation bytes in the map with the same utf16Index
        for (size_t j = 1; j < seqLen && (i + j) < byteLen; j++) {
            map[i + j] = utf16Index;
        }

        i += seqLen;
        utf16Index += utf16Units;
    }

    // Sentinel: map[byteLen] = total UTF-16 length
    map[byteLen] = utf16Index;

    return map;
}

} // namespace tml

#endif // TEXTMATELIB_UTF16_UTILS_H
