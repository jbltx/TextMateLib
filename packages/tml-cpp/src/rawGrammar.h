#ifndef TEXTMATELIB_RAW_GRAMMAR_H
#define TEXTMATELIB_RAW_GRAMMAR_H

#include "types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace tml {

// ILocation interface
struct ILocation {
    std::string filename;
    int line;
    int charPos;

    ILocation() : line(0), charPos(0) {}
    ILocation(const std::string& fname, int l, int c)
        : filename(fname), line(l), charPos(c) {}
};

// ILocatable interface
struct ILocatable {
    ILocation* tmlLocation;

    ILocatable() : tmlLocation(nullptr) {}
    ~ILocatable() {
        delete tmlLocation;
    }
};

// Forward declarations
struct IRawRule;
struct IRawRepository;
struct IRawCaptures;

// IRawCaptures interface
struct IRawCapturesMap {
    std::map<std::string, IRawRule*> captures;

    IRawCapturesMap() {}
    ~IRawCapturesMap();
};

struct IRawCaptures : public IRawCapturesMap, public ILocatable {
    IRawCaptures() {}
};

// IRawRule interface
struct IRawRule : public ILocatable {
    RuleId* id;  // Not part of spec, used internally

    IncludeString* include;

    ScopeName* name;
    ScopeName* contentName;

    RegExpString* match;
    IRawCaptures* captures;

    RegExpString* begin;
    IRawCaptures* beginCaptures;

    RegExpString* end;
    IRawCaptures* endCaptures;

    RegExpString* whilePattern;  // 'while' is a C++ keyword, so using whilePattern
    IRawCaptures* whileCaptures;

    std::vector<IRawRule*>* patterns;

    IRawRepository* repository;

    bool* applyEndPatternLast;

    IRawRule()
        : id(nullptr), include(nullptr), name(nullptr), contentName(nullptr),
          match(nullptr), captures(nullptr), begin(nullptr), beginCaptures(nullptr),
          end(nullptr), endCaptures(nullptr), whilePattern(nullptr), whileCaptures(nullptr),
          patterns(nullptr), repository(nullptr), applyEndPatternLast(nullptr) {}

    ~IRawRule();
};

// IRawRepository interface
struct IRawRepositoryMap {
    std::map<std::string, IRawRule*> rules;
    IRawRule* selfRule;   // $self
    IRawRule* baseRule;   // $base

    IRawRepositoryMap() : selfRule(nullptr), baseRule(nullptr) {}
    ~IRawRepositoryMap();

    // Helper method to get a rule by name (handles $self, $base, and regular rules)
    IRawRule* getRule(const std::string& name) const;
};

struct IRawRepository : public IRawRepositoryMap, public ILocatable {
    IRawRepository() {}
};

// IRawGrammar interface
struct IRawGrammar : public IRawRule {
    ScopeName scopeName;

    std::map<std::string, IRawRule*>* injections;
    std::string* injectionSelector;

    std::vector<std::string>* fileTypes;
    // name and repository are inherited from IRawRule
    std::string* firstLineMatch;

    IRawGrammar()
        : IRawRule(), injections(nullptr), injectionSelector(nullptr),
          fileTypes(nullptr), firstLineMatch(nullptr) {
    }

    ~IRawGrammar();
};

// Helper function to delete pointer if not null
template<typename T>
void deleteIfNotNull(T*& ptr) {
    if (ptr != nullptr) {
        delete ptr;
        ptr = nullptr;
    }
}

} // namespace tml

#endif // TEXTMATELIB_RAW_GRAMMAR_H
