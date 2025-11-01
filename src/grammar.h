#ifndef VSCODE_TEXTMATE_GRAMMAR_H
#define VSCODE_TEXTMATE_GRAMMAR_H

#include "types.h"
#include "rule.h"
#include "theme.h"
#include "onigLib.h"
#include "rawGrammar.h"
#include "registry.h"
#include "basicScopesAttributeProvider.h"
#include "matcher.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace vscode_textmate {

// Forward declarations
class StateStackImpl;
class AttributedScopeStack;
class LineTokens;
struct TokenTypeMatcher;

// IToken interface
struct IToken {
    int startIndex;
    int endIndex;
    std::vector<std::string> scopes;

    IToken() : startIndex(0), endIndex(0) {}
};

// ITokenizeLineResult interface
struct ITokenizeLineResult {
    std::vector<IToken> tokens;
    StateStack* ruleStack;
    bool stoppedEarly;

    ITokenizeLineResult() : ruleStack(nullptr), stoppedEarly(false) {}
};

// ITokenizeLineResult2 interface
struct ITokenizeLineResult2 {
    std::vector<uint32_t> tokens; // Uint32Array equivalent
    StateStack* ruleStack;
    bool stoppedEarly;

    ITokenizeLineResult2() : ruleStack(nullptr), stoppedEarly(false) {}
};

// IGrammar interface
class IGrammar {
public:
    virtual ~IGrammar() {}

    virtual ITokenizeLineResult tokenizeLine(
        const std::string& lineText,
        StateStack* prevState,
        int timeLimit = 0
    ) = 0;

    virtual ITokenizeLineResult2 tokenizeLine2(
        const std::string& lineText,
        StateStack* prevState,
        int timeLimit = 0
    ) = 0;
};

// Injection structure
struct Injection {
    std::string debugSelector;
    Matcher<std::vector<std::string>> matcher;
    int priority; // -1, 0, or 1
    RuleId ruleId;
    IRawGrammar* grammar;

    Injection() : priority(0), ruleId(ruleIdFromNumber(-1)), grammar(nullptr) {}
};

// TokenTypeMatcher structure
struct TokenTypeMatcher {
    Matcher<std::vector<std::string>> matcher;
    StandardTokenType type;

    TokenTypeMatcher() : type(StandardTokenType::Other) {}
};

// BalancedBracketSelectors class
class BalancedBracketSelectors {
private:
    std::vector<Matcher<std::vector<std::string>>> _balancedBracketMatchers;
    std::vector<Matcher<std::vector<std::string>>> _unbalancedBracketMatchers;
    bool _allowAny;

public:
    BalancedBracketSelectors(
        const std::vector<std::string>& balancedBracketSelectors,
        const std::vector<std::string>& unbalancedBracketSelectors
    );

    bool matchesAlways() const;
    bool matchesNever() const;
    bool match(const std::vector<std::string>& scopes) const;
};

// AttributedScopeStack class
class AttributedScopeStack {
public:
    AttributedScopeStack* parent;
    ScopeName scopeName;
    EncodedTokenAttributes tokenAttributes;

    AttributedScopeStack(
        AttributedScopeStack* parent_,
        const ScopeName& scopeName_,
        EncodedTokenAttributes tokenAttributes_
    );
    ~AttributedScopeStack();

    static AttributedScopeStack* createRoot(
        const std::string& scopeName,
        EncodedTokenAttributes tokenAttributes
    );

    static AttributedScopeStack* createRootAndLookUpScopeName(
        const std::string& scopeName,
        EncodedTokenAttributes tokenAttributes,
        Grammar* grammar
    );

    AttributedScopeStack* push(
        Grammar* grammar,
        const std::string& scopeName
    );

    AttributedScopeStack* pushAttributed(
        const std::string& scopePath,
        Grammar* grammar
    );

    std::vector<std::string> getScopeNames() const;

    static bool equals(AttributedScopeStack* a, AttributedScopeStack* b);

private:
    static AttributedScopeStack* _pushAttributed(
        AttributedScopeStack* target,
        const std::string& scopeName,
        Grammar* grammar
    );
};

// StateStackImpl class (StateStack implementation)
class StateStackImpl : public StateStack {
private:
    int _enterPos;
    int _anchorPos;

public:
    static StateStackImpl* NULL_STATE;

    StateStackImpl* parent;
    RuleId ruleId;
    bool beginRuleCapturedEOL;
    std::string* endRule;
    AttributedScopeStack* nameScopesList;
    AttributedScopeStack* contentNameScopesList;

    StateStackImpl(
        StateStackImpl* parent_,
        RuleId ruleId_,
        int enterPos_,
        int anchorPos_,
        bool beginRuleCapturedEOL_,
        const std::string* endRule_,
        AttributedScopeStack* nameScopesList_,
        AttributedScopeStack* contentNameScopesList_
    );

    ~StateStackImpl();

    // StateStack interface implementation
    int depth;
    int getDepth() const override { return depth; }
    StateStack* clone() override;
    bool equals(StateStack* other) override;

    void reset();

    // Stack manipulation
    StateStackImpl* push(
        RuleId ruleId,
        int enterPos,
        int anchorPos,
        bool beginRuleCapturedEOL,
        const std::string* endRule,
        AttributedScopeStack* nameScopesList,
        AttributedScopeStack* contentNameScopesList
    );

    StateStackImpl* pop();
    StateStackImpl* safePop();

    // Accessors
    int getEnterPos() const { return _enterPos; }
    int getAnchorPos() const { return _anchorPos; }
    Rule* getRule(Grammar* grammar);

    // State modification
    StateStackImpl* withContentNameScopesList(AttributedScopeStack* contentNameScopesList);
    StateStackImpl* withEndRule(const std::string& endRule);

    // Comparison
    bool hasSameRuleAs(StateStackImpl* other);

    std::string toString() const;
};

// LineTokens class
class LineTokens {
private:
    bool _emitBinaryTokens;
    std::string _lineText;
    std::vector<TokenTypeMatcher> _tokenTypeMatchers;
    BalancedBracketSelectors* _balancedBracketSelectors;

    std::vector<IToken> _tokens;
    std::vector<uint32_t> _binaryTokens;
    int _lastTokenEndIndex;

public:
    LineTokens(
        bool emitBinaryTokens,
        const std::string& lineText,
        const std::vector<TokenTypeMatcher>& tokenTypeMatchers,
        BalancedBracketSelectors* balancedBracketSelectors
    );

    void produce(StateStackImpl* stack, int endIndex);
    void produceFromScopes(AttributedScopeStack* scopesList, int endIndex);

    std::vector<IToken> getResult(StateStackImpl* stack, int lineLength);
    std::vector<uint32_t> getBinaryResult(StateStackImpl* stack, int lineLength);
};

// Grammar class
class Grammar : public IGrammar, public IRuleFactoryHelper, public IOnigLib {
private:
    ScopeName _rootScopeName;
    RuleId _rootId;
    int _lastRuleId;
    std::vector<Rule*> _ruleId2desc;
    std::map<std::string, IRawGrammar*> _includedGrammars;
    IGrammarRepository* _grammarRepository;
    IThemeProvider* _themeProvider;
    IRawGrammar* _grammar;
    std::vector<Injection>* _injections;
    BasicScopeAttributesProvider* _basicScopeAttributesProvider;
    std::vector<TokenTypeMatcher> _tokenTypeMatchers;
    IOnigLib* _onigLib;

public:
    BalancedBracketSelectors* balancedBracketSelectors;

    Grammar(
        const ScopeName& rootScopeName,
        IRawGrammar* grammar,
        int initialLanguage,
        const EmbeddedLanguagesMap* embeddedLanguages,
        const TokenTypeMap* tokenTypes,
        BalancedBracketSelectors* balancedBracketSelectors_,
        IGrammarRepository* grammarRepository,
        IThemeProvider* themeProvider,
        IOnigLib* onigLib
    );

    ~Grammar();

    void dispose();

    IThemeProvider* getThemeProvider() const { return _themeProvider; }

    size_t getRuleCount() const { return _ruleId2desc.size(); }

    // IOnigLib implementation
    OnigScanner* createOnigScanner(const std::vector<std::string>& sources) override;
    OnigString* createOnigString(const std::string& str) override;

    // IRuleRegistry implementation
    Rule* getRule(RuleId ruleId) override;
    RuleId registerRule(Rule* rule) override;

    // IRuleFactoryHelper implementation (new methods)
    RuleId allocateRuleId() override;
    void setRule(RuleId ruleId, Rule* rule) override;

    // IGrammarRegistry implementation
    IRawGrammar* getExternalGrammar(const std::string& scopeName, IRawRepository* repository) override;

    // Get metadata for scope
    BasicScopeAttributes getMetadataForScope(const std::string& scope);

    // Get injections
    std::vector<Injection> getInjections();

    // IGrammar implementation
    ITokenizeLineResult tokenizeLine(
        const std::string& lineText,
        StateStack* prevState,
        int timeLimit = 0
    ) override;

    ITokenizeLineResult2 tokenizeLine2(
        const std::string& lineText,
        StateStack* prevState,
        int timeLimit = 0
    ) override;

    // Get the root scope name of this grammar
    ScopeName getScopeName() const { return _rootScopeName; }

private:
    std::vector<Injection> _collectInjections();

    struct TokenizeResult {
        int lineLength;
        LineTokens* lineTokens;
        StateStackImpl* ruleStack;
        bool stoppedEarly;
    };

    TokenizeResult _tokenize(
        const std::string& lineText,
        StateStackImpl* prevState,
        bool emitBinaryTokens,
        int timeLimit
    );
};

// Helper function to create grammar
Grammar* createGrammar(
    const ScopeName& scopeName,
    IRawGrammar* grammar,
    int initialLanguage,
    const EmbeddedLanguagesMap* embeddedLanguages,
    const TokenTypeMap* tokenTypes,
    BalancedBracketSelectors* balancedBracketSelectors,
    IGrammarRepository* grammarRepository,
    IThemeProvider* themeProvider,
    IOnigLib* onigLib
);

// Initialize grammar (merge with base if needed)
IRawGrammar* initGrammar(IRawGrammar* grammar, IRawRule* base);

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_GRAMMAR_H
