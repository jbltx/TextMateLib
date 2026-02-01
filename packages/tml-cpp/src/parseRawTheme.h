#ifndef TEXTMATELIB_PARSE_RAW_THEME_H
#define TEXTMATELIB_PARSE_RAW_THEME_H

#include "theme.h"
#include <string>

namespace tml {

// Parse raw theme from JSON content
// Returns IRawTheme that must be freed by caller
IRawTheme* parseRawTheme(const std::string& content);

// Parse JSON theme and return Theme object directly
// Returns Theme that must be freed by caller
Theme* parseJSONTheme(const std::string& content);

} // namespace tml

#endif // TEXTMATELIB_PARSE_RAW_THEME_H
