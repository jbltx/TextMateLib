#ifndef TEXTMATELIB_TOKENIZE_STRING_H
#define TEXTMATELIB_TOKENIZE_STRING_H

#include "types.h"
#include "onigLib.h"
#include "rule.h"
#include <vector>

namespace tml {

// Forward declarations
class Grammar;
class StateStackImpl;
class LineTokens;
class AttributedScopeStack;
class BeginEndRule;
class BeginWhileRule;
class MatchRule;
struct Injection;

// StackElement result structure
struct StackElement {
    StateStackImpl* stack;
    int linePos;
    int anchorPosition;
    bool stoppedEarly;

    StackElement()
        : stack(nullptr), linePos(0), anchorPosition(0), stoppedEarly(false) {}
};

// Match result structure
struct IMatchResult {
    std::vector<IOnigCaptureIndex> captureIndices;
    RuleId matchedRuleId;

    IMatchResult() : matchedRuleId(ruleIdFromNumber(0)) {}
};

// Match injections result structure
struct IMatchInjectionsResult {
    bool priorityMatch;
    std::vector<IOnigCaptureIndex> captureIndices;
    RuleId matchedRuleId;

    IMatchInjectionsResult() : priorityMatch(false), matchedRuleId(ruleIdFromNumber(0)) {}
};

// While check result structure
struct IWhileCheckResult {
    StateStackImpl* stack;
    int linePos;
    int anchorPosition;
    bool isFirstLine;

    IWhileCheckResult()
        : stack(nullptr), linePos(0), anchorPosition(-1), isFirstLine(false) {}
};

// LocalStackElement for capture handling
class LocalStackElement {
public:
    AttributedScopeStack* scopes;
    int endPos;

    LocalStackElement(AttributedScopeStack* scopes_, int endPos_)
        : scopes(scopes_), endPos(endPos_) {}
};

// Main tokenization function
StackElement tokenizeString(
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    int linePos,
    StateStackImpl* stack,
    LineTokens* lineTokens,
    bool checkWhileConditions,
    int timeLimit
);

// Helper functions
IWhileCheckResult _checkWhileConditions(
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    int linePos,
    StateStackImpl* stack,
    LineTokens* lineTokens
);

IMatchResult* matchRuleOrInjections(
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    int linePos,
    StateStackImpl* stack,
    int anchorPosition
);

IMatchResult* matchRule(
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    int linePos,
    StateStackImpl* stack,
    int anchorPosition
);

IMatchInjectionsResult* matchInjections(
    const std::vector<Injection>& injections,
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    int linePos,
    StateStackImpl* stack,
    int anchorPosition
);

void handleCaptures(
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    StateStackImpl* stack,
    LineTokens* lineTokens,
    const std::vector<CaptureRule*>& captures,
    const std::vector<IOnigCaptureIndex>& captureIndices
);

} // namespace tml

#endif // TEXTMATELIB_TOKENIZE_STRING_H
