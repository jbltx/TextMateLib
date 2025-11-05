#ifndef TEXTMATELIB_ENCODED_TOKEN_ATTRIBUTES_H
#define TEXTMATELIB_ENCODED_TOKEN_ATTRIBUTES_H

#include "types.h"
#include <string>
#include <cstdint>

namespace tml {

// Encoded token data constants
namespace EncodedTokenDataConsts {
    const int32_t LANGUAGEID_MASK = 0b00000000000000000000000011111111;
    const int32_t TOKEN_TYPE_MASK = 0b00000000000000000000001100000000;
    const int32_t BALANCED_BRACKETS_MASK = 0b00000000000000000000010000000000;
    const int32_t FONT_STYLE_MASK = 0b00000000000000000111100000000000;
    const int32_t FOREGROUND_MASK = 0b00000000111111111000000000000000;
    const int32_t BACKGROUND_MASK = 0b11111111000000000000000000000000;

    const int LANGUAGEID_OFFSET = 0;
    const int TOKEN_TYPE_OFFSET = 8;
    const int BALANCED_BRACKETS_OFFSET = 10;
    const int FONT_STYLE_OFFSET = 11;
    const int FOREGROUND_OFFSET = 15;
    const int BACKGROUND_OFFSET = 24;
}

// Convert token type functions
inline OptionalStandardTokenType toOptionalTokenType(StandardTokenType standardType) {
    return static_cast<OptionalStandardTokenType>(static_cast<int>(standardType));
}

inline StandardTokenType fromOptionalTokenType(OptionalStandardTokenType standardType) {
    return static_cast<StandardTokenType>(static_cast<int>(standardType));
}

// EncodedTokenAttributes namespace (equivalent to TypeScript namespace)
namespace EncodedTokenAttributesHelper {
    std::string toBinaryStr(EncodedTokenAttributes encodedTokenAttributes);

    void print(EncodedTokenAttributes encodedTokenAttributes);

    int getLanguageId(EncodedTokenAttributes encodedTokenAttributes);

    StandardTokenType getTokenType(EncodedTokenAttributes encodedTokenAttributes);

    bool containsBalancedBrackets(EncodedTokenAttributes encodedTokenAttributes);

    int getFontStyle(EncodedTokenAttributes encodedTokenAttributes);

    int getForeground(EncodedTokenAttributes encodedTokenAttributes);

    int getBackground(EncodedTokenAttributes encodedTokenAttributes);

    EncodedTokenAttributes set(
        EncodedTokenAttributes encodedTokenAttributes,
        int languageId,
        OptionalStandardTokenType tokenType,
        bool* containsBalancedBrackets,
        int fontStyle,
        int foreground,
        int background
    );
}

} // namespace tml

#endif // TEXTMATELIB_ENCODED_TOKEN_ATTRIBUTES_H
