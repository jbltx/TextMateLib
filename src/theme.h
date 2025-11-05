#ifndef TEXTMATELIB_THEME_H
#define TEXTMATELIB_THEME_H

#include "types.h"
#include "utils.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace tml {

// Forward declarations
class Theme;
class ThemeTrieElement;
class ColorMap;

// IRawTheme interface
struct IRawThemeSetting {
    std::string* name;
    std::vector<std::string>* scope;  // Can be single string or array
    std::string scopeString;          // Single scope string

    struct Settings {
        std::string* fontStyle;
        std::string* foreground;
        std::string* background;

        Settings() : fontStyle(nullptr), foreground(nullptr), background(nullptr) {}
        ~Settings() {
            delete fontStyle;
            delete foreground;
            delete background;
        }
    } settings;

    IRawThemeSetting() : name(nullptr), scope(nullptr) {}
    ~IRawThemeSetting() {
        delete name;
        delete scope;
    }
};

struct IRawTheme {
    std::string* name;
    std::vector<IRawThemeSetting*> settings;

    IRawTheme() : name(nullptr) {}
    ~IRawTheme() {
        delete name;
        for (auto* setting : settings) {
            delete setting;
        }
    }
};

// ScopeStack class
class ScopeStack {
public:
    ScopeStack* parent;
    ScopeName scopeName;

    ScopeStack(ScopeStack* parent_, const ScopeName& scopeName_);
    ~ScopeStack();

    static ScopeStack* push(ScopeStack* path, const std::vector<ScopeName>& scopeNames);
    static ScopeStack* from(const std::vector<ScopeName>& segments);

    ScopeStack* push(const ScopeName& scopeName);
    std::vector<ScopeName> getSegments() const;
    std::string toString() const;
    bool extends(const ScopeStack* other) const;
    std::vector<ScopeName>* getExtensionIfDefined(const ScopeStack* base) const;
};

// StyleAttributes class
class StyleAttributes {
public:
    int fontStyle;
    int foregroundId;
    int backgroundId;

    StyleAttributes(int fontStyle_, int foregroundId_, int backgroundId_);
};

// ParsedThemeRule class
class ParsedThemeRule {
public:
    ScopeName scope;
    std::vector<ScopeName>* parentScopes;
    int index;
    int fontStyle;
    std::string* foreground;
    std::string* background;

    ParsedThemeRule(const ScopeName& scope_,
                   std::vector<ScopeName>* parentScopes_,
                   int index_,
                   int fontStyle_,
                   const std::string* foreground_,
                   const std::string* background_);
    ~ParsedThemeRule();
};

// Parse theme from raw theme
std::vector<ParsedThemeRule*> parseTheme(const IRawTheme* source);

// Convert font style to string
std::string fontStyleToString(int fontStyle);

// ColorMap class
class ColorMap {
private:
    bool _isFrozen;
    int _lastColorId;
    std::vector<std::string> _id2color;
    std::map<std::string, int> _color2id;

public:
    ColorMap(const std::vector<std::string>* colorMap = nullptr);

    int getId(const std::string* color);
    std::vector<std::string> getColorMap() const;
};

// ThemeTrieElementRule class
class ThemeTrieElementRule {
public:
    int scopeDepth;
    std::vector<ScopeName> parentScopes;
    int fontStyle;
    int foreground;
    int background;

    ThemeTrieElementRule(int scopeDepth_,
                        const std::vector<ScopeName>* parentScopes_,
                        int fontStyle_,
                        int foreground_,
                        int background_);

    ThemeTrieElementRule* clone() const;
    static std::vector<ThemeTrieElementRule*> cloneArr(const std::vector<ThemeTrieElementRule*>& arr);

    void acceptOverwrite(int scopeDepth_, int fontStyle_, int foreground_, int background_);
};

// ThemeTrieElement class
class ThemeTrieElement {
private:
    ThemeTrieElementRule* _mainRule;
    std::vector<ThemeTrieElementRule*> _rulesWithParentScopes;
    std::map<std::string, ThemeTrieElement*> _children;

public:
    ThemeTrieElement(ThemeTrieElementRule* mainRule,
                    const std::vector<ThemeTrieElementRule*>& rulesWithParentScopes = std::vector<ThemeTrieElementRule*>());
    ~ThemeTrieElement();

    std::vector<ThemeTrieElementRule*> match(const ScopeName& scope);

    void insert(int scopeDepth,
               const std::string& scope,
               const std::vector<ScopeName>* parentScopes,
               int fontStyle,
               int foreground,
               int background);
};

// Theme class
class Theme {
private:
    ColorMap* _colorMap;
    StyleAttributes* _defaults;
    ThemeTrieElement* _root;
    CachedFn<ScopeName, std::vector<ThemeTrieElementRule*>>* _cachedMatchRoot;

public:
    Theme(ColorMap* colorMap, StyleAttributes* defaults, ThemeTrieElement* root);
    ~Theme();

    static Theme* createFromRawTheme(const IRawTheme* source, const std::vector<std::string>* colorMap = nullptr);
    static Theme* createFromParsedTheme(const std::vector<ParsedThemeRule*>& source, const std::vector<std::string>* colorMap = nullptr);

    std::vector<std::string> getColorMap() const;
    StyleAttributes* getDefaults() const;
    StyleAttributes* match(ScopeStack* scopePath);
};

// Helper functions
bool _matchesScope(const ScopeName& scopeName, const ScopeName& scopePattern);
bool _scopePathMatchesParentScopes(ScopeStack* scopePath, const std::vector<ScopeName>& parentScopes);

// Resolve parsed theme rules
Theme* resolveParsedThemeRules(std::vector<ParsedThemeRule*>& parsedThemeRules,
                               const std::vector<std::string>* colorMap);

} // namespace tml

#endif // TEXTMATELIB_THEME_H
