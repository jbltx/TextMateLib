#ifndef TEXTMATELIB_BASIC_SCOPES_ATTRIBUTE_PROVIDER_H
#define TEXTMATELIB_BASIC_SCOPES_ATTRIBUTE_PROVIDER_H

#include "types.h"
#include "utils.h"
#include <string>
#include <map>
#include <vector>
#include <regex>

namespace tml {

// BasicScopeAttributes class
class BasicScopeAttributes {
public:
    int languageId;
    OptionalStandardTokenType tokenType;

    BasicScopeAttributes() : languageId(0), tokenType() {}
    BasicScopeAttributes(int languageId_, OptionalStandardTokenType tokenType_);
};

// ScopeMatcher template class
template<typename TValue>
class ScopeMatcher {
private:
    std::map<std::string, TValue> values;
    std::regex* scopesRegExp;
    bool hasValues;

public:
    ScopeMatcher(const std::vector<std::pair<std::string, TValue>>& valuesList);
    ~ScopeMatcher();

    TValue* match(const ScopeName& scope);
};

// BasicScopeAttributesProvider class
class BasicScopeAttributesProvider {
private:
    BasicScopeAttributes _defaultAttributes;
    ScopeMatcher<int>* _embeddedLanguagesMatcher;
    CachedFn<ScopeName, BasicScopeAttributes>* _getBasicScopeAttributesCache;

    static std::regex STANDARD_TOKEN_TYPE_REGEXP;
    static BasicScopeAttributes NULL_SCOPE_METADATA;

public:
    BasicScopeAttributesProvider(int initialLanguageId, const EmbeddedLanguagesMap* embeddedLanguages);
    ~BasicScopeAttributesProvider();

    BasicScopeAttributes getDefaultAttributes() const;
    BasicScopeAttributes getBasicScopeAttributes(const ScopeName* scopeName);

private:
    int _scopeToLanguage(const ScopeName& scope);
    OptionalStandardTokenType _toStandardTokenType(const ScopeName& scopeName);
};

} // namespace tml

#endif // TEXTMATELIB_BASIC_SCOPES_ATTRIBUTE_PROVIDER_H
