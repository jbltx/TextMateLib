#include "rawGrammar.h"
#include <iostream>

namespace vscode_textmate {

// IRawCapturesMap destructor
IRawCapturesMap::~IRawCapturesMap() {
    for (auto& pair : captures) {
        delete pair.second;
    }
    captures.clear();
}

// IRawRule destructor
IRawRule::~IRawRule() {
    deleteIfNotNull(id);
    deleteIfNotNull(include);
    deleteIfNotNull(name);
    deleteIfNotNull(contentName);
    deleteIfNotNull(match);
    deleteIfNotNull(captures);
    deleteIfNotNull(begin);
    deleteIfNotNull(beginCaptures);
    deleteIfNotNull(end);
    deleteIfNotNull(endCaptures);
    deleteIfNotNull(whilePattern);
    deleteIfNotNull(whileCaptures);
    deleteIfNotNull(applyEndPatternLast);

    if (patterns != nullptr) {
        for (IRawRule* rule : *patterns) {
            delete rule;
        }
        delete patterns;
        patterns = nullptr;
    }

    deleteIfNotNull(repository);
}

// IRawRepositoryMap destructor
IRawRepositoryMap::~IRawRepositoryMap() {
    for (auto& pair : rules) {
        delete pair.second;
    }
    rules.clear();

    // IMPORTANT: Only delete selfRule. DO NOT delete baseRule!
    // When baseRule != selfRule, baseRule points to an external grammar's rule
    // that is owned by that grammar. Deleting it here would cause a double-free
    // when the external grammar is destroyed.

    deleteIfNotNull(selfRule);

    // baseRule is NOT owned by this repository, so we don't delete it
    baseRule = nullptr;  // Just nullify the pointer

}

// Helper method to get a rule by name
IRawRule* IRawRepositoryMap::getRule(const std::string& name) const {
    if (name == "$self") {
        return selfRule;
    }
    if (name == "$base") {
        return baseRule;
    }
    auto it = rules.find(name);
    if (it != rules.end()) {
        return it->second;
    }
    return nullptr;
}

// IRawGrammar destructor
IRawGrammar::~IRawGrammar() {

    deleteIfNotNull(repository);

    // patterns is now a pointer (inherited from IRawRule)
    if (patterns != nullptr) {
        for (size_t i = 0; i < patterns->size(); i++) {
            delete (*patterns)[i];
        }
        delete patterns;
        patterns = nullptr;
    }

    if (injections != nullptr) {
        for (auto& pair : *injections) {
            delete pair.second;
        }
        delete injections;
        injections = nullptr;
    }

    deleteIfNotNull(injectionSelector);
    deleteIfNotNull(fileTypes);
    deleteIfNotNull(name);
    deleteIfNotNull(firstLineMatch);
}

} // namespace vscode_textmate
