#ifndef TEXTMATELIB_MATCHER_H
#define TEXTMATELIB_MATCHER_H

#include "types.h"
#include <string>
#include <vector>
#include <functional>

namespace tml {

// Matcher function type
template<typename T>
using Matcher = std::function<bool(const T&)>;

// MatcherWithPriority structure
template<typename T>
struct MatcherWithPriority {
    Matcher<T> matcher;
    int priority; // -1, 0, or 1

    MatcherWithPriority() : priority(0) {}
    MatcherWithPriority(const Matcher<T>& m, int p) : matcher(m), priority(p) {}
};

// Helper functions
bool isIdentifier(const std::string& token);
bool isIdentifier(const std::string* token);

// Tokenizer class
class SelectorTokenizer {
private:
    std::string _input;
    size_t _position;

public:
    explicit SelectorTokenizer(const std::string& input);
    std::string* next();
};

// Create matchers from a selector string (template implementation in header)
template<typename T>
std::vector<MatcherWithPriority<T>> createMatchers(
    const std::string& selector,
    std::function<bool(const std::vector<std::string>&, const T&)> matchesName
) {
    std::vector<MatcherWithPriority<T>> results;
    SelectorTokenizer tokenizer(selector);
    std::string* token = tokenizer.next();

    // Forward declarations of parsing functions
    std::function<Matcher<T>(std::string*&)> parseOperand;
    std::function<Matcher<T>(std::string*&)> parseConjunction;
    std::function<Matcher<T>(std::string*&)> parseInnerExpression;

    // parseOperand: Parse a single operand (identifier, negation, or parenthesized expression)
    parseOperand = [&](std::string*& tok) -> Matcher<T> {
        if (tok && *tok == "-") {
            delete tok;
            tok = tokenizer.next();
            Matcher<T> expressionToNegate = parseOperand(tok);
            return [expressionToNegate](const T& matcherInput) -> bool {
                return expressionToNegate ? !expressionToNegate(matcherInput) : true;
            };
        }
        if (tok && *tok == "(") {
            delete tok;
            tok = tokenizer.next();
            Matcher<T> expressionInParents = parseInnerExpression(tok);
            if (tok && *tok == ")") {
                delete tok;
                tok = tokenizer.next();
            }
            return expressionInParents;
        }
        if (isIdentifier(tok)) {
            std::vector<std::string> identifiers;
            while (isIdentifier(tok)) {
                identifiers.push_back(*tok);
                delete tok;
                tok = tokenizer.next();
            }
            return [identifiers, matchesName](const T& matcherInput) -> bool {
                return matchesName(identifiers, matcherInput);
            };
        }
        return Matcher<T>();  // null matcher
    };

    // parseConjunction: Parse a conjunction (AND) of operands
    parseConjunction = [&](std::string*& tok) -> Matcher<T> {
        std::vector<Matcher<T>> matchers;
        Matcher<T> matcher = parseOperand(tok);
        while (matcher) {
            matchers.push_back(matcher);
            matcher = parseOperand(tok);
        }
        return [matchers](const T& matcherInput) -> bool {
            for (const auto& m : matchers) {
                if (!m(matcherInput)) {
                    return false;
                }
            }
            return true;
        };
    };

    // parseInnerExpression: Parse disjunction (OR) of conjunctions
    parseInnerExpression = [&](std::string*& tok) -> Matcher<T> {
        std::vector<Matcher<T>> matchers;
        Matcher<T> matcher = parseConjunction(tok);
        while (matcher) {
            matchers.push_back(matcher);
            if (tok && (*tok == "|" || *tok == ",")) {
                do {
                    delete tok;
                    tok = tokenizer.next();
                } while (tok && (*tok == "|" || *tok == ","));
            } else {
                break;
            }
            matcher = parseConjunction(tok);
        }
        return [matchers](const T& matcherInput) -> bool {
            for (const auto& m : matchers) {
                if (m(matcherInput)) {
                    return true;
                }
            }
            return false;
        };
    };

    // Main parsing loop
    while (token != nullptr) {
        int priority = 0;
        if (token->length() == 2 && token->at(1) == ':') {
            switch (token->at(0)) {
                case 'R': priority = 1; break;
                case 'L': priority = -1; break;
                default:
                    // Unknown priority
                    break;
            }
            delete token;
            token = tokenizer.next();
        }
        Matcher<T> matcher = parseConjunction(token);
        results.push_back(MatcherWithPriority<T>(matcher, priority));
        if (token == nullptr || *token != ",") {
            break;
        }
        delete token;
        token = tokenizer.next();
    }

    if (token != nullptr) {
        delete token;
    }

    return results;
}

} // namespace tml

#endif // TEXTMATELIB_MATCHER_H
