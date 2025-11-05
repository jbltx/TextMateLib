#ifndef TEXTMATELIB_TYPES_H
#define TEXTMATELIB_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace tml {

// Type aliases matching TypeScript types
using ScopeName = std::string;
using ScopePath = std::string;
using ScopePattern = std::string;
using IncludeString = std::string;
using RegExpString = std::string;

// OrMask type for bitwise flags
template<typename T>
using OrMask = int;

// Forward declarations
class Rule;
class Grammar;
class Theme;
class ScopeStack;
class Registry;
class SyncRegistry;

// StateStack interface - define here since it's used everywhere
class StateStack {
public:
    virtual ~StateStack() {}
    virtual int getDepth() const = 0;
    virtual StateStack* clone() = 0;
    virtual bool equals(StateStack* other) = 0;
};

// RuleId type (opaque identifier for rules)
struct RuleId {
    int id;

    explicit RuleId(int value) : id(value) {}

    bool operator==(const RuleId& other) const {
        return id == other.id;
    }

    bool operator!=(const RuleId& other) const {
        return id != other.id;
    }
};

// Special rule ID constants
const RuleId END_RULE_ID(-1);
const RuleId WHILE_RULE_ID(-2);

// Helper functions for RuleId
inline RuleId ruleIdFromNumber(int id) {
    return RuleId(id);
}

inline int ruleIdToNumber(RuleId id) {
    return id.id;
}

// Encoded token attributes type
using EncodedTokenAttributes = int32_t;

// Standard token type enum
enum class StandardTokenType {
    Other = 0,
    Comment = 1,
    String = 2,
    RegEx = 3
};

// Optional standard token type enum
enum class OptionalStandardTokenType {
    Other = 0,
    Comment = 1,
    String = 2,
    RegEx = 3,
    NotSet = 8
};

// Font style enum
enum class FontStyle {
    NotSet = -1,
    None = 0,
    Italic = 1,
    Bold = 2,
    Underline = 4,
    Strikethrough = 8
};

// Embedded languages map
using EmbeddedLanguagesMap = std::map<std::string, int>;

// Token type map
using TokenTypeMap = std::map<std::string, StandardTokenType>;

} // namespace tml

#endif // TEXTMATELIB_TYPES_H
