/// @file types.h
/// @brief Core type definitions and interfaces for TextMateLib

#ifndef TEXTMATELIB_TYPES_H
#define TEXTMATELIB_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace tml {

/// @defgroup core_types Core Types and Identifiers
/// @{

/// @brief Semantic name identifying a scope (e.g., "source.javascript", "comment.line")
/// @see ScopePath
using ScopeName = std::string;

/// @brief Full hierarchical scope path (e.g., "source.js keyword.control string.quoted.double")
/// @details Space-separated list of scopes representing the nesting hierarchy at a point in the text.
/// @see ScopeName
using ScopePath = std::string;

/// @brief Regular expression pattern string used in grammar rules
/// @see Rule
using ScopePattern = std::string;

/// @brief String referencing another grammar to include (scope name or file path)
/// @see Rule
using IncludeString = std::string;

/// @brief Regular expression pattern as a string
/// @see Rule
using RegExpString = std::string;

/// @brief Generic template for bitwise OR-able flags/masks
/// @tparam T The flag enum type (not actually used in current implementation)
template<typename T>
using OrMask = int;

/// @}

/// @defgroup state_management State and Parsing Management
/// @{

/// @brief Abstract interface representing the parsing state at the end of a line
/// @details StateStack is immutable and encodes the hierarchy of active grammar rules
///          after parsing a line. Two StateStacks are equal() if and only if parsing can
///          resume from the exact same position (all scopes and nesting are identical).
///          This enables incremental tokenization optimization: if prevState equals previousLine's ruleStack,
///          the line's tokens may not have changed (early stopping).
class StateStack {
public:
    virtual ~StateStack() {}

    /// @brief Get the nesting depth (number of active rules)
    /// @return Stack depth (0 for initial state)
    virtual int getDepth() const = 0;

    /// @brief Create an independent copy of this state
    /// @return Dynamically allocated copy (caller must delete)
    virtual StateStack* clone() = 0;

    /// @brief Check if this state equals another (for incremental tokenization)
    /// @param other StateStack to compare with
    /// @return True if states represent identical parsing positions
    /// @note Critical for incremental tokenization performance
    virtual bool equals(StateStack* other) = 0;
};

/// @}

/// @defgroup rule_identification Rule Identification
/// @{

/// @brief Opaque identifier for a grammar rule
/// @details RuleIds are small integers assigned during grammar compilation.
///          They uniquely identify rules within a grammar for state tracking.
struct RuleId {
    int id;  ///< Internal rule identifier (positive for normal rules, negative for special rules)

    /// @brief Create a RuleId with the given value
    /// @param value Rule identifier value
    explicit RuleId(int value) : id(value) {}

    /// @brief Equality comparison
    bool operator==(const RuleId& other) const {
        return id == other.id;
    }

    /// @brief Inequality comparison
    bool operator!=(const RuleId& other) const {
        return id != other.id;
    }
};

/// @brief Special rule ID indicating the end of a matched region
const RuleId END_RULE_ID(-1);

/// @brief Special rule ID for 'while' rule matching
const RuleId WHILE_RULE_ID(-2);

/// @brief Convert an integer to a RuleId
/// @param id Integer value to convert
/// @return RuleId wrapping the value
inline RuleId ruleIdFromNumber(int id) {
    return RuleId(id);
}

/// @brief Convert a RuleId to its integer value
/// @param id RuleId to extract
/// @return Integer identifier value
inline int ruleIdToNumber(RuleId id) {
    return id.id;
}

/// @}

/// @defgroup token_encoding Token Encoding and Attributes
/// @{

/// @brief Compact 32-bit encoding of a token's attributes
/// @details Encodes start/end position, scope depth, and attributes in a single int32_t
///          for space-efficient token representation (alternative to detailed token structs).
using EncodedTokenAttributes = int32_t;

/// @}

/// @defgroup token_classification Token Type Classification
/// @{

/// @brief Standard TextMate token type for syntax classification
enum class StandardTokenType {
    Other = 0,  ///< Not a recognized standard type
    Comment = 1,  ///< Comment text
    String = 2,  ///< String literal
    RegEx = 3  ///< Regular expression literal
};

/// @brief Standard token type with optional (unknown) state
/// @details Like StandardTokenType but includes a NotSet value for unclassified regions
enum class OptionalStandardTokenType {
    Other = 0,  ///< Not a recognized standard type
    Comment = 1,  ///< Comment text
    String = 2,  ///< String literal
    RegEx = 3,  ///< Regular expression literal
    NotSet = 8  ///< Type not determined or not applicable
};

/// @}

/// @defgroup styling Font and Display Styling
/// @{

/// @brief Font styling attributes (italic, bold, underline, strikethrough)
enum class FontStyle {
    NotSet = -1,  ///< Styling not specified (inherit or use default)
    None = 0,  ///< No special styling
    Italic = 1,  ///< Italic text
    Bold = 2,  ///< Bold (heavy weight) text
    Underline = 4,  ///< Underlined text
    Strikethrough = 8  ///< Struck-through text
};

/// @}

/// @defgroup mapping Maps and Collections
/// @{

/// @brief Map from embedded language name to token type ID
/// @details Used to classify tokens as specific language constructs
using EmbeddedLanguagesMap = std::map<std::string, int>;

/// @brief Map from scope pattern to standard token type
/// @details Associates scope names with their semantic token classifications
using TokenTypeMap = std::map<std::string, StandardTokenType>;

/// @}

} // namespace tml

#endif // TEXTMATELIB_TYPES_H
