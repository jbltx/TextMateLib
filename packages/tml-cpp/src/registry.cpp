#include "registry.h"
#include "grammar.h"
#include "grammarDependencies.h"
#include <iostream>

namespace tml {

// SyncRegistry implementation

SyncRegistry::SyncRegistry(Theme* theme, IOnigLib* onigLib)
    : _theme(theme), _onigLib(onigLib) {
}

SyncRegistry::~SyncRegistry() {
    dispose();
}

void SyncRegistry::dispose() {
    for (auto& pair : _grammars) {
        if (pair.second) {
            pair.second->dispose();
            delete pair.second;
        }
    }
    _grammars.clear();

    // Note: _rawGrammars are owned by the caller, don't delete them
    _rawGrammars.clear();
}

void SyncRegistry::setTheme(Theme* theme) {
    _theme = theme;
}

std::vector<std::string> SyncRegistry::getColorMap() {
    return _theme->getColorMap();
}

void SyncRegistry::addGrammar(IRawGrammar* grammar, const std::vector<ScopeName>* injectionScopeNames) {
    _rawGrammars[grammar->scopeName] = grammar;

    if (injectionScopeNames) {
        _injectionGrammars[grammar->scopeName] = *injectionScopeNames;
    }
}

IRawGrammar* SyncRegistry::lookup(const ScopeName& scopeName) {
    auto it = _rawGrammars.find(scopeName);
    if (it != _rawGrammars.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<ScopeName> SyncRegistry::injections(const ScopeName& scopeName) {
    auto it = _injectionGrammars.find(scopeName);
    if (it != _injectionGrammars.end()) {
        return it->second;
    }
    return std::vector<ScopeName>();
}

StyleAttributes* SyncRegistry::themeMatch(ScopeStack* scopePath) {
    return _theme->match(scopePath);
}

StyleAttributes* SyncRegistry::getDefaults() {
    return _theme->getDefaults();
}

Grammar* SyncRegistry::grammarForScopeName(
    const ScopeName& scopeName,
    int initialLanguage,
    const EmbeddedLanguagesMap* embeddedLanguages,
    const TokenTypeMap* tokenTypes,
    BalancedBracketSelectors* balancedBracketSelectors) {

    auto it = _grammars.find(scopeName);
    if (it == _grammars.end()) {
        IRawGrammar* rawGrammar = lookup(scopeName);
        if (!rawGrammar) {
            return nullptr;
        }

        Grammar* grammar = createGrammar(
            scopeName,
            rawGrammar,
            initialLanguage,
            embeddedLanguages,
            tokenTypes,
            balancedBracketSelectors,
            this,
            this,
            _onigLib
        );

        _grammars[scopeName] = grammar;
        return grammar;
    }

    return it->second;
}

// Registry implementation

Registry::Registry(const RegistryOptions& options)
    : _options(options) {

    Theme* theme = Theme::createFromRawTheme(options.theme, options.colorMap);
    _syncRegistry = new SyncRegistry(theme, options.onigLib);
}

Registry::~Registry() {
    dispose();
}

void Registry::dispose() {
    if (_syncRegistry) {
        _syncRegistry->dispose();
        delete _syncRegistry;
        _syncRegistry = nullptr;
    }
}

void Registry::setTheme(const IRawTheme* theme, const std::vector<std::string>* colorMap) {
    Theme* newTheme = Theme::createFromRawTheme(theme, colorMap);
    _syncRegistry->setTheme(newTheme);
}

std::vector<std::string> Registry::getColorMap() {
    return _syncRegistry->getColorMap();
}

Grammar* Registry::loadGrammarWithEmbeddedLanguages(
    const ScopeName& initialScopeName,
    int initialLanguage,
    const EmbeddedLanguagesMap& embeddedLanguages) {

    return _loadGrammar(initialScopeName, initialLanguage, &embeddedLanguages, nullptr, nullptr);
}

Grammar* Registry::loadGrammarWithConfiguration(
    const ScopeName& initialScopeName,
    int initialLanguage,
    const IGrammarConfiguration& configuration) {

    BalancedBracketSelectors* balancedBracketSelectors = nullptr;
    if (configuration.balancedBracketSelectors || configuration.unbalancedBracketSelectors) {
        std::vector<std::string> balanced = configuration.balancedBracketSelectors ?
            *configuration.balancedBracketSelectors : std::vector<std::string>();
        std::vector<std::string> unbalanced = configuration.unbalancedBracketSelectors ?
            *configuration.unbalancedBracketSelectors : std::vector<std::string>();
        balancedBracketSelectors = new BalancedBracketSelectors(balanced, unbalanced);
    }

    return _loadGrammar(
        initialScopeName,
        initialLanguage,
        configuration.embeddedLanguages,
        configuration.tokenTypes,
        balancedBracketSelectors
    );
}

Grammar* Registry::loadGrammar(const ScopeName& initialScopeName) {
    return _loadGrammar(initialScopeName, 0, nullptr, nullptr, nullptr);
}

Grammar* Registry::addGrammar(
    IRawGrammar* rawGrammar,
    const std::vector<std::string>& injections,
    int initialLanguage,
    const EmbeddedLanguagesMap* embeddedLanguages) {

    _syncRegistry->addGrammar(rawGrammar, injections.empty() ? nullptr : &injections);

    // Note: Unlike _loadGrammar, addGrammar does NOT automatically load dependencies.
    // The caller is responsible for ensuring all required grammars are already loaded.
    // This matches the TypeScript implementation.

    return _grammarForScopeName(rawGrammar->scopeName, initialLanguage, embeddedLanguages);
}

Grammar* Registry::_loadGrammar(
    const ScopeName& initialScopeName,
    int initialLanguage,
    const EmbeddedLanguagesMap* embeddedLanguages,
    const TokenTypeMap* tokenTypes,
    BalancedBracketSelectors* balancedBracketSelectors) {

    ScopeDependencyProcessor dependencyProcessor(_syncRegistry, initialScopeName);

    while (!dependencyProcessor.Q.empty()) {
        // Save references to process
        std::vector<AbsoluteRuleReference> toProcess;
        while (!dependencyProcessor.Q.empty()) {
            toProcess.push_back(dependencyProcessor.Q.front());
            dependencyProcessor.Q.pop();
        }

        // Load the grammars
        for (const auto& ref : toProcess) {
            _loadSingleGrammar(ref.scopeName);
        }

        // Put references back in queue for processQueue to scan
        for (const auto& ref : toProcess) {
            dependencyProcessor.Q.push(ref);
        }

        // Scan those grammars for dependencies
        dependencyProcessor.processQueue();
    }

    return _grammarForScopeName(
        initialScopeName,
        initialLanguage,
        embeddedLanguages,
        tokenTypes,
        balancedBracketSelectors
    );
}

void Registry::_loadSingleGrammar(const ScopeName& scopeName) {
    if (_ensureGrammarCache.find(scopeName) != _ensureGrammarCache.end()) {
        return;
    }

    _ensureGrammarCache[scopeName] = true;
    _doLoadSingleGrammar(scopeName);
}

void Registry::_doLoadSingleGrammar(const ScopeName& scopeName) {
    IRawGrammar* grammar = _options.loadGrammar(scopeName);
    if (grammar) {
        std::vector<ScopeName> injections;
        if (_options.getInjections) {
            injections = _options.getInjections(scopeName);
        }
        _syncRegistry->addGrammar(grammar, injections.empty() ? nullptr : &injections);

        // Also load the injection grammars themselves
        for (const auto& injectionScopeName : injections) {
            _loadSingleGrammar(injectionScopeName);
        }
    }
}

Grammar* Registry::_grammarForScopeName(
    const ScopeName& scopeName,
    int initialLanguage,
    const EmbeddedLanguagesMap* embeddedLanguages,
    const TokenTypeMap* tokenTypes,
    BalancedBracketSelectors* balancedBracketSelectors) {

    return _syncRegistry->grammarForScopeName(
        scopeName,
        initialLanguage,
        embeddedLanguages,
        tokenTypes,
        balancedBracketSelectors
    );
}

} // namespace tml
