#ifndef VSCODE_TEXTMATE_PARSE_RAW_GRAMMAR_H
#define VSCODE_TEXTMATE_PARSE_RAW_GRAMMAR_H

#include "rawGrammar.h"
#include <string>

namespace vscode_textmate {

// Parse raw grammar from JSON content
IRawGrammar* parseRawGrammar(const std::string& content, const std::string* filePath = nullptr);

// Parse JSON grammar specifically
IRawGrammar* parseJSONGrammar(const std::string& content, const std::string* filename = nullptr);

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_PARSE_RAW_GRAMMAR_H
