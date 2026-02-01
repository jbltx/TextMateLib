#include "encodedTokenAttributes.h"
#include <iostream>
#include <bitset>
#include <sstream>

namespace tml {

namespace EncodedTokenAttributesHelper {

std::string toBinaryStr(EncodedTokenAttributes encodedTokenAttributes) {
    std::bitset<32> bits(encodedTokenAttributes);
    return bits.to_string();
}

void print(EncodedTokenAttributes encodedTokenAttributes) {
    int languageId = getLanguageId(encodedTokenAttributes);
    StandardTokenType tokenType = getTokenType(encodedTokenAttributes);
    int fontStyle = getFontStyle(encodedTokenAttributes);
    int foreground = getForeground(encodedTokenAttributes);
    int background = getBackground(encodedTokenAttributes);
}

int getLanguageId(EncodedTokenAttributes encodedTokenAttributes) {
    return (encodedTokenAttributes & EncodedTokenDataConsts::LANGUAGEID_MASK) >>
           EncodedTokenDataConsts::LANGUAGEID_OFFSET;
}

StandardTokenType getTokenType(EncodedTokenAttributes encodedTokenAttributes) {
    return static_cast<StandardTokenType>(
        (encodedTokenAttributes & EncodedTokenDataConsts::TOKEN_TYPE_MASK) >>
        EncodedTokenDataConsts::TOKEN_TYPE_OFFSET
    );
}

bool containsBalancedBrackets(EncodedTokenAttributes encodedTokenAttributes) {
    return (encodedTokenAttributes & EncodedTokenDataConsts::BALANCED_BRACKETS_MASK) != 0;
}

int getFontStyle(EncodedTokenAttributes encodedTokenAttributes) {
    return (encodedTokenAttributes & EncodedTokenDataConsts::FONT_STYLE_MASK) >>
           EncodedTokenDataConsts::FONT_STYLE_OFFSET;
}

int getForeground(EncodedTokenAttributes encodedTokenAttributes) {
    return (encodedTokenAttributes & EncodedTokenDataConsts::FOREGROUND_MASK) >>
           EncodedTokenDataConsts::FOREGROUND_OFFSET;
}

int getBackground(EncodedTokenAttributes encodedTokenAttributes) {
    return (static_cast<uint32_t>(encodedTokenAttributes) &
            static_cast<uint32_t>(EncodedTokenDataConsts::BACKGROUND_MASK)) >>
           EncodedTokenDataConsts::BACKGROUND_OFFSET;
}

EncodedTokenAttributes set(
    EncodedTokenAttributes encodedTokenAttributes,
    int languageId,
    OptionalStandardTokenType tokenType,
    bool* containsBalancedBracketsParam,
    int fontStyle,
    int foreground,
    int background
) {
    int _languageId = getLanguageId(encodedTokenAttributes);
    int _tokenType = static_cast<int>(getTokenType(encodedTokenAttributes));
    int _containsBalancedBracketsBit = containsBalancedBrackets(encodedTokenAttributes) ? 1 : 0;
    int _fontStyle = getFontStyle(encodedTokenAttributes);
    int _foreground = getForeground(encodedTokenAttributes);
    int _background = getBackground(encodedTokenAttributes);

    if (languageId != 0) {
        _languageId = languageId;
    }
    if (tokenType != OptionalStandardTokenType::NotSet) {
        _tokenType = static_cast<int>(fromOptionalTokenType(tokenType));
    }
    if (containsBalancedBracketsParam != nullptr) {
        _containsBalancedBracketsBit = *containsBalancedBracketsParam ? 1 : 0;
    }
    if (fontStyle != static_cast<int>(FontStyle::NotSet)) {
        _fontStyle = fontStyle;
    }
    if (foreground != 0) {
        _foreground = foreground;
    }
    if (background != 0) {
        _background = background;
    }

    return static_cast<EncodedTokenAttributes>(
        ((_languageId << EncodedTokenDataConsts::LANGUAGEID_OFFSET) |
         (_tokenType << EncodedTokenDataConsts::TOKEN_TYPE_OFFSET) |
         (_containsBalancedBracketsBit << EncodedTokenDataConsts::BALANCED_BRACKETS_OFFSET) |
         (_fontStyle << EncodedTokenDataConsts::FONT_STYLE_OFFSET) |
         (_foreground << EncodedTokenDataConsts::FOREGROUND_OFFSET) |
         (_background << EncodedTokenDataConsts::BACKGROUND_OFFSET))
    );
}

} // namespace EncodedTokenAttributesHelper

} // namespace tml
