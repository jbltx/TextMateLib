#include "grammar.h"
#include "tokenizeString.h"
#include "encodedTokenAttributes.h"
#include "matcher.h"
#include <algorithm>
#include <iostream>

namespace vscode_textmate {

// Static member initialization
StateStackImpl* StateStackImpl::NULL_STATE = nullptr;

// BalancedBracketSelectors implementation

BalancedBracketSelectors::BalancedBracketSelectors(
    const std::vector<std::string>& balancedBracketSelectors,
    const std::vector<std::string>& unbalancedBracketSelectors)
    : _allowAny(false) {

    if (balancedBracketSelectors.empty() && unbalancedBracketSelectors.empty()) {
        _allowAny = true;
    }

    // Create matchers (simplified - full implementation would use createMatchers)
    // For now, store the selectors
}

bool BalancedBracketSelectors::matchesAlways() const {
    return _allowAny && _unbalancedBracketMatchers.empty();
}

bool BalancedBracketSelectors::matchesNever() const {
    return !_allowAny && _balancedBracketMatchers.empty();
}

bool BalancedBracketSelectors::match(const std::vector<std::string>& scopes) const {
    // Simple implementation: returns true if balanced brackets are enabled globally.
    // Full implementation would match against specific scope selectors in _balancedBracketMatchers.
    return _allowAny;
}

// AttributedScopeStack implementation

AttributedScopeStack::AttributedScopeStack(
    AttributedScopeStack* parent_,
    const ScopeName& scopeName_,
    EncodedTokenAttributes tokenAttributes_)
    : parent(parent_), scopeName(scopeName_), tokenAttributes(tokenAttributes_) {
}

AttributedScopeStack::~AttributedScopeStack() {
    // Don't delete parent - it's managed separately
}

AttributedScopeStack* AttributedScopeStack::createRoot(
    const std::string& scopeName,
    EncodedTokenAttributes tokenAttributes) {
    return new AttributedScopeStack(nullptr, scopeName, tokenAttributes);
}

AttributedScopeStack* AttributedScopeStack::createRootAndLookUpScopeName(
    const std::string& scopeName,
    EncodedTokenAttributes tokenAttributes,
    Grammar* grammar) {

    BasicScopeAttributes rawMetadata = grammar->getMetadataForScope(scopeName);
    EncodedTokenAttributes scopeTokenAttributes = EncodedTokenAttributesHelper::set(
        tokenAttributes,
        rawMetadata.languageId,
        rawMetadata.tokenType,
        nullptr,
        static_cast<int>(FontStyle::NotSet),
        0,
        0
    );

    return new AttributedScopeStack(nullptr, scopeName, scopeTokenAttributes);
}

AttributedScopeStack* AttributedScopeStack::push(
    Grammar* grammar,
    const std::string& scopeName) {

    if (scopeName.empty()) {
        return this;
    }

    BasicScopeAttributes rawMetadata = grammar->getMetadataForScope(scopeName);
    EncodedTokenAttributes scopeTokenAttributes = EncodedTokenAttributesHelper::set(
        this->tokenAttributes,
        rawMetadata.languageId,
        rawMetadata.tokenType,
        nullptr,
        static_cast<int>(FontStyle::NotSet),
        0,
        0
    );

    return new AttributedScopeStack(this, scopeName, scopeTokenAttributes);
}

AttributedScopeStack* AttributedScopeStack::pushAttributed(
    const std::string& scopePath,
    Grammar* grammar) {

    if (scopePath.empty()) {
        return this;
    }

    // Check if scopePath contains spaces (multiple scopes)
    if (scopePath.find(' ') == std::string::npos) {
        // This is the common case and much faster - single scope
        return _pushAttributed(this, scopePath, grammar);
    }

    // Split by spaces and push each scope
    std::vector<std::string> scopes;
    std::string currentScope;
    for (char c : scopePath) {
        if (c == ' ') {
            if (!currentScope.empty()) {
                scopes.push_back(currentScope);
                currentScope.clear();
            }
        } else {
            currentScope += c;
        }
    }
    if (!currentScope.empty()) {
        scopes.push_back(currentScope);
    }

    AttributedScopeStack* result = this;
    for (const std::string& scope : scopes) {
        result = _pushAttributed(result, scope, grammar);
    }
    return result;
}

AttributedScopeStack* AttributedScopeStack::_pushAttributed(
    AttributedScopeStack* target,
    const std::string& scopeName,
    Grammar* grammar) {

    if (scopeName.empty()) {
        return target;
    }

    BasicScopeAttributes rawMetadata = grammar->getMetadataForScope(scopeName);

    // Get theme match result
    IThemeProvider* themeProvider = grammar->getThemeProvider();
    StyleAttributes* defaultStyle = themeProvider->getDefaults();

    // Merge attributes
    EncodedTokenAttributes metadata = EncodedTokenAttributesHelper::set(
        target->tokenAttributes,
        rawMetadata.languageId,
        rawMetadata.tokenType,
        nullptr,
        defaultStyle->fontStyle,
        defaultStyle->foregroundId,
        defaultStyle->backgroundId
    );

    return new AttributedScopeStack(target, scopeName, metadata);
}

std::vector<std::string> AttributedScopeStack::getScopeNames() const {
    std::vector<std::string> result;
    const AttributedScopeStack* current = this;
    while (current) {
        result.push_back(current->scopeName);
        current = current->parent;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

bool AttributedScopeStack::equals(AttributedScopeStack* a, AttributedScopeStack* b) {
    if (a == b) return true;
    if (!a || !b) return false;

    while (a && b) {
        if (a->scopeName != b->scopeName || a->tokenAttributes != b->tokenAttributes) {
            return false;
        }
        a = a->parent;
        b = b->parent;
    }

    return a == nullptr && b == nullptr;
}

// StateStackImpl implementation

StateStackImpl::StateStackImpl(
    StateStackImpl* parent_,
    RuleId ruleId_,
    int enterPos_,
    int anchorPos_,
    bool beginRuleCapturedEOL_,
    const std::string* endRule_,
    AttributedScopeStack* nameScopesList_,
    AttributedScopeStack* contentNameScopesList_)
    : parent(parent_),
      ruleId(ruleId_),
      _enterPos(enterPos_),
      _anchorPos(anchorPos_),
      beginRuleCapturedEOL(beginRuleCapturedEOL_),
      endRule(endRule_ ? new std::string(*endRule_) : nullptr),
      nameScopesList(nameScopesList_),
      contentNameScopesList(contentNameScopesList_) {

    depth = parent ? parent->depth + 1 : 1;
}

StateStackImpl::~StateStackImpl() {
    delete endRule;
    // Don't delete nameScopesList and contentNameScopesList - managed separately
}

StateStack* StateStackImpl::clone() {
    return new StateStackImpl(
        parent,
        ruleId,
        _enterPos,
        _anchorPos,
        beginRuleCapturedEOL,
        endRule,
        nameScopesList,
        contentNameScopesList
    );
}

bool StateStackImpl::equals(StateStack* other) {
    if (this == other) return true;
    if (!other) return false;

    StateStackImpl* otherImpl = dynamic_cast<StateStackImpl*>(other);
    if (!otherImpl) return false;

    // Compare all fields
    if (ruleIdToNumber(ruleId) != ruleIdToNumber(otherImpl->ruleId)) return false;
    if (_enterPos != otherImpl->_enterPos) return false;

    bool thisHasEndRule = (endRule != nullptr);
    bool otherHasEndRule = (otherImpl->endRule != nullptr);
    if (thisHasEndRule != otherHasEndRule) return false;
    if (thisHasEndRule && *endRule != *otherImpl->endRule) return false;

    if (!AttributedScopeStack::equals(nameScopesList, otherImpl->nameScopesList)) return false;
    if (!AttributedScopeStack::equals(contentNameScopesList, otherImpl->contentNameScopesList)) return false;

    // Compare parents recursively
    if (parent == nullptr && otherImpl->parent == nullptr) return true;
    if (parent == nullptr || otherImpl->parent == nullptr) return false;

    return parent->equals(otherImpl->parent);
}

void StateStackImpl::reset() {
    // Reset enter and anchor positions
    StateStackImpl* el = this;
    while (el) {
        el->_enterPos = -1;
        el->_anchorPos = -1;
        el = el->parent;
    }
}

StateStackImpl* StateStackImpl::push(
    RuleId ruleId,
    int enterPos,
    int anchorPos,
    bool beginRuleCapturedEOL,
    const std::string* endRule,
    AttributedScopeStack* nameScopesList,
    AttributedScopeStack* contentNameScopesList) {

    return new StateStackImpl(
        this,
        ruleId,
        enterPos,
        anchorPos,
        beginRuleCapturedEOL,
        endRule,
        nameScopesList,
        contentNameScopesList
    );
}

StateStackImpl* StateStackImpl::pop() {
    return this->parent;
}

StateStackImpl* StateStackImpl::safePop() {
    if (this->parent) {
        return this->parent;
    }
    return this;
}

Rule* StateStackImpl::getRule(Grammar* grammar) {
    return grammar->getRule(this->ruleId);
}

StateStackImpl* StateStackImpl::withContentNameScopesList(AttributedScopeStack* contentNameScopesList) {
    if (this->contentNameScopesList == contentNameScopesList) {
        return this;
    }
    return this->parent->push(
        this->ruleId,
        this->_enterPos,
        this->_anchorPos,
        this->beginRuleCapturedEOL,
        this->endRule,
        this->nameScopesList,
        contentNameScopesList
    );
}

StateStackImpl* StateStackImpl::withEndRule(const std::string& endRule) {
    if (this->endRule && *this->endRule == endRule) {
        return this;
    }
    return new StateStackImpl(
        this->parent,
        this->ruleId,
        this->_enterPos,
        this->_anchorPos,
        this->beginRuleCapturedEOL,
        &endRule,
        this->nameScopesList,
        this->contentNameScopesList
    );
}

bool StateStackImpl::hasSameRuleAs(StateStackImpl* other) {
    StateStackImpl* el = this;
    while (el && el->_enterPos == other->_enterPos) {
        if (ruleIdToNumber(el->ruleId) == ruleIdToNumber(other->ruleId)) {
            return true;
        }
        el = el->parent;
    }
    return false;
}

std::string StateStackImpl::toString() const {
    std::string result = "StateStack[";
    const StateStackImpl* current = this;
    while (current) {
        result += "Rule#" + std::to_string(ruleIdToNumber(current->ruleId));
        if (current->parent) result += ", ";
        current = current->parent;
    }
    result += "]";
    return result;
}

// LineTokens implementation

LineTokens::LineTokens(
    bool emitBinaryTokens,
    const std::string& lineText,
    const std::vector<TokenTypeMatcher>& tokenTypeMatchers,
    BalancedBracketSelectors* balancedBracketSelectors)
    : _emitBinaryTokens(emitBinaryTokens),
      _lineText(lineText),
      _tokenTypeMatchers(tokenTypeMatchers),
      _balancedBracketSelectors(balancedBracketSelectors),
      _lastTokenEndIndex(0) {
}

void LineTokens::produce(StateStackImpl* stack, int endIndex) {
    produceFromScopes(stack->contentNameScopesList, endIndex);
}

void LineTokens::produceFromScopes(AttributedScopeStack* scopesList, int endIndex) {
    if (_lastTokenEndIndex >= endIndex) {
        return;
    }

    if (_emitBinaryTokens) {
        _binaryTokens.push_back(_lastTokenEndIndex);
        _binaryTokens.push_back(scopesList->tokenAttributes);
        _lastTokenEndIndex = endIndex;
    } else {
        IToken token;
        token.startIndex = _lastTokenEndIndex;
        token.endIndex = endIndex;
        token.scopes = scopesList->getScopeNames();
        _tokens.push_back(token);
        _lastTokenEndIndex = endIndex;
    }
}

std::vector<IToken> LineTokens::getResult(StateStackImpl* stack, int lineLength) {
    // Remove token for newline if it exists
    if (!_tokens.empty() && _tokens.back().startIndex == lineLength - 1) {
        _tokens.pop_back();
    }

    // If no tokens, produce one for the entire line
    if (_tokens.empty()) {
        _lastTokenEndIndex = -1;
        produce(stack, lineLength);
        if (!_tokens.empty()) {
            _tokens.back().startIndex = 0;
        }
    }

    return _tokens;
}

std::vector<uint32_t> LineTokens::getBinaryResult(StateStackImpl* stack, int lineLength) {
    return _binaryTokens;
}

// Grammar implementation

Grammar::Grammar(
    const ScopeName& rootScopeName,
    IRawGrammar* grammar,
    int initialLanguage,
    const EmbeddedLanguagesMap* embeddedLanguages,
    const TokenTypeMap* tokenTypes,
    BalancedBracketSelectors* balancedBracketSelectors_,
    IGrammarRepository* grammarRepository,
    IThemeProvider* themeProvider,
    IOnigLib* onigLib)
    : _rootScopeName(rootScopeName),
      _rootId(ruleIdFromNumber(-1)),
      _lastRuleId(0),
      _grammarRepository(grammarRepository),
      _themeProvider(themeProvider),
      _grammar(grammar),
      _injections(nullptr),
      balancedBracketSelectors(balancedBracketSelectors_),
      _onigLib(onigLib) {

    _ruleId2desc.push_back(nullptr); // Index 0 is null

    _basicScopeAttributesProvider = new BasicScopeAttributesProvider(
        initialLanguage,
        embeddedLanguages
    );

    _grammar = initGrammar(grammar, nullptr);

    // Build token type matchers
    if (tokenTypes) {
        for (const auto& pair : *tokenTypes) {
            // Simple implementation: creates basic matchers without selector parsing.
            // Full implementation would use createMatchers to parse scope selectors.
            TokenTypeMatcher matcher;
            matcher.type = pair.second;
            _tokenTypeMatchers.push_back(matcher);
        }
    }
}

Grammar::~Grammar() {
    dispose();
}

void Grammar::dispose() {
    for (size_t i = 0; i < _ruleId2desc.size(); i++) {
        auto* rule = _ruleId2desc[i];
        if (rule) {
            rule->dispose();
            delete rule;
        }
    }
    _ruleId2desc.clear();

    if (_basicScopeAttributesProvider) {
        delete _basicScopeAttributesProvider;
        _basicScopeAttributesProvider = nullptr;
    }
    if (_injections) {
        delete _injections;
        _injections = nullptr;
    }
    if (balancedBracketSelectors) {
        delete balancedBracketSelectors;
        balancedBracketSelectors = nullptr;
    }
}

OnigScanner* Grammar::createOnigScanner(const std::vector<std::string>& sources) {
    return _onigLib->createOnigScanner(sources);
}

OnigString* Grammar::createOnigString(const std::string& str) {
    return _onigLib->createOnigString(str);
}

BasicScopeAttributes Grammar::getMetadataForScope(const std::string& scope) {
    return _basicScopeAttributesProvider->getBasicScopeAttributes(&scope);
}

Rule* Grammar::getRule(RuleId ruleId) {
    int id = ruleIdToNumber(ruleId);
    if (id >= 0 && id < static_cast<int>(_ruleId2desc.size())) {
        return _ruleId2desc[id];
    }
    return nullptr;
}

RuleId Grammar::registerRule(Rule* rule) {
    int id = ++_lastRuleId;
    if (_ruleId2desc.size() <= static_cast<size_t>(id)) {
        _ruleId2desc.resize(id + 1, nullptr);
    }
    _ruleId2desc[id] = rule;
    return ruleIdFromNumber(id);
}

RuleId Grammar::allocateRuleId() {
    int id = ++_lastRuleId;
    if (_ruleId2desc.size() <= static_cast<size_t>(id)) {
        _ruleId2desc.resize(id + 1, nullptr);
    }
    return ruleIdFromNumber(id);
}

void Grammar::setRule(RuleId ruleId, Rule* rule) {
    int id = ruleIdToNumber(ruleId);
    if (id >= 0 && id < static_cast<int>(_ruleId2desc.size())) {
        _ruleId2desc[id] = rule;
    }
}

IRawGrammar* Grammar::getExternalGrammar(const std::string& scopeName, IRawRepository* repository) {
    auto it = _includedGrammars.find(scopeName);
    if (it != _includedGrammars.end()) {
        return it->second;
    }

    if (_grammarRepository) {
        IRawGrammar* rawIncludedGrammar = _grammarRepository->lookup(scopeName);
        if (rawIncludedGrammar) {
            IRawRule* base = (repository && repository->baseRule) ? repository->baseRule : nullptr;
            _includedGrammars[scopeName] = initGrammar(rawIncludedGrammar, base);
            return _includedGrammars[scopeName];
        } else {
        }
    } else {
    }

    return nullptr;
}

std::vector<Injection> Grammar::getInjections() {
    if (_injections == nullptr) {
        _injections = new std::vector<Injection>(_collectInjections());
    }
    return *_injections;
}

// Helper function: Check if two scope names match (exact or prefix match)
static bool scopesAreMatching(const std::string& thisScopeName, const std::string& scopeName) {
    if (thisScopeName.empty()) {
        return false;
    }
    if (thisScopeName == scopeName) {
        return true;
    }
    size_t len = scopeName.length();
    return thisScopeName.length() > len &&
           thisScopeName.substr(0, len) == scopeName &&
           thisScopeName[len] == '.';
}

// Helper function: Match identifiers against scopes
static bool nameMatcher(const std::vector<std::string>& identifiers,
                       const std::vector<std::string>& scopes) {
    if (scopes.size() < identifiers.size()) {
        return false;
    }
    size_t lastIndex = 0;
    for (const auto& identifier : identifiers) {
        bool found = false;
        for (size_t i = lastIndex; i < scopes.size(); i++) {
            if (scopesAreMatching(scopes[i], identifier)) {
                lastIndex = i + 1;
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

// Helper function: Collect injections from a single injection rule
static void collectInjections(std::vector<Injection>& result,
                              const std::string& selector,
                              IRawRule* rule,
                              Grammar* grammar,
                              IRawGrammar* grammarDef) {
    if (!rule) {
        return;
    }

    // Create matchers from the selector
    auto matchers = createMatchers<std::vector<std::string>>(selector, nameMatcher);

    // Get the compiled rule ID
    RuleId ruleId = RuleFactory::getCompiledRuleId(rule, grammar, grammarDef->repository);

    // Add an injection for each matcher
    for (const auto& matcherWithPriority : matchers) {
        Injection injection;
        injection.debugSelector = selector;
        injection.matcher = matcherWithPriority.matcher;
        injection.ruleId = ruleId;
        injection.grammar = grammarDef;
        injection.priority = matcherWithPriority.priority;
        result.push_back(injection);
    }
}

std::vector<Injection> Grammar::_collectInjections() {
    std::vector<Injection> result;

    // Get the current grammar
    IRawGrammar* grammar = _grammar;
    if (!grammar) {
        return result;
    }

    // Add injections from the current grammar
    if (grammar->injections) {
        for (const auto& pair : *grammar->injections) {
            const std::string& expression = pair.first;
            IRawRule* rule = pair.second;
            collectInjections(result, expression, rule, this, grammar);
        }
    }

    // Add injection grammars contributed for the current scope
    if (_grammarRepository) {
        std::vector<std::string> injectionScopeNames = _grammarRepository->injections(_rootScopeName);
        for (const auto& injectionScopeName : injectionScopeNames) {
            IRawGrammar* injectionGrammar = getExternalGrammar(injectionScopeName, nullptr);
            if (injectionGrammar) {
                const std::string* selector = injectionGrammar->injectionSelector;
                if (selector && !selector->empty()) {
                    // Use the injection grammar's $self rule which contains the patterns
                    // After initGrammar, the patterns are moved to repository->selfRule
                    IRawRule* injectionRule = (injectionGrammar->repository && injectionGrammar->repository->selfRule)
                        ? injectionGrammar->repository->selfRule
                        : injectionGrammar;
                    collectInjections(result, *selector, injectionRule, this, injectionGrammar);
                }
            }
        }
    }

    // Sort by priority
    std::sort(result.begin(), result.end(), [](const Injection& a, const Injection& b) {
        return a.priority < b.priority;
    });

    return result;
}

ITokenizeLineResult Grammar::tokenizeLine(
    const std::string& lineText,
    StateStack* prevState,
    int timeLimit) {

    StateStackImpl* prevStateImpl = dynamic_cast<StateStackImpl*>(prevState);
    TokenizeResult r = _tokenize(lineText, prevStateImpl, false, timeLimit);

    ITokenizeLineResult result;
    result.tokens = r.lineTokens->getResult(r.ruleStack, r.lineLength);
    result.ruleStack = r.ruleStack;
    result.stoppedEarly = r.stoppedEarly;

    delete r.lineTokens;
    return result;
}

ITokenizeLineResult2 Grammar::tokenizeLine2(
    const std::string& lineText,
    StateStack* prevState,
    int timeLimit) {

    StateStackImpl* prevStateImpl = dynamic_cast<StateStackImpl*>(prevState);
    TokenizeResult r = _tokenize(lineText, prevStateImpl, true, timeLimit);

    ITokenizeLineResult2 result;
    result.tokens = r.lineTokens->getBinaryResult(r.ruleStack, r.lineLength);
    result.ruleStack = r.ruleStack;
    result.stoppedEarly = r.stoppedEarly;

    delete r.lineTokens;
    return result;
}

Grammar::TokenizeResult Grammar::_tokenize(
    const std::string& lineText,
    StateStackImpl* prevState,
    bool emitBinaryTokens,
    int timeLimit) {

    // Initialize root rule if needed
    if (ruleIdToNumber(_rootId) == -1) {
        _rootId = RuleFactory::getCompiledRuleId(
            _grammar->repository->selfRule,
            this,
            _grammar->repository
        );
        getInjections();
    }

    bool isFirstLine;
    if (!prevState || prevState == StateStackImpl::NULL_STATE) {
        isFirstLine = true;

        BasicScopeAttributes rawDefaultMetadata =
            _basicScopeAttributesProvider->getDefaultAttributes();
        StyleAttributes* defaultStyle = _themeProvider->getDefaults();

        EncodedTokenAttributes defaultMetadata = EncodedTokenAttributesHelper::set(
            0,
            rawDefaultMetadata.languageId,
            rawDefaultMetadata.tokenType,
            nullptr,
            defaultStyle->fontStyle,
            defaultStyle->foregroundId,
            defaultStyle->backgroundId
        );

        Rule* rootRule = getRule(_rootId);
        std::string* rootScopeName = rootRule ? rootRule->getName(nullptr, nullptr) : nullptr;

        AttributedScopeStack* scopeList;
        if (rootScopeName) {
            scopeList = AttributedScopeStack::createRootAndLookUpScopeName(
                *rootScopeName,
                defaultMetadata,
                this
            );
            delete rootScopeName;
        } else {
            scopeList = AttributedScopeStack::createRoot("unknown", defaultMetadata);
        }

        prevState = new StateStackImpl(
            nullptr,
            _rootId,
            -1,
            -1,
            false,
            nullptr,
            scopeList,
            scopeList
        );
    } else {
        isFirstLine = false;
        prevState->reset();
    }

    std::string lineTextWithNewline = lineText + "\n";
    OnigString* onigLineText = createOnigString(lineTextWithNewline);
    int lineLength = onigLineText->content().length();

    LineTokens* lineTokens = new LineTokens(
        emitBinaryTokens,
        lineTextWithNewline,
        _tokenTypeMatchers,
        balancedBracketSelectors
    );

    StackElement resultStack = tokenizeString(
        this,
        onigLineText,
        isFirstLine,
        0,
        prevState,
        lineTokens,
        true,
        timeLimit
    );

    disposeOnigString(onigLineText);

    TokenizeResult result;
    result.lineLength = lineLength;
    result.lineTokens = lineTokens;
    result.ruleStack = resultStack.stack;
    result.stoppedEarly = resultStack.stoppedEarly;

    return result;
}

// Helper functions

Grammar* createGrammar(
    const ScopeName& scopeName,
    IRawGrammar* grammar,
    int initialLanguage,
    const EmbeddedLanguagesMap* embeddedLanguages,
    const TokenTypeMap* tokenTypes,
    BalancedBracketSelectors* balancedBracketSelectors,
    IGrammarRepository* grammarRepository,
    IThemeProvider* themeProvider,
    IOnigLib* onigLib) {

    return new Grammar(
        scopeName,
        grammar,
        initialLanguage,
        embeddedLanguages,
        tokenTypes,
        balancedBracketSelectors,
        grammarRepository,
        themeProvider,
        onigLib
    );
}

IRawGrammar* initGrammar(IRawGrammar* grammar, IRawRule* base) {
    // Create repository if it doesn't exist
    if (!grammar->repository) {
        grammar->repository = new IRawRepository();
    }

    // Create $self rule with grammar's patterns and scope name
    IRawRule* selfRule = new IRawRule();
    // Transfer ownership of patterns from grammar to $self rule
    // This avoids double-free when both grammar and selfRule are destroyed
    if (grammar->patterns && !grammar->patterns->empty()) {
        selfRule->patterns = new std::vector<IRawRule*>(*grammar->patterns);
        // Clear grammar->patterns so we don't have shared ownership
        // The IRawRule* objects are now owned only by selfRule->patterns
        grammar->patterns->clear();
    }
    // Set name to grammar's scopeName
    selfRule->name = new std::string(grammar->scopeName);

    grammar->repository->selfRule = selfRule;

    // Create $base rule
    grammar->repository->baseRule = base ? base : selfRule;

    return grammar;
}

} // namespace vscode_textmate
