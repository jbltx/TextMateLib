#include "theme.h"
#include <algorithm>
#include <sstream>

namespace vscode_textmate {

// ScopeStack implementation

ScopeStack::ScopeStack(ScopeStack* parent_, const ScopeName& scopeName_)
    : parent(parent_), scopeName(scopeName_) {
}

ScopeStack::~ScopeStack() {
    // Note: parent is not owned, don't delete
}

ScopeStack* ScopeStack::push(ScopeStack* path, const std::vector<ScopeName>& scopeNames) {
    for (const auto& name : scopeNames) {
        path = new ScopeStack(path, name);
    }
    return path;
}

ScopeStack* ScopeStack::from(const std::vector<ScopeName>& segments) {
    ScopeStack* result = nullptr;
    for (const auto& segment : segments) {
        result = new ScopeStack(result, segment);
    }
    return result;
}

ScopeStack* ScopeStack::push(const ScopeName& scopeName_) {
    return new ScopeStack(this, scopeName_);
}

std::vector<ScopeName> ScopeStack::getSegments() const {
    std::vector<ScopeName> result;
    const ScopeStack* item = this;
    while (item) {
        result.push_back(item->scopeName);
        item = item->parent;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::string ScopeStack::toString() const {
    std::vector<ScopeName> segments = getSegments();
    std::string result;
    for (size_t i = 0; i < segments.size(); i++) {
        if (i > 0) result += " ";
        result += segments[i];
    }
    return result;
}

bool ScopeStack::extends(const ScopeStack* other) const {
    if (this == other) {
        return true;
    }
    if (parent == nullptr) {
        return false;
    }
    return parent->extends(other);
}

std::vector<ScopeName>* ScopeStack::getExtensionIfDefined(const ScopeStack* base) const {
    std::vector<ScopeName>* result = new std::vector<ScopeName>();
    const ScopeStack* item = this;
    while (item && item != base) {
        result->push_back(item->scopeName);
        item = item->parent;
    }
    if (item == base) {
        std::reverse(result->begin(), result->end());
        return result;
    }
    delete result;
    return nullptr;
}

// StyleAttributes implementation

StyleAttributes::StyleAttributes(int fontStyle_, int foregroundId_, int backgroundId_)
    : fontStyle(fontStyle_), foregroundId(foregroundId_), backgroundId(backgroundId_) {
}

// ParsedThemeRule implementation

ParsedThemeRule::ParsedThemeRule(const ScopeName& scope_,
                                std::vector<ScopeName>* parentScopes_,
                                int index_,
                                int fontStyle_,
                                const std::string* foreground_,
                                const std::string* background_)
    : scope(scope_), parentScopes(parentScopes_), index(index_), fontStyle(fontStyle_),
      foreground(foreground_ ? new std::string(*foreground_) : nullptr),
      background(background_ ? new std::string(*background_) : nullptr) {
}

ParsedThemeRule::~ParsedThemeRule() {
    delete parentScopes;
    delete foreground;
    delete background;
}

// Parse theme

std::vector<ParsedThemeRule*> parseTheme(const IRawTheme* source) {
    std::vector<ParsedThemeRule*> result;

    if (!source || source->settings.empty()) {
        return result;
    }

    for (size_t i = 0; i < source->settings.size(); i++) {
        const IRawThemeSetting* entry = source->settings[i];

        std::vector<std::string> scopes;
        if (entry->scope != nullptr && !entry->scope->empty()) {
            scopes = *entry->scope;
        } else if (!entry->scopeString.empty()) {
            std::string scopeStr = entry->scopeString;

            // Remove leading and trailing commas
            size_t start = scopeStr.find_first_not_of(',');
            size_t end = scopeStr.find_last_not_of(',');
            if (start != std::string::npos && end != std::string::npos) {
                scopeStr = scopeStr.substr(start, end - start + 1);
            }

            // Split by comma
            size_t pos = 0;
            while ((pos = scopeStr.find(',')) != std::string::npos) {
                scopes.push_back(scopeStr.substr(0, pos));
                scopeStr.erase(0, pos + 1);
            }
            if (!scopeStr.empty()) {
                scopes.push_back(scopeStr);
            }
        } else {
            scopes.push_back("");
        }

        int fontStyle = static_cast<int>(FontStyle::NotSet);
        if (entry->settings.fontStyle != nullptr) {
            fontStyle = static_cast<int>(FontStyle::None);

            std::string fontStyleStr = *entry->settings.fontStyle;
            std::istringstream iss(fontStyleStr);
            std::string segment;
            while (iss >> segment) {
                if (segment == "italic") {
                    fontStyle |= static_cast<int>(FontStyle::Italic);
                } else if (segment == "bold") {
                    fontStyle |= static_cast<int>(FontStyle::Bold);
                } else if (segment == "underline") {
                    fontStyle |= static_cast<int>(FontStyle::Underline);
                } else if (segment == "strikethrough") {
                    fontStyle |= static_cast<int>(FontStyle::Strikethrough);
                }
            }
        }

        std::string* foreground = nullptr;
        if (entry->settings.foreground != nullptr && isValidHexColor(*entry->settings.foreground)) {
            foreground = new std::string(*entry->settings.foreground);
        }

        std::string* background = nullptr;
        if (entry->settings.background != nullptr && isValidHexColor(*entry->settings.background)) {
            background = new std::string(*entry->settings.background);
        }

        for (const auto& scopeStr : scopes) {
            std::string trimmedScope = scopeStr;
            // Trim whitespace
            trimmedScope.erase(0, trimmedScope.find_first_not_of(" \t\n\r"));
            trimmedScope.erase(trimmedScope.find_last_not_of(" \t\n\r") + 1);

            // Split by spaces
            std::vector<std::string> segments;
            std::istringstream iss(trimmedScope);
            std::string segment;
            while (iss >> segment) {
                segments.push_back(segment);
            }

            std::string scope = segments.empty() ? "" : segments.back();
            std::vector<ScopeName>* parentScopes = nullptr;
            if (segments.size() > 1) {
                parentScopes = new std::vector<ScopeName>(segments.begin(), segments.end() - 1);
                std::reverse(parentScopes->begin(), parentScopes->end());
            }

            result.push_back(new ParsedThemeRule(
                scope,
                parentScopes,
                i,
                fontStyle,
                foreground,
                background
            ));
        }

        delete foreground;
        delete background;
    }

    return result;
}

// Font style to string

std::string fontStyleToString(int fontStyle) {
    if (fontStyle == static_cast<int>(FontStyle::NotSet)) {
        return "not set";
    }

    std::string style;
    if (fontStyle & static_cast<int>(FontStyle::Italic)) {
        style += "italic ";
    }
    if (fontStyle & static_cast<int>(FontStyle::Bold)) {
        style += "bold ";
    }
    if (fontStyle & static_cast<int>(FontStyle::Underline)) {
        style += "underline ";
    }
    if (fontStyle & static_cast<int>(FontStyle::Strikethrough)) {
        style += "strikethrough ";
    }
    if (style.empty()) {
        style = "none";
    } else {
        // Remove trailing space
        style = style.substr(0, style.length() - 1);
    }
    return style;
}

// ColorMap implementation

ColorMap::ColorMap(const std::vector<std::string>* colorMap)
    : _lastColorId(0) {

    if (colorMap != nullptr && !colorMap->empty()) {
        _isFrozen = true;
        for (size_t i = 0; i < colorMap->size(); i++) {
            std::string upperColor = (*colorMap)[i];
            std::transform(upperColor.begin(), upperColor.end(), upperColor.begin(), ::toupper);
            _color2id[upperColor] = i;
            _id2color.push_back((*colorMap)[i]);
        }
        _lastColorId = colorMap->size() - 1;
    } else {
        _isFrozen = false;
    }
}

int ColorMap::getId(const std::string* color) {
    if (color == nullptr) {
        return 0;
    }

    std::string upperColor = *color;
    std::transform(upperColor.begin(), upperColor.end(), upperColor.begin(), ::toupper);

    auto it = _color2id.find(upperColor);
    if (it != _color2id.end()) {
        return it->second;
    }

    if (_isFrozen) {
        // In frozen mode, we should throw an error
        // For now, return 0
        return 0;
    }

    int value = ++_lastColorId;
    _color2id[upperColor] = value;
    if (_id2color.size() <= static_cast<size_t>(value)) {
        _id2color.resize(value + 1);
    }
    _id2color[value] = *color;
    return value;
}

std::vector<std::string> ColorMap::getColorMap() const {
    return _id2color;
}

// ThemeTrieElementRule implementation

ThemeTrieElementRule::ThemeTrieElementRule(int scopeDepth_,
                                          const std::vector<ScopeName>* parentScopes_,
                                          int fontStyle_,
                                          int foreground_,
                                          int background_)
    : scopeDepth(scopeDepth_),
      parentScopes(parentScopes_ ? *parentScopes_ : std::vector<ScopeName>()),
      fontStyle(fontStyle_),
      foreground(foreground_),
      background(background_) {
}

ThemeTrieElementRule* ThemeTrieElementRule::clone() const {
    return new ThemeTrieElementRule(scopeDepth, &parentScopes, fontStyle, foreground, background);
}

std::vector<ThemeTrieElementRule*> ThemeTrieElementRule::cloneArr(const std::vector<ThemeTrieElementRule*>& arr) {
    std::vector<ThemeTrieElementRule*> result;
    for (auto* rule : arr) {
        result.push_back(rule->clone());
    }
    return result;
}

void ThemeTrieElementRule::acceptOverwrite(int scopeDepth_, int fontStyle_, int foreground_, int background_) {
    if (scopeDepth_ > this->scopeDepth) {
        this->scopeDepth = scopeDepth_;
    }

    if (fontStyle_ != static_cast<int>(FontStyle::NotSet)) {
        this->fontStyle = fontStyle_;
    }
    if (foreground_ != 0) {
        this->foreground = foreground_;
    }
    if (background_ != 0) {
        this->background = background_;
    }
}

// ThemeTrieElement implementation

ThemeTrieElement::ThemeTrieElement(ThemeTrieElementRule* mainRule,
                                  const std::vector<ThemeTrieElementRule*>& rulesWithParentScopes)
    : _mainRule(mainRule), _rulesWithParentScopes(rulesWithParentScopes) {
}

ThemeTrieElement::~ThemeTrieElement() {
    delete _mainRule;
    for (auto* rule : _rulesWithParentScopes) {
        delete rule;
    }
    for (auto& pair : _children) {
        delete pair.second;
    }
}

std::vector<ThemeTrieElementRule*> ThemeTrieElement::match(const ScopeName& scope) {
    // Collect all matching rules
    std::vector<ThemeTrieElementRule*> result;

    // Check main rule
    if (_mainRule) {
        result.push_back(_mainRule);
    }

    // Add rules with parent scopes
    result.insert(result.end(), _rulesWithParentScopes.begin(), _rulesWithParentScopes.end());

    // Check children
    std::string dotScope = "." + scope;
    for (auto& pair : _children) {
        if (_matchesScope(scope, pair.first)) {
            auto childMatches = pair.second->match(scope);
            result.insert(result.end(), childMatches.begin(), childMatches.end());
        }
    }

    return result;
}

void ThemeTrieElement::insert(int scopeDepth,
                              const std::string& scope,
                              const std::vector<ScopeName>* parentScopes,
                              int fontStyle,
                              int foreground,
                              int background) {
    if (scope.empty()) {
        // Reached the end of the scope path
        if (parentScopes == nullptr || parentScopes->empty()) {
            _mainRule->acceptOverwrite(scopeDepth, fontStyle, foreground, background);
        } else {
            // Has parent scopes
            bool found = false;
            for (auto* rule : _rulesWithParentScopes) {
                if (rule->parentScopes == *parentScopes) {
                    rule->acceptOverwrite(scopeDepth, fontStyle, foreground, background);
                    found = true;
                    break;
                }
            }
            if (!found) {
                _rulesWithParentScopes.push_back(
                    new ThemeTrieElementRule(scopeDepth, parentScopes, fontStyle, foreground, background)
                );
            }
        }
        return;
    }

    // Find dot separator
    size_t dotIndex = scope.find('.');
    std::string head;
    std::string tail;

    if (dotIndex == std::string::npos) {
        head = scope;
        tail = "";
    } else {
        head = scope.substr(0, dotIndex);
        tail = scope.substr(dotIndex + 1);
    }

    // Get or create child
    ThemeTrieElement* child = nullptr;
    auto it = _children.find(head);
    if (it != _children.end()) {
        child = it->second;
    } else {
        child = new ThemeTrieElement(
            new ThemeTrieElementRule(scopeDepth, parentScopes, fontStyle, foreground, background),
            std::vector<ThemeTrieElementRule*>()
        );
        _children[head] = child;
    }

    child->insert(scopeDepth + 1, tail, parentScopes, fontStyle, foreground, background);
}

// Helper functions

bool _matchesScope(const ScopeName& scopeName, const ScopeName& scopePattern) {
    return scopePattern == scopeName ||
           (scopeName.length() > scopePattern.length() &&
            scopeName.substr(0, scopePattern.length()) == scopePattern &&
            scopeName[scopePattern.length()] == '.');
}

bool _scopePathMatchesParentScopes(ScopeStack* scopePath, const std::vector<ScopeName>& parentScopes) {
    if (parentScopes.empty()) {
        return true;
    }

    for (size_t index = 0; index < parentScopes.size(); index++) {
        std::string scopePattern = parentScopes[index];
        bool scopeMustMatch = false;

        // Check for child combinator
        if (scopePattern == ">") {
            if (index == parentScopes.size() - 1) {
                return false;
            }
            scopePattern = parentScopes[++index];
            scopeMustMatch = true;
        }

        while (scopePath) {
            if (_matchesScope(scopePath->scopeName, scopePattern)) {
                break;
            }
            if (scopeMustMatch) {
                return false;
            }
            scopePath = scopePath->parent;
        }

        if (!scopePath) {
            return false;
        }
        scopePath = scopePath->parent;
    }

    return true;
}

// Resolve parsed theme rules

Theme* resolveParsedThemeRules(std::vector<ParsedThemeRule*>& parsedThemeRules,
                               const std::vector<std::string>* colorMap) {
    // Sort rules
    std::sort(parsedThemeRules.begin(), parsedThemeRules.end(),
        [](const ParsedThemeRule* a, const ParsedThemeRule* b) {
            int r = strcmp_custom(a->scope, b->scope);
            if (r != 0) return r < 0;
            r = strArrCmp(a->parentScopes, b->parentScopes);
            if (r != 0) return r < 0;
            return a->index < b->index;
        });

    // Determine defaults
    int defaultFontStyle = static_cast<int>(FontStyle::None);
    std::string defaultForeground = "#000000";
    std::string defaultBackground = "#ffffff";

    while (!parsedThemeRules.empty() && parsedThemeRules[0]->scope.empty()) {
        ParsedThemeRule* incomingDefaults = parsedThemeRules[0];
        parsedThemeRules.erase(parsedThemeRules.begin());

        if (incomingDefaults->fontStyle != static_cast<int>(FontStyle::NotSet)) {
            defaultFontStyle = incomingDefaults->fontStyle;
        }
        if (incomingDefaults->foreground != nullptr) {
            defaultForeground = *incomingDefaults->foreground;
        }
        if (incomingDefaults->background != nullptr) {
            defaultBackground = *incomingDefaults->background;
        }

        delete incomingDefaults;
    }

    ColorMap* colorMapObj = new ColorMap(colorMap);
    StyleAttributes* defaults = new StyleAttributes(
        defaultFontStyle,
        colorMapObj->getId(&defaultForeground),
        colorMapObj->getId(&defaultBackground)
    );

    ThemeTrieElement* root = new ThemeTrieElement(
        new ThemeTrieElementRule(0, nullptr, static_cast<int>(FontStyle::NotSet), 0, 0),
        std::vector<ThemeTrieElementRule*>()
    );

    for (auto* rule : parsedThemeRules) {
        root->insert(0, rule->scope, rule->parentScopes,
                    rule->fontStyle,
                    colorMapObj->getId(rule->foreground),
                    colorMapObj->getId(rule->background));
    }

    return new Theme(colorMapObj, defaults, root);
}

// Theme implementation

Theme::Theme(ColorMap* colorMap, StyleAttributes* defaults, ThemeTrieElement* root)
    : _colorMap(colorMap), _defaults(defaults), _root(root) {

    _cachedMatchRoot = new CachedFn<ScopeName, std::vector<ThemeTrieElementRule*>>(
        [this](const ScopeName& scopeName) {
            return this->_root->match(scopeName);
        }
    );
}

Theme::~Theme() {
    delete _colorMap;
    delete _defaults;
    delete _root;
    delete _cachedMatchRoot;
}

Theme* Theme::createFromRawTheme(const IRawTheme* source, const std::vector<std::string>* colorMap) {
    return createFromParsedTheme(parseTheme(source), colorMap);
}

Theme* Theme::createFromParsedTheme(const std::vector<ParsedThemeRule*>& source,
                                    const std::vector<std::string>* colorMap) {
    std::vector<ParsedThemeRule*> mutableSource = source;
    return resolveParsedThemeRules(mutableSource, colorMap);
}

std::vector<std::string> Theme::getColorMap() const {
    return _colorMap->getColorMap();
}

StyleAttributes* Theme::getDefaults() const {
    return _defaults;
}

StyleAttributes* Theme::match(ScopeStack* scopePath) {
    if (scopePath == nullptr) {
        return _defaults;
    }

    const ScopeName& scopeName = scopePath->scopeName;
    std::vector<ThemeTrieElementRule*> matchingTrieElements = _cachedMatchRoot->get(scopeName);

    // Find the effective rule
    ThemeTrieElementRule* effectiveRule = nullptr;
    for (auto* rule : matchingTrieElements) {
        if (_scopePathMatchesParentScopes(scopePath->parent, rule->parentScopes)) {
            effectiveRule = rule;
            break;
        }
    }

    if (!effectiveRule) {
        return nullptr;
    }

    return new StyleAttributes(
        effectiveRule->fontStyle,
        effectiveRule->foreground,
        effectiveRule->background
    );
}

} // namespace vscode_textmate
