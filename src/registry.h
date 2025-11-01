#ifndef VSCODE_TEXTMATE_REGISTRY_H
#define VSCODE_TEXTMATE_REGISTRY_H

#include "types.h"
#include "theme.h"
#include "rawGrammar.h"
#include "onigLib.h"
#include <string>
#include <map>
#include <memory>
#include <functional>

namespace vscode_textmate {

// Forward declarations
class Grammar;
class BalancedBracketSelectors;

// IGrammarRepository interface
class IGrammarRepository {
public:
    virtual ~IGrammarRepository() {}
    virtual IRawGrammar* lookup(const ScopeName& scopeName) = 0;
    virtual std::vector<ScopeName> injections(const ScopeName& scopeName) = 0;
};

// IThemeProvider interface
class IThemeProvider {
public:
    virtual ~IThemeProvider() {}
    virtual StyleAttributes* themeMatch(ScopeStack* scopePath) = 0;
    virtual StyleAttributes* getDefaults() = 0;
};

// SyncRegistry class
class SyncRegistry : public IGrammarRepository, public IThemeProvider {
private:
    std::map<ScopeName, Grammar*> _grammars;
    std::map<ScopeName, IRawGrammar*> _rawGrammars;
    std::map<ScopeName, std::vector<ScopeName>> _injectionGrammars;
    Theme* _theme;
    IOnigLib* _onigLib;

public:
    SyncRegistry(Theme* theme, IOnigLib* onigLib);
    ~SyncRegistry();

    void dispose();
    void setTheme(Theme* theme);
    std::vector<std::string> getColorMap();

    // Add grammar to registry
    void addGrammar(IRawGrammar* grammar, const std::vector<ScopeName>* injectionScopeNames = nullptr);

    // IGrammarRepository implementation
    IRawGrammar* lookup(const ScopeName& scopeName) override;
    std::vector<ScopeName> injections(const ScopeName& scopeName) override;

    // IThemeProvider implementation
    StyleAttributes* themeMatch(ScopeStack* scopePath) override;
    StyleAttributes* getDefaults() override;

    // Get grammar for scope name
    Grammar* grammarForScopeName(
        const ScopeName& scopeName,
        int initialLanguage,
        const EmbeddedLanguagesMap* embeddedLanguages,
        const TokenTypeMap* tokenTypes,
        BalancedBracketSelectors* balancedBracketSelectors
    );
};

// RegistryOptions structure
struct RegistryOptions {
    IOnigLib* onigLib;
    IRawTheme* theme;
    std::vector<std::string>* colorMap;
    std::function<IRawGrammar*(const ScopeName&)> loadGrammar;
    std::function<std::vector<ScopeName>(const ScopeName&)> getInjections;

    RegistryOptions()
        : onigLib(nullptr), theme(nullptr), colorMap(nullptr) {}
};

// IGrammarConfiguration structure
struct IGrammarConfiguration {
    EmbeddedLanguagesMap* embeddedLanguages;
    TokenTypeMap* tokenTypes;
    std::vector<std::string>* balancedBracketSelectors;
    std::vector<std::string>* unbalancedBracketSelectors;

    IGrammarConfiguration()
        : embeddedLanguages(nullptr), tokenTypes(nullptr),
          balancedBracketSelectors(nullptr), unbalancedBracketSelectors(nullptr) {}
};

// Registry class
class Registry {
private:
    RegistryOptions _options;
    SyncRegistry* _syncRegistry;
    std::map<std::string, bool> _ensureGrammarCache;

public:
    explicit Registry(const RegistryOptions& options);
    ~Registry();

    void dispose();

    // Change the theme
    void setTheme(const IRawTheme* theme, const std::vector<std::string>* colorMap = nullptr);

    // Get color map
    std::vector<std::string> getColorMap();

    // Load grammar with embedded languages
    Grammar* loadGrammarWithEmbeddedLanguages(
        const ScopeName& initialScopeName,
        int initialLanguage,
        const EmbeddedLanguagesMap& embeddedLanguages
    );

    // Load grammar with configuration
    Grammar* loadGrammarWithConfiguration(
        const ScopeName& initialScopeName,
        int initialLanguage,
        const IGrammarConfiguration& configuration
    );

    // Load grammar
    Grammar* loadGrammar(const ScopeName& initialScopeName);

    // Add grammar
    Grammar* addGrammar(
        IRawGrammar* rawGrammar,
        const std::vector<std::string>& injections = std::vector<std::string>(),
        int initialLanguage = 0,
        const EmbeddedLanguagesMap* embeddedLanguages = nullptr
    );

private:
    Grammar* _loadGrammar(
        const ScopeName& initialScopeName,
        int initialLanguage,
        const EmbeddedLanguagesMap* embeddedLanguages,
        const TokenTypeMap* tokenTypes,
        BalancedBracketSelectors* balancedBracketSelectors
    );

    void _loadSingleGrammar(const ScopeName& scopeName);
    void _doLoadSingleGrammar(const ScopeName& scopeName);

    Grammar* _grammarForScopeName(
        const ScopeName& scopeName,
        int initialLanguage = 0,
        const EmbeddedLanguagesMap* embeddedLanguages = nullptr,
        const TokenTypeMap* tokenTypes = nullptr,
        BalancedBracketSelectors* balancedBracketSelectors = nullptr
    );
};

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_REGISTRY_H
