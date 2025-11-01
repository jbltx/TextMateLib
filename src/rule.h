#ifndef VSCODE_TEXTMATE_RULE_H
#define VSCODE_TEXTMATE_RULE_H

#include "types.h"
#include "utils.h"
#include "onigLib.h"
#include "rawGrammar.h"
#include <string>
#include <vector>
#include <memory>

namespace vscode_textmate {

// Forward declarations
class Rule;
class RegExpSourceList;
class CompiledRule;

// IRuleRegistry interface
class IRuleRegistry {
public:
    virtual ~IRuleRegistry() {}
    virtual Rule* getRule(RuleId ruleId) = 0;
    virtual RuleId registerRule(Rule* rule) = 0;
};

// IGrammarRegistry interface
class IGrammarRegistry {
public:
    virtual ~IGrammarRegistry() {}
    virtual IRawGrammar* getExternalGrammar(const std::string& scopeName, IRawRepository* repository) = 0;
};

// IRuleFactoryHelper interface
class IRuleFactoryHelper : public IRuleRegistry, public IGrammarRegistry {
public:
    virtual ~IRuleFactoryHelper() {}

    // New methods for proper rule registration
    virtual RuleId allocateRuleId() = 0;
    virtual void setRule(RuleId ruleId, Rule* rule) = 0;
};

// ICompilePatternsResult structure
struct ICompilePatternsResult {
    std::vector<RuleId> patterns;
    bool hasMissingPatterns;

    ICompilePatternsResult() : hasMissingPatterns(false) {}
};

// CompiledRule class
class CompiledRule {
public:
    OnigScanner* scanner;
    std::vector<RuleId> rules;

    CompiledRule();
    ~CompiledRule();

    void dispose();
};

// RegExpSourceList class
class RegExpSourceList {
private:
    std::vector<RegexSource*> _items;
    bool _hasAnchors;
    CompiledRule* _cached;
    CompiledRule* _anchorCache_A0_G0;
    CompiledRule* _anchorCache_A0_G1;
    CompiledRule* _anchorCache_A1_G0;
    CompiledRule* _anchorCache_A1_G1;

public:
    RegExpSourceList();
    ~RegExpSourceList();

    void push(RegexSource* item);
    void unshift(RegexSource* item);
    void setSource(int index, const std::string& newSource);
    int length() const;

    CompiledRule* compile(IOnigLib* onigLib);
    CompiledRule* compileAG(IOnigLib* onigLib, bool allowA, bool allowG);

    void dispose();
};

// Abstract Rule base class
class Rule {
public:
    ILocation* location;
    RuleId id;

protected:
    bool _nameIsCapturing;
    std::string* _name;
    bool _contentNameIsCapturing;
    std::string* _contentName;

public:
    Rule(ILocation* location_, RuleId id_,
         const std::string* name, const std::string* contentName);
    virtual ~Rule();

    virtual void dispose() = 0;

    std::string getDebugName() const;
    std::string* getName(const std::string* lineText,
                         const std::vector<IOnigCaptureIndex>* captureIndices) const;
    std::string* getContentName(const std::string& lineText,
                               const std::vector<IOnigCaptureIndex>& captureIndices) const;

    virtual void collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) = 0;
    virtual CompiledRule* compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                  const std::string* endRegexSource) = 0;
    virtual CompiledRule* compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                   const std::string* endRegexSource,
                                   bool allowA, bool allowG) = 0;
};

// CaptureRule class
class CaptureRule : public Rule {
public:
    RuleId retokenizeCapturedWithRuleId;

    CaptureRule(ILocation* location_, RuleId id_,
               const std::string* name, const std::string* contentName,
               RuleId retokenizeCapturedWithRuleId_);

    void dispose() override;

    void collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) override;
    CompiledRule* compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                         const std::string* endRegexSource) override;
    CompiledRule* compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                           const std::string* endRegexSource,
                           bool allowA, bool allowG) override;
};

// MatchRule class
class MatchRule : public Rule {
private:
    RegexSource _match;
    RegExpSourceList* _cachedCompiledPatterns;

public:
    std::vector<CaptureRule*> captures;

    MatchRule(ILocation* location_, RuleId id_,
             const std::string* name, const std::string& match,
             const std::vector<CaptureRule*>& captures_);
    ~MatchRule();

    void dispose() override;

    std::string getDebugMatchRegExp() const;

    void collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) override;
    CompiledRule* compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                         const std::string* endRegexSource) override;
    CompiledRule* compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                           const std::string* endRegexSource,
                           bool allowA, bool allowG) override;

private:
    RegExpSourceList* _getCachedCompiledPatterns(IRuleRegistry* grammar);
};

// IncludeOnlyRule class
class IncludeOnlyRule : public Rule {
private:
    RegExpSourceList* _cachedCompiledPatterns;

public:
    bool hasMissingPatterns;
    std::vector<RuleId> patterns;

    IncludeOnlyRule(ILocation* location_, RuleId id_,
                   const std::string* name, const std::string* contentName,
                   const ICompilePatternsResult& patterns_);

    void dispose() override;

    void collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) override;
    CompiledRule* compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                         const std::string* endRegexSource) override;
    CompiledRule* compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                           const std::string* endRegexSource,
                           bool allowA, bool allowG) override;

private:
    RegExpSourceList* _getCachedCompiledPatterns(IRuleRegistry* grammar);
};

// BeginEndRule class
class BeginEndRule : public Rule {
private:
    RegexSource _begin;
    RegexSource _end;
    RegExpSourceList* _cachedCompiledPatterns;

public:
    std::vector<CaptureRule*> beginCaptures;
    bool endHasBackReferences;
    std::vector<CaptureRule*> endCaptures;
    bool applyEndPatternLast;
    bool hasMissingPatterns;
    std::vector<RuleId> patterns;

    BeginEndRule(ILocation* location_, RuleId id_,
                const std::string* name, const std::string* contentName,
                const std::string& begin, const std::vector<CaptureRule*>& beginCaptures_,
                const std::string& end, const std::vector<CaptureRule*>& endCaptures_,
                bool applyEndPatternLast_, const ICompilePatternsResult& patterns_);
    ~BeginEndRule();

    void dispose() override;

    std::string getDebugBeginRegExp() const;
    std::string getDebugEndRegExp() const;
    std::string getEndWithResolvedBackReferences(const std::string& lineText,
                                                  const std::vector<IOnigCaptureIndex>& captureIndices);

    void collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) override;
    CompiledRule* compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                         const std::string* endRegexSource) override;
    CompiledRule* compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                           const std::string* endRegexSource,
                           bool allowA, bool allowG) override;

private:
    RegExpSourceList* _getCachedCompiledPatterns(IRuleRegistry* grammar, const std::string& endRegexSource);
};

// BeginWhileRule class
class BeginWhileRule : public Rule {
private:
    RegexSource _begin;
    RegexSource _while;
    RegExpSourceList* _cachedCompiledPatterns;
    RegExpSourceList* _cachedCompiledWhilePatterns;

public:
    std::vector<CaptureRule*> beginCaptures;
    std::vector<CaptureRule*> whileCaptures;
    bool whileHasBackReferences;
    bool hasMissingPatterns;
    std::vector<RuleId> patterns;

    BeginWhileRule(ILocation* location_, RuleId id_,
                  const std::string* name, const std::string* contentName,
                  const std::string& begin, const std::vector<CaptureRule*>& beginCaptures_,
                  const std::string& whilePattern, const std::vector<CaptureRule*>& whileCaptures_,
                  const ICompilePatternsResult& patterns_);
    ~BeginWhileRule();

    void dispose() override;

    std::string getDebugBeginRegExp() const;
    std::string getDebugWhileRegExp() const;
    std::string getWhileWithResolvedBackReferences(const std::string& lineText,
                                                    const std::vector<IOnigCaptureIndex>& captureIndices);

    void collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) override;
    CompiledRule* compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                         const std::string* endRegexSource) override;
    CompiledRule* compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                           const std::string* endRegexSource,
                           bool allowA, bool allowG) override;

    CompiledRule* compileWhile(IOnigLib* onigLib, const std::string* endRegexSource);
    CompiledRule* compileWhileAG(IOnigLib* onigLib, const std::string* endRegexSource,
                                 bool allowA, bool allowG);

private:
    RegExpSourceList* _getCachedCompiledPatterns(IRuleRegistry* grammar);
    RegExpSourceList* _getCachedCompiledWhilePatterns(IOnigLib* onigLib, const std::string& whileRegexSource);
};

// RuleFactory class
class RuleFactory {
public:
    static RuleId getCompiledRuleId(IRawRule* desc, IRuleFactoryHelper* helper, IRawRepository* repository);
    static Rule* createCaptureRule(IRuleFactoryHelper* helper, ILocation* location,
                                   const std::string* name, const std::string* contentName,
                                   RuleId retokenizeCapturedWithRuleId);

private:
    static std::vector<CaptureRule*> _compileCaptures(IRawCaptures* captures,
                                                       IRuleFactoryHelper* helper,
                                                       IRawRepository* repository);
    static ICompilePatternsResult _compilePatterns(std::vector<IRawRule*>* patterns,
                                                   IRuleFactoryHelper* helper,
                                                   IRawRepository* repository);
};

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_RULE_H
