#ifndef VSCODE_TEXTMATE_GRAMMAR_DEPENDENCIES_H
#define VSCODE_TEXTMATE_GRAMMAR_DEPENDENCIES_H

#include "types.h"
#include <string>
#include <vector>
#include <queue>
#include <set>

namespace vscode_textmate {

// Forward declarations
class SyncRegistry;

// Include reference kinds
enum class IncludeReferenceKind {
    Base,
    Self,
    RelativeReference,
    TopLevelReference,
    TopLevelRepositoryReference
};

// IncludeReference structure
struct IncludeReference {
    IncludeReferenceKind kind;
    std::string scopeName;
    std::string ruleName;

    IncludeReference() : kind(IncludeReferenceKind::Base) {}
    IncludeReference(IncludeReferenceKind k, const std::string& scope = "", const std::string& rule = "")
        : kind(k), scopeName(scope), ruleName(rule) {}
};

// Parse include string
IncludeReference parseInclude(const std::string& include);

// AbsoluteRuleReference structure
struct AbsoluteRuleReference {
    std::string scopeName;
    std::string ruleName;

    AbsoluteRuleReference() {}
    AbsoluteRuleReference(const std::string& scope, const std::string& rule)
        : scopeName(scope), ruleName(rule) {}
};

// ScopeDependencyProcessor class
class ScopeDependencyProcessor {
public:
    std::queue<AbsoluteRuleReference> Q;

private:
    SyncRegistry* _repo;
    std::string _initialScopeName;
    std::set<std::string> _seenScopes;

public:
    ScopeDependencyProcessor(SyncRegistry* repo, const std::string& initialScopeName);

    void processQueue();
};

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_GRAMMAR_DEPENDENCIES_H
