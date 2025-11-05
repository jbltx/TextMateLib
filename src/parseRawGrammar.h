#ifndef TEXTMATELIB_PARSE_RAW_GRAMMAR_H
#define TEXTMATELIB_PARSE_RAW_GRAMMAR_H

#include "rawGrammar.h"
#include <string>

namespace tml {

// Parse raw grammar from JSON content
IRawGrammar* parseRawGrammar(const std::string& content, const std::string* filePath = nullptr);

// Parse JSON grammar specifically
IRawGrammar* parseJSONGrammar(const std::string& content, const std::string* filename = nullptr);

} // namespace tml

#endif // TEXTMATELIB_PARSE_RAW_GRAMMAR_H
