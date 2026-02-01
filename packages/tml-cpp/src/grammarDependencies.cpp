#include "grammarDependencies.h"
#include "registry.h"
#include <iostream>
#include <set>

namespace tml {

// Helper to collect external references from rules
static void collectExternalReferencesInRules(
    const std::vector<IRawRule*>& rules,
    IRawGrammar* baseGrammar,
    IRawGrammar* selfGrammar,
    std::vector<AbsoluteRuleReference>& result,
    std::set<IRawRule*>& visitedRules
);

// Helper to collect references in a single rule
static void collectExternalReferencesInRule(
    IRawRule* rule,
    IRawGrammar* baseGrammar,
    IRawGrammar* selfGrammar,
    std::vector<AbsoluteRuleReference>& result,
    std::set<IRawRule*>& visitedRules
) {
    if (!rule || visitedRules.find(rule) != visitedRules.end()) {
        return;
    }
    visitedRules.insert(rule);

    // Check if rule has an include first
    if (rule->include && !rule->include->empty()) {
        IncludeReference ref = parseInclude(*rule->include);

        if (ref.kind == IncludeReferenceKind::TopLevelReference) {
            // External grammar reference like "source.c"
            if (ref.scopeName != selfGrammar->scopeName && ref.scopeName != baseGrammar->scopeName) {
                result.push_back(AbsoluteRuleReference(ref.scopeName, ""));
            }
        } else if (ref.kind == IncludeReferenceKind::TopLevelRepositoryReference) {
            // External grammar repository reference like "source.c#ruleName"
            if (ref.scopeName != selfGrammar->scopeName && ref.scopeName != baseGrammar->scopeName) {
                result.push_back(AbsoluteRuleReference(ref.scopeName, ref.ruleName));
            }
        } else if (ref.kind == IncludeReferenceKind::RelativeReference) {
            // Local repository reference like "#ruleName" - need to look it up and scan it
            if (selfGrammar->repository) {
                IRawRule* referencedRule = selfGrammar->repository->getRule(ref.ruleName);
                if (referencedRule) {
                    collectExternalReferencesInRule(referencedRule, baseGrammar, selfGrammar, result, visitedRules);
                }
            }
        }
    }

    // Recursively check if rule has patterns (IMPORTANT: this finds includes in nested rules)
    if (rule->patterns && !rule->patterns->empty()) {
        collectExternalReferencesInRules(*rule->patterns, baseGrammar, selfGrammar, result, visitedRules);
    }
}

static void collectExternalReferencesInRules(
    const std::vector<IRawRule*>& rules,
    IRawGrammar* baseGrammar,
    IRawGrammar* selfGrammar,
    std::vector<AbsoluteRuleReference>& result,
    std::set<IRawRule*>& visitedRules
) {
    for (IRawRule* rule : rules) {
        collectExternalReferencesInRule(rule, baseGrammar, selfGrammar, result, visitedRules);
    }
}

IncludeReference parseInclude(const std::string& include) {
    if (include == "$base" || include == "$self") {
        return IncludeReference(
            include == "$base" ? IncludeReferenceKind::Base : IncludeReferenceKind::Self
        );
    }

    if (include[0] == '#') {
        // Relative reference: #ruleName
        return IncludeReference(
            IncludeReferenceKind::RelativeReference,
            "",
            include.substr(1)
        );
    }

    size_t sharpIndex = include.find('#');
    if (sharpIndex != std::string::npos) {
        // Top level repository reference: scopeName#ruleName
        return IncludeReference(
            IncludeReferenceKind::TopLevelRepositoryReference,
            include.substr(0, sharpIndex),
            include.substr(sharpIndex + 1)
        );
    }

    // Top level reference: scopeName
    return IncludeReference(
        IncludeReferenceKind::TopLevelReference,
        include,
        ""
    );
}

ScopeDependencyProcessor::ScopeDependencyProcessor(SyncRegistry* repo, const std::string& initialScopeName)
    : _repo(repo), _initialScopeName(initialScopeName) {
    _seenScopes.insert(initialScopeName);
    Q.push(AbsoluteRuleReference(initialScopeName, ""));
}

void ScopeDependencyProcessor::processQueue() {
    // Process all items in the queue and collect their external dependencies
    std::queue<AbsoluteRuleReference> currentQ;
    currentQ.swap(Q);


    std::vector<AbsoluteRuleReference> externalRefs;
    std::set<IRawRule*> visitedRules;

    while (!currentQ.empty()) {
        AbsoluteRuleReference ref = currentQ.front();
        currentQ.pop();

        // Look up the grammar for this reference
        IRawGrammar* grammar = _repo->lookup(ref.scopeName);
        if (!grammar) {
            continue;
        }


        // Collect external references from this grammar's patterns
        if (grammar->patterns && !grammar->patterns->empty()) {
            collectExternalReferencesInRules(*grammar->patterns, grammar, grammar, externalRefs, visitedRules);
        }

        // Also scan injections if present
        if (grammar->injections) {
            for (const auto& injection : *grammar->injections) {
                if (injection.second->patterns) {
                    collectExternalReferencesInRules(*injection.second->patterns, grammar, grammar, externalRefs, visitedRules);
                }
            }
        }
    }


    // Add new external references to the queue if not seen before
    for (const AbsoluteRuleReference& extRef : externalRefs) {
        std::string key = extRef.scopeName;
        if (!extRef.ruleName.empty()) {
            key += "#" + extRef.ruleName;
        }


        if (_seenScopes.find(key) == _seenScopes.end()) {
            _seenScopes.insert(key);
            Q.push(extRef);
        } else {
        }
    }

}

} // namespace tml
