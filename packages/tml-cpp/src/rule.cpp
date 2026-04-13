#include "rule.h"
#include "grammarDependencies.h"
#include <stdexcept>
#include <iostream>

namespace tml {

// CompiledRule implementation

CompiledRule::CompiledRule() : scanner(nullptr) {}

CompiledRule::~CompiledRule() {
    dispose();
}

void CompiledRule::dispose() {
    if (scanner != nullptr) {
        scanner->dispose();
        delete scanner;
        scanner = nullptr;
    }
}

// RegExpSourceList implementation

RegExpSourceList::RegExpSourceList()
    : _hasAnchors(false), _cached(nullptr),
      _anchorCache_A0_G0(nullptr), _anchorCache_A0_G1(nullptr),
      _anchorCache_A1_G0(nullptr), _anchorCache_A1_G1(nullptr) {
}

RegExpSourceList::~RegExpSourceList() {
    dispose();
}

void RegExpSourceList::push(RegexSource* item) {
    _items.push_back(item);
    if (item->hasAnchor) {
        _hasAnchors = true;
    }
}

void RegExpSourceList::unshift(RegexSource* item) {
    _items.insert(_items.begin(), item);
    if (item->hasAnchor) {
        _hasAnchors = true;
    }
}

void RegExpSourceList::setSource(int index, const std::string& newSource) {
    if (index >= 0 && index < static_cast<int>(_items.size())) {
        if (_items[index]->source != newSource) {
            // Bust the cache when source changes
            dispose();
            _items[index]->source = newSource;
        }
    }
}

int RegExpSourceList::length() const {
    return _items.size();
}

CompiledRule* RegExpSourceList::compile(IOnigLib* onigLib) {
    if (!_cached) {
        std::vector<std::string> sources;
        for (auto* item : _items) {
            sources.push_back(item->source);
        }

        _cached = new CompiledRule();
        _cached->scanner = onigLib->createOnigScanner(sources);

        for (auto* item : _items) {
            _cached->rules.push_back(item->ruleId);
        }
    }
    return _cached;
}

CompiledRule* RegExpSourceList::compileAG(IOnigLib* onigLib, bool allowA, bool allowG) {
    // Check if we need to resolve anchors
    if (!_hasAnchors) {
        // No anchors, use the cached compile
        return compile(onigLib);
    }

    // Cache the compiled rules for different allowA/allowG combinations
    CompiledRule** cacheSlot = nullptr;
    if (allowA) {
        if (allowG) {
            cacheSlot = &_anchorCache_A1_G1;
        } else {
            cacheSlot = &_anchorCache_A1_G0;
        }
    } else {
        if (allowG) {
            cacheSlot = &_anchorCache_A0_G1;
        } else {
            cacheSlot = &_anchorCache_A0_G0;
        }
    }

    if (*cacheSlot != nullptr) {
        return *cacheSlot;
    }

    // Create and cache the compiled rule
    std::vector<std::string> sources;
    for (auto* item : _items) {
        sources.push_back(item->resolveAnchors(allowA, allowG));
    }

    CompiledRule* result = new CompiledRule();
    result->scanner = onigLib->createOnigScanner(sources);

    for (auto* item : _items) {
        result->rules.push_back(item->ruleId);
    }

    *cacheSlot = result;
    return result;
}

void RegExpSourceList::dispose() {
    if (_cached) {
        _cached->dispose();
        delete _cached;
        _cached = nullptr;
    }
    if (_anchorCache_A0_G0) {
        _anchorCache_A0_G0->dispose();
        delete _anchorCache_A0_G0;
        _anchorCache_A0_G0 = nullptr;
    }
    if (_anchorCache_A0_G1) {
        _anchorCache_A0_G1->dispose();
        delete _anchorCache_A0_G1;
        _anchorCache_A0_G1 = nullptr;
    }
    if (_anchorCache_A1_G0) {
        _anchorCache_A1_G0->dispose();
        delete _anchorCache_A1_G0;
        _anchorCache_A1_G0 = nullptr;
    }
    if (_anchorCache_A1_G1) {
        _anchorCache_A1_G1->dispose();
        delete _anchorCache_A1_G1;
        _anchorCache_A1_G1 = nullptr;
    }
}

// Rule base class implementation

Rule::Rule(ILocation* location_, RuleId id_,
          const std::string* name, const std::string* contentName)
    : location(location_), id(id_),
      _name(name ? new std::string(*name) : nullptr),
      _contentName(contentName ? new std::string(*contentName) : nullptr) {

    _nameIsCapturing = RegexSource::hasCaptures(_name);
    _contentNameIsCapturing = RegexSource::hasCaptures(_contentName);
}

Rule::~Rule() {
    delete location;
    delete _name;
    delete _contentName;
}

std::string Rule::getDebugName() const {
    std::string loc = location ? (basename(location->filename) + ":" + std::to_string(location->line)) : "unknown";
    return "Rule#" + std::to_string(ruleIdToNumber(id)) + " @ " + loc;
}

std::string* Rule::getName(const std::string* lineText,
                           const std::vector<IOnigCaptureIndex>* captureIndices) const {
    if (!_nameIsCapturing || _name == nullptr || lineText == nullptr || captureIndices == nullptr) {
        return _name ? new std::string(*_name) : nullptr;
    }
    return new std::string(RegexSource::replaceCaptures(*_name, *lineText, *captureIndices));
}

std::string* Rule::getContentName(const std::string& lineText,
                                  const std::vector<IOnigCaptureIndex>& captureIndices) const {
    if (!_contentNameIsCapturing || _contentName == nullptr) {
        return _contentName ? new std::string(*_contentName) : nullptr;
    }
    return new std::string(RegexSource::replaceCaptures(*_contentName, lineText, captureIndices));
}

// CaptureRule implementation

CaptureRule::CaptureRule(ILocation* location_, RuleId id_,
                        const std::string* name, const std::string* contentName,
                        RuleId retokenizeCapturedWithRuleId_)
    : Rule(location_, id_, name, contentName),
      retokenizeCapturedWithRuleId(retokenizeCapturedWithRuleId_) {
}

void CaptureRule::dispose() {
    // Nothing specific to dispose
}

void CaptureRule::collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) {
    throw std::runtime_error("Not supported!");
}

CompiledRule* CaptureRule::compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                  const std::string* endRegexSource) {
    throw std::runtime_error("Not supported!");
}

CompiledRule* CaptureRule::compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                    const std::string* endRegexSource,
                                    bool allowA, bool allowG) {
    throw std::runtime_error("Not supported!");
}

// MatchRule implementation

MatchRule::MatchRule(ILocation* location_, RuleId id_,
                    const std::string* name, const std::string& match,
                    const std::vector<CaptureRule*>& captures_)
    : Rule(location_, id_, name, nullptr),
      _match(match, id_),
      captures(captures_),
      _cachedCompiledPatterns(nullptr) {
}

MatchRule::~MatchRule() {
    // Note: CaptureRules are now registered with the Grammar and will be
    // deleted by the Grammar's dispose() method. We should NOT delete them here
    // to avoid double-free.
}

void MatchRule::dispose() {
    if (_cachedCompiledPatterns) {
        _cachedCompiledPatterns->dispose();
        delete _cachedCompiledPatterns;
        _cachedCompiledPatterns = nullptr;
    }
}

std::string MatchRule::getDebugMatchRegExp() const {
    return _match.source;
}

void MatchRule::collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) {
    out->push(&_match);
}

CompiledRule* MatchRule::compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                const std::string* endRegexSource) {
    return _getCachedCompiledPatterns(grammar)->compile(onigLib);
}

CompiledRule* MatchRule::compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                  const std::string* endRegexSource,
                                  bool allowA, bool allowG) {
    return _getCachedCompiledPatterns(grammar)->compileAG(onigLib, allowA, allowG);
}

RegExpSourceList* MatchRule::_getCachedCompiledPatterns(IRuleRegistry* grammar) {
    if (!_cachedCompiledPatterns) {
        _cachedCompiledPatterns = new RegExpSourceList();
        collectPatterns(grammar, _cachedCompiledPatterns);
    }
    return _cachedCompiledPatterns;
}

// IncludeOnlyRule implementation

IncludeOnlyRule::IncludeOnlyRule(ILocation* location_, RuleId id_,
                                const std::string* name, const std::string* contentName,
                                const ICompilePatternsResult& patterns_)
    : Rule(location_, id_, name, contentName),
      patterns(patterns_.patterns),
      hasMissingPatterns(patterns_.hasMissingPatterns),
      _cachedCompiledPatterns(nullptr) {
}

void IncludeOnlyRule::dispose() {
    if (_cachedCompiledPatterns) {
        _cachedCompiledPatterns->dispose();
        delete _cachedCompiledPatterns;
        _cachedCompiledPatterns = nullptr;
    }
}

void IncludeOnlyRule::collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) {
    for (const auto& pattern : patterns) {
        Rule* rule = grammar->getRule(pattern);
        rule->collectPatterns(grammar, out);
    }
}

CompiledRule* IncludeOnlyRule::compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                      const std::string* endRegexSource) {
    return _getCachedCompiledPatterns(grammar)->compile(onigLib);
}

CompiledRule* IncludeOnlyRule::compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                        const std::string* endRegexSource,
                                        bool allowA, bool allowG) {
    return _getCachedCompiledPatterns(grammar)->compileAG(onigLib, allowA, allowG);
}

RegExpSourceList* IncludeOnlyRule::_getCachedCompiledPatterns(IRuleRegistry* grammar) {
    if (!_cachedCompiledPatterns) {
        _cachedCompiledPatterns = new RegExpSourceList();
        collectPatterns(grammar, _cachedCompiledPatterns);
    }
    return _cachedCompiledPatterns;
}

// BeginEndRule implementation

BeginEndRule::BeginEndRule(ILocation* location_, RuleId id_,
                          const std::string* name, const std::string* contentName,
                          const std::string& begin, const std::vector<CaptureRule*>& beginCaptures_,
                          const std::string& end, const std::vector<CaptureRule*>& endCaptures_,
                          bool applyEndPatternLast_, const ICompilePatternsResult& patterns_)
    : Rule(location_, id_, name, contentName),
      _begin(begin, id_),
      _end(end.empty() ? "\uFFFF" : end, END_RULE_ID),
      beginCaptures(beginCaptures_),
      endHasBackReferences(_end.hasBackReferences),
      endCaptures(endCaptures_),
      applyEndPatternLast(applyEndPatternLast_),
      patterns(patterns_.patterns),
      hasMissingPatterns(patterns_.hasMissingPatterns),
      _cachedCompiledPatterns(nullptr) {
}

BeginEndRule::~BeginEndRule() {
    // Note: CaptureRules are now registered with the Grammar and will be
    // deleted by the Grammar's dispose() method. We should NOT delete them here
    // to avoid double-free.
    // The beginCaptures and endCaptures vectors just hold pointers, not ownership.
}

void BeginEndRule::dispose() {
    if (_cachedCompiledPatterns) {
        _cachedCompiledPatterns->dispose();
        delete _cachedCompiledPatterns;
        _cachedCompiledPatterns = nullptr;
    }
}

std::string BeginEndRule::getDebugBeginRegExp() const {
    return _begin.source;
}

std::string BeginEndRule::getDebugEndRegExp() const {
    return _end.source;
}

std::string BeginEndRule::getEndWithResolvedBackReferences(const std::string& lineText,
                                                           const std::vector<IOnigCaptureIndex>& captureIndices) {
    return _end.resolveBackReferences(lineText, captureIndices);
}

void BeginEndRule::collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) {
    out->push(&_begin);
}

CompiledRule* BeginEndRule::compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                   const std::string* endRegexSource) {
    std::string endSource = endRegexSource ? *endRegexSource : _end.source;
    return _getCachedCompiledPatterns(grammar, endSource)->compile(onigLib);
}

CompiledRule* BeginEndRule::compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                     const std::string* endRegexSource,
                                     bool allowA, bool allowG) {
    std::string endSource = endRegexSource ? *endRegexSource : _end.source;
    return _getCachedCompiledPatterns(grammar, endSource)->compileAG(onigLib, allowA, allowG);
}

RegExpSourceList* BeginEndRule::_getCachedCompiledPatterns(IRuleRegistry* grammar,
                                                           const std::string& endRegexSource) {
    if (!_cachedCompiledPatterns) {
        _cachedCompiledPatterns = new RegExpSourceList();

        for (const auto& pattern : patterns) {
            Rule* rule = grammar->getRule(pattern);
            rule->collectPatterns(grammar, _cachedCompiledPatterns);
        }

        // Clone the end pattern if it has back-references to avoid sharing
        // the same RegexSource across multiple concurrent rule instances
        RegexSource* endPattern = _end.hasBackReferences ? _end.clone() : new RegexSource(_end.source, _end.ruleId);
        if (applyEndPatternLast) {
            _cachedCompiledPatterns->push(endPattern);
        } else {
            _cachedCompiledPatterns->unshift(endPattern);
        }
    }

    if (_end.hasBackReferences) {
        int index = applyEndPatternLast ? (_cachedCompiledPatterns->length() - 1) : 0;
        _cachedCompiledPatterns->setSource(index, endRegexSource);
    }

    return _cachedCompiledPatterns;
}

// BeginWhileRule implementation

BeginWhileRule::BeginWhileRule(ILocation* location_, RuleId id_,
                              const std::string* name, const std::string* contentName,
                              const std::string& begin, const std::vector<CaptureRule*>& beginCaptures_,
                              const std::string& whilePattern, const std::vector<CaptureRule*>& whileCaptures_,
                              const ICompilePatternsResult& patterns_)
    : Rule(location_, id_, name, contentName),
      _begin(begin, id_),
      _while(whilePattern, WHILE_RULE_ID),
      beginCaptures(beginCaptures_),
      whileCaptures(whileCaptures_),
      whileHasBackReferences(_while.hasBackReferences),
      patterns(patterns_.patterns),
      hasMissingPatterns(patterns_.hasMissingPatterns),
      _cachedCompiledPatterns(nullptr),
      _cachedCompiledWhilePatterns(nullptr) {
}

BeginWhileRule::~BeginWhileRule() {
    // Note: CaptureRules are now registered with the Grammar and will be
    // deleted by the Grammar's dispose() method. We should NOT delete them here
    // to avoid double-free.
}

void BeginWhileRule::dispose() {
    if (_cachedCompiledPatterns) {
        _cachedCompiledPatterns->dispose();
        delete _cachedCompiledPatterns;
        _cachedCompiledPatterns = nullptr;
    }
    if (_cachedCompiledWhilePatterns) {
        _cachedCompiledWhilePatterns->dispose();
        delete _cachedCompiledWhilePatterns;
        _cachedCompiledWhilePatterns = nullptr;
    }
}

std::string BeginWhileRule::getDebugBeginRegExp() const {
    return _begin.source;
}

std::string BeginWhileRule::getDebugWhileRegExp() const {
    return _while.source;
}

std::string BeginWhileRule::getWhileWithResolvedBackReferences(const std::string& lineText,
                                                               const std::vector<IOnigCaptureIndex>& captureIndices) {
    return _while.resolveBackReferences(lineText, captureIndices);
}

void BeginWhileRule::collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) {
    out->push(&_begin);
}

CompiledRule* BeginWhileRule::compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                     const std::string* endRegexSource) {
    return _getCachedCompiledPatterns(grammar)->compile(onigLib);
}

CompiledRule* BeginWhileRule::compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                       const std::string* endRegexSource,
                                       bool allowA, bool allowG) {
    return _getCachedCompiledPatterns(grammar)->compileAG(onigLib, allowA, allowG);
}

CompiledRule* BeginWhileRule::compileWhile(IOnigLib* onigLib, const std::string* endRegexSource) {
    std::string whileSource = endRegexSource ? *endRegexSource : _while.source;
    return _getCachedCompiledWhilePatterns(onigLib, whileSource)->compile(onigLib);
}

CompiledRule* BeginWhileRule::compileWhileAG(IOnigLib* onigLib, const std::string* endRegexSource,
                                            bool allowA, bool allowG) {
    std::string whileSource = endRegexSource ? *endRegexSource : _while.source;
    return _getCachedCompiledWhilePatterns(onigLib, whileSource)->compileAG(onigLib, allowA, allowG);
}

RegExpSourceList* BeginWhileRule::_getCachedCompiledPatterns(IRuleRegistry* grammar) {
    if (!_cachedCompiledPatterns) {
        _cachedCompiledPatterns = new RegExpSourceList();

        for (const auto& pattern : patterns) {
            Rule* rule = grammar->getRule(pattern);
            rule->collectPatterns(grammar, _cachedCompiledPatterns);
        }
    }
    return _cachedCompiledPatterns;
}

RegExpSourceList* BeginWhileRule::_getCachedCompiledWhilePatterns(IOnigLib* onigLib,
                                                                  const std::string& whileRegexSource) {
    if (!_cachedCompiledWhilePatterns) {
        _cachedCompiledWhilePatterns = new RegExpSourceList();
        // Clone the while pattern if it has back-references to avoid sharing
        // the same RegexSource across multiple concurrent rule instances
        RegexSource* whilePattern = _while.hasBackReferences ? _while.clone() : new RegexSource(_while.source, _while.ruleId);
        _cachedCompiledWhilePatterns->push(whilePattern);
    }

    if (_while.hasBackReferences) {
        _cachedCompiledWhilePatterns->setSource(0, whileRegexSource);
    }

    return _cachedCompiledWhilePatterns;
}

// RuleFactory implementation

Rule* RuleFactory::createCaptureRule(IRuleFactoryHelper* helper, ILocation* location,
                                     const std::string* name, const std::string* contentName,
                                     RuleId retokenizeCapturedWithRuleId) {
    CaptureRule* rule = new CaptureRule(location, ruleIdFromNumber(-1), name, contentName, retokenizeCapturedWithRuleId);
    RuleId registeredId = helper->registerRule(rule);
    rule->id = registeredId;  // Update the rule's ID with the registered ID
    return rule;
}

RuleId RuleFactory::getCompiledRuleId(IRawRule* desc, IRuleFactoryHelper* helper, IRawRepository* repository) {
    if (!desc) {
        return ruleIdFromNumber(-1);
    }

    if (desc->id != nullptr) {
        return *desc->id;
    }

    // ✅ PHASE 2 FIX: Allocate ID first, build rule, then store it
    RuleId ruleId = helper->allocateRuleId();
    desc->id = new RuleId(ruleId);

    Rule* rule = nullptr;

    if (desc->match) {
        rule = new MatchRule(
            desc->tmlLocation,
            ruleId,
            desc->name,
            *desc->match,
            _compileCaptures(desc->captures, helper, repository)
        );
    } else if (desc->begin == nullptr) {
        // Note: In TypeScript, repositories are merged here using mergeObjects.
        // In C++, repository resolution happens during pattern compilation via
        // the passed repository parameter, so explicit merging isn't needed.
        std::vector<IRawRule*>* patterns = desc->patterns;
        if (!patterns && desc->include) {
            patterns = new std::vector<IRawRule*>();
            IRawRule* includeRule = new IRawRule();
            includeRule->include = new std::string(*desc->include);
            patterns->push_back(includeRule);
        }

        rule = new IncludeOnlyRule(
            desc->tmlLocation,
            ruleId,
            desc->name,
            desc->contentName,
            _compilePatterns(patterns, helper, repository)
        );
    } else if (desc->whilePattern) {
        rule = new BeginWhileRule(
            desc->tmlLocation,
            ruleId,
            desc->name,
            desc->contentName,
            *desc->begin,
            _compileCaptures(desc->beginCaptures ? desc->beginCaptures : desc->captures, helper, repository),
            *desc->whilePattern,
            _compileCaptures(desc->whileCaptures ? desc->whileCaptures : desc->captures, helper, repository),
            _compilePatterns(desc->patterns, helper, repository)
        );
    } else {
        rule = new BeginEndRule(
            desc->tmlLocation,
            ruleId,
            desc->name,
            desc->contentName,
            *desc->begin,
            _compileCaptures(desc->beginCaptures ? desc->beginCaptures : desc->captures, helper, repository),
            desc->end ? *desc->end : "",
            _compileCaptures(desc->endCaptures ? desc->endCaptures : desc->captures, helper, repository),
            desc->applyEndPatternLast ? *desc->applyEndPatternLast : false,
            _compilePatterns(desc->patterns, helper, repository)
        );
    }

    // ✅ PHASE 2 FIX: Actually store the rule in the registry!
    if (rule) {
        helper->setRule(ruleId, rule);
    }

    return ruleId;
}

std::vector<CaptureRule*> RuleFactory::_compileCaptures(IRawCaptures* captures,
                                                        IRuleFactoryHelper* helper,
                                                        IRawRepository* repository) {
    std::vector<CaptureRule*> result;

    if (!captures) {
        return result;
    }

    // Find maximum capture id
    int maximumCaptureId = 0;
    for (const auto& pair : captures->captures) {
        int numericCaptureId = std::stoi(pair.first);
        if (numericCaptureId > maximumCaptureId) {
            maximumCaptureId = numericCaptureId;
        }
    }

    // Initialize result
    result.resize(maximumCaptureId + 1, nullptr);

    // Fill out result
    for (const auto& pair : captures->captures) {
        int numericCaptureId = std::stoi(pair.first);
        RuleId retokenizeCapturedWithRuleId = ruleIdFromNumber(0);

        if (pair.second->patterns) {
            retokenizeCapturedWithRuleId = getCompiledRuleId(pair.second, helper, repository);
        }

        // Use createCaptureRule to properly register the capture rule
        Rule* captureRule = createCaptureRule(
            helper,
            pair.second->tmlLocation,
            pair.second->name,
            pair.second->contentName,
            retokenizeCapturedWithRuleId
        );
        result[numericCaptureId] = dynamic_cast<CaptureRule*>(captureRule);
    }

    return result;
}

ICompilePatternsResult RuleFactory::_compilePatterns(std::vector<IRawRule*>* patterns,
                                                     IRuleFactoryHelper* helper,
                                                     IRawRepository* repository) {
    ICompilePatternsResult result;

    if (!patterns) {
        return result;
    }

    for (auto* pattern : *patterns) {
        RuleId ruleId = ruleIdFromNumber(-1);

        if (pattern->include) {
            IncludeReference reference = parseInclude(*pattern->include);

            switch (reference.kind) {
                case IncludeReferenceKind::Base:
                case IncludeReferenceKind::Self: {
                    // Look up in repository using the include string
                    IRawRule* repoRule = repository->getRule(*pattern->include);
                    if (repoRule) {
                        ruleId = getCompiledRuleId(repoRule, helper, repository);
                    }
                    break;
                }

                case IncludeReferenceKind::RelativeReference: {
                    // Local include found in `repository` (e.g., #ruleName)
                    IRawRule* localIncludedRule = repository->getRule(reference.ruleName);
                    if (localIncludedRule) {
                        ruleId = getCompiledRuleId(localIncludedRule, helper, repository);
                    }
                    break;
                }

                case IncludeReferenceKind::TopLevelReference:
                case IncludeReferenceKind::TopLevelRepositoryReference: {
                    const std::string& externalGrammarName = reference.scopeName;
                    const std::string* externalGrammarInclude =
                        (reference.kind == IncludeReferenceKind::TopLevelRepositoryReference)
                            ? &reference.ruleName
                            : nullptr;

                    // External include - get the external grammar
                    IRawGrammar* externalGrammar = helper->getExternalGrammar(externalGrammarName, repository);

                    if (externalGrammar) {
                        if (externalGrammarInclude) {
                            IRawRule* externalIncludedRule = externalGrammar->repository->getRule(*externalGrammarInclude);
                            if (externalIncludedRule) {
                                ruleId = getCompiledRuleId(externalIncludedRule, helper, externalGrammar->repository);
                            }
                        } else {
                            // Reference to top-level rule
                            IRawRule* selfRule = externalGrammar->repository->getRule("$self");
                            if (selfRule) {
                                ruleId = getCompiledRuleId(selfRule, helper, externalGrammar->repository);
                            }
                        }
                    }
                    break;
                }
            }
        } else {
            ruleId = getCompiledRuleId(pattern, helper, repository);
        }

        if (ruleIdToNumber(ruleId) != -1) {
            Rule* rule = helper->getRule(ruleId);

            bool skipRule = false;

            // Check if rule should be skipped (has missing patterns and no actual patterns)
            IncludeOnlyRule* includeOnlyRule = dynamic_cast<IncludeOnlyRule*>(rule);
            BeginEndRule* beginEndRule = dynamic_cast<BeginEndRule*>(rule);
            BeginWhileRule* beginWhileRule = dynamic_cast<BeginWhileRule*>(rule);

            if (includeOnlyRule) {
                if (includeOnlyRule->hasMissingPatterns && includeOnlyRule->patterns.empty()) {
                    skipRule = true;
                }
            } else if (beginEndRule) {
                if (beginEndRule->hasMissingPatterns && beginEndRule->patterns.empty()) {
                    skipRule = true;
                }
            } else if (beginWhileRule) {
                if (beginWhileRule->hasMissingPatterns && beginWhileRule->patterns.empty()) {
                    skipRule = true;
                }
            }

            if (!skipRule) {
                result.patterns.push_back(ruleId);
            }
        }
    }

    result.hasMissingPatterns = (patterns->size() != result.patterns.size());
    return result;
}

} // namespace tml
