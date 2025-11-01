#include "matcher.h"
#include <regex>

namespace vscode_textmate {

bool isIdentifier(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    std::regex identifierPattern("[\\w\\.:]+");
    return std::regex_match(token, identifierPattern);
}

bool isIdentifier(const std::string* token) {
    if (!token || token->empty()) {
        return false;
    }
    return isIdentifier(*token);
}

SelectorTokenizer::SelectorTokenizer(const std::string& input)
    : _input(input), _position(0) {
}

std::string* SelectorTokenizer::next() {
    std::regex tokenPattern("([LR]:|[\\w\\.:][\\.:\\-\\w]*|[\\,\\|\\-\\(\\)])");
    std::smatch match;

    std::string remaining = _input.substr(_position);
    if (std::regex_search(remaining, match, tokenPattern)) {
        _position += match.position() + match.length();
        return new std::string(match.str());
    }

    return nullptr;
}

// Template instantiation would go in the cpp file
// But since this is a template, we need to keep implementation in header or explicitly instantiate

} // namespace vscode_textmate
