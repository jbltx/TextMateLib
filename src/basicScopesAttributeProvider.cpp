#include "basicScopesAttributeProvider.h"
#include <algorithm>

namespace vscode_textmate {

// Static members initialization
std::regex BasicScopeAttributesProvider::STANDARD_TOKEN_TYPE_REGEXP("\\b(comment|string|regex|meta\\.embedded)\\b");
BasicScopeAttributes BasicScopeAttributesProvider::NULL_SCOPE_METADATA(0, OptionalStandardTokenType::Other);

// BasicScopeAttributes implementation

BasicScopeAttributes::BasicScopeAttributes(int languageId_, OptionalStandardTokenType tokenType_)
    : languageId(languageId_), tokenType(tokenType_) {
}

// ScopeMatcher implementation

template<typename TValue>
ScopeMatcher<TValue>::ScopeMatcher(const std::vector<std::pair<std::string, TValue>>& valuesList)
    : scopesRegExp(nullptr), hasValues(!valuesList.empty()) {

    if (valuesList.empty()) {
        return;
    }

    for (const auto& pair : valuesList) {
        values[pair.first] = pair.second;
    }

    // Create the regex
    std::vector<std::string> escapedScopes;
    for (const auto& pair : valuesList) {
        escapedScopes.push_back(escapeRegExpCharacters(pair.first));
    }

    std::sort(escapedScopes.begin(), escapedScopes.end());
    std::reverse(escapedScopes.begin(), escapedScopes.end()); // Longest scope first

    // Build regex pattern
    std::string pattern = "^((";
    for (size_t i = 0; i < escapedScopes.size(); i++) {
        if (i > 0) pattern += ")|(";
        pattern += escapedScopes[i];
    }
    pattern += "))($|\\.)";

    scopesRegExp = new std::regex(pattern);
}

template<typename TValue>
ScopeMatcher<TValue>::~ScopeMatcher() {
    delete scopesRegExp;
}

template<typename TValue>
TValue* ScopeMatcher<TValue>::match(const ScopeName& scope) {
    if (!scopesRegExp) {
        return nullptr;
    }

    std::smatch m;
    if (!std::regex_search(scope, m, *scopesRegExp)) {
        return nullptr;
    }

    std::string matchedScope = m[1].str();
    auto it = values.find(matchedScope);
    if (it != values.end()) {
        return &it->second;
    }

    return nullptr;
}

// Explicit template instantiation
template class ScopeMatcher<int>;

// BasicScopeAttributesProvider implementation

BasicScopeAttributesProvider::BasicScopeAttributesProvider(
    int initialLanguageId,
    const EmbeddedLanguagesMap* embeddedLanguages)
    : _defaultAttributes(initialLanguageId, OptionalStandardTokenType::NotSet) {

    std::vector<std::pair<std::string, int>> embeddedLanguagesList;
    if (embeddedLanguages) {
        for (const auto& pair : *embeddedLanguages) {
            embeddedLanguagesList.push_back(pair);
        }
    }

    _embeddedLanguagesMatcher = new ScopeMatcher<int>(embeddedLanguagesList);

    _getBasicScopeAttributesCache = new CachedFn<ScopeName, BasicScopeAttributes>(
        [this](const ScopeName& scopeName) {
            int languageId = this->_scopeToLanguage(scopeName);
            OptionalStandardTokenType standardTokenType = this->_toStandardTokenType(scopeName);
            return BasicScopeAttributes(languageId, standardTokenType);
        }
    );
}

BasicScopeAttributesProvider::~BasicScopeAttributesProvider() {
    delete _embeddedLanguagesMatcher;
    delete _getBasicScopeAttributesCache;
}

BasicScopeAttributes BasicScopeAttributesProvider::getDefaultAttributes() const {
    return _defaultAttributes;
}

BasicScopeAttributes BasicScopeAttributesProvider::getBasicScopeAttributes(const ScopeName* scopeName) {
    if (scopeName == nullptr) {
        return NULL_SCOPE_METADATA;
    }
    return _getBasicScopeAttributesCache->get(*scopeName);
}

int BasicScopeAttributesProvider::_scopeToLanguage(const ScopeName& scope) {
    int* languageId = _embeddedLanguagesMatcher->match(scope);
    return languageId ? *languageId : 0;
}

OptionalStandardTokenType BasicScopeAttributesProvider::_toStandardTokenType(const ScopeName& scopeName) {
    std::smatch m;
    if (!std::regex_search(scopeName, m, STANDARD_TOKEN_TYPE_REGEXP)) {
        return OptionalStandardTokenType::NotSet;
    }

    std::string matchedType = m[1].str();
    if (matchedType == "comment") {
        return OptionalStandardTokenType::Comment;
    } else if (matchedType == "string") {
        return OptionalStandardTokenType::String;
    } else if (matchedType == "regex") {
        return OptionalStandardTokenType::RegEx;
    } else if (matchedType == "meta.embedded") {
        return OptionalStandardTokenType::Other;
    }

    return OptionalStandardTokenType::NotSet;
}

} // namespace vscode_textmate
