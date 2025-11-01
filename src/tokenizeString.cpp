#include "tokenizeString.h"
#include "grammar.h"
#include "rule.h"
#include <chrono>
#include <limits>
#include <algorithm>
#include <iostream>

namespace vscode_textmate {

// Forward declarations of helper functions
static int getFindOptions(bool allowA, bool allowG);
static void prepareRuleSearch(
    Rule* rule,
    Grammar* grammar,
    const std::string* endRegexSource,
    bool allowA,
    bool allowG,
    CompiledRule*& ruleScanner,
    int& findOptions
);
static void prepareRuleWhileSearch(
    BeginWhileRule* rule,
    Grammar* grammar,
    const std::string* endRegexSource,
    bool allowA,
    bool allowG,
    CompiledRule*& ruleScanner,
    int& findOptions
);

// Get find options based on anchor flags
static int getFindOptions(bool allowA, bool allowG) {
    int options = FindOption::None;
    if (!allowA) {
        options |= FindOption::NotBeginString;
    }
    if (!allowG) {
        options |= FindOption::NotBeginPosition;
    }
    return options;
}

// Prepare rule for scanning
static void prepareRuleSearch(
    Rule* rule,
    Grammar* grammar,
    const std::string* endRegexSource,
    bool allowA,
    bool allowG,
    CompiledRule*& ruleScanner,
    int& findOptions
) {
    // In the TS version, there's a UseOnigurumaFindOptions flag
    // For now, we'll use compileAG which handles allowA/allowG
    ruleScanner = rule->compileAG(grammar, grammar, endRegexSource, allowA, allowG);
    findOptions = FindOption::None;
}

// Prepare while rule for scanning
static void prepareRuleWhileSearch(
    BeginWhileRule* rule,
    Grammar* grammar,
    const std::string* endRegexSource,
    bool allowA,
    bool allowG,
    CompiledRule*& ruleScanner,
    int& findOptions
) {
    ruleScanner = rule->compileWhileAG(grammar, endRegexSource, allowA, allowG);
    findOptions = FindOption::None;
}

// Check while conditions
IWhileCheckResult _checkWhileConditions(
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    int linePos,
    StateStackImpl* stack,
    LineTokens* lineTokens
) {
    IWhileCheckResult result;
    result.stack = stack;
    result.linePos = linePos;
    result.isFirstLine = isFirstLine;
    result.anchorPosition = (stack->beginRuleCapturedEOL ? 0 : -1);

    // Collect all while rules in the stack
    struct WhileStack {
        StateStackImpl* stack;
        BeginWhileRule* rule;
    };
    std::vector<WhileStack> whileRules;

    for (StateStackImpl* node = stack; node != nullptr; node = node->parent) {
        Rule* nodeRule = node->getRule(grammar);
        BeginWhileRule* whileRule = dynamic_cast<BeginWhileRule*>(nodeRule);
        if (whileRule) {
            WhileStack ws;
            ws.stack = node;
            ws.rule = whileRule;
            whileRules.push_back(ws);
        }
    }

    // Check each while rule from bottom to top
    std::reverse(whileRules.begin(), whileRules.end());

    for (const auto& whileRuleEntry : whileRules) {
        CompiledRule* ruleScanner = nullptr;
        int findOptions = 0;
        prepareRuleWhileSearch(
            whileRuleEntry.rule,
            grammar,
            whileRuleEntry.stack->endRule,
            isFirstLine,
            linePos == result.anchorPosition,
            ruleScanner,
            findOptions
        );

        IOnigMatch* r = ruleScanner->scanner->findNextMatchSync(
            lineText,
            linePos,
            findOptions
        );

        if (r) {
            RuleId matchedRuleId = ruleScanner->rules[r->index];
            if (ruleIdToNumber(matchedRuleId) != ruleIdToNumber(WHILE_RULE_ID)) {
                // Shouldn't end up here
                result.stack = whileRuleEntry.stack->parent;
                break;
            }

            if (!r->captureIndices.empty()) {
                lineTokens->produce(whileRuleEntry.stack, r->captureIndices[0].start);
                handleCaptures(
                    grammar,
                    lineText,
                    isFirstLine,
                    whileRuleEntry.stack,
                    lineTokens,
                    whileRuleEntry.rule->whileCaptures,
                    r->captureIndices
                );
                lineTokens->produce(whileRuleEntry.stack, r->captureIndices[0].end);
                result.anchorPosition = r->captureIndices[0].end;
                if (r->captureIndices[0].end > linePos) {
                    result.linePos = r->captureIndices[0].end;
                    result.isFirstLine = false;
                }
            }
            delete r;
        } else {
            // While condition failed, pop the rule
            result.stack = whileRuleEntry.stack->parent;
            break;
        }
    }

    return result;
}

// Match rule or injections
IMatchResult* matchRuleOrInjections(
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    int linePos,
    StateStackImpl* stack,
    int anchorPosition
) {
    // Look for normal grammar rule
    IMatchResult* matchResult = matchRule(
        grammar,
        lineText,
        isFirstLine,
        linePos,
        stack,
        anchorPosition
    );

    // Look for injected rules
    std::vector<Injection> injections = grammar->getInjections();
    if (injections.empty()) {
        // No injections whatsoever => early return
        return matchResult;
    }

    IMatchInjectionsResult* injectionResult = matchInjections(
        injections,
        grammar,
        lineText,
        isFirstLine,
        linePos,
        stack,
        anchorPosition
    );

    if (!injectionResult) {
        // No injections matched => early return
        return matchResult;
    }

    if (!matchResult) {
        // Only injections matched => convert and return
        IMatchResult* result = new IMatchResult();
        result->captureIndices = injectionResult->captureIndices;
        result->matchedRuleId = injectionResult->matchedRuleId;
        delete injectionResult;
        return result;
    }

    // Decide if matchResult or injectionResult should win
    int matchResultScore = matchResult->captureIndices[0].start;
    int injectionResultScore = injectionResult->captureIndices[0].start;

    if (injectionResultScore < matchResultScore ||
        (injectionResult->priorityMatch && injectionResultScore == matchResultScore)) {
        // Injection won!
        IMatchResult* result = new IMatchResult();
        result->captureIndices = injectionResult->captureIndices;
        result->matchedRuleId = injectionResult->matchedRuleId;
        delete matchResult;
        delete injectionResult;
        return result;
    }

    delete injectionResult;
    return matchResult;
}

// Match rule
IMatchResult* matchRule(
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    int linePos,
    StateStackImpl* stack,
    int anchorPosition
) {
    Rule* rule = stack->getRule(grammar);
    if (!rule) {
        return nullptr;
    }

    CompiledRule* ruleScanner = nullptr;
    int findOptions = 0;
    prepareRuleSearch(
        rule,
        grammar,
        stack->endRule,
        isFirstLine,
        linePos == anchorPosition,
        ruleScanner,
        findOptions
    );

    IOnigMatch* r = ruleScanner->scanner->findNextMatchSync(
        lineText,
        linePos,
        findOptions
    );

    if (r) {
        IMatchResult* result = new IMatchResult();
        result->captureIndices = r->captureIndices;
        result->matchedRuleId = ruleScanner->rules[r->index];
        delete r;
        return result;
    }

    return nullptr;
}

// Match injections
IMatchInjectionsResult* matchInjections(
    const std::vector<Injection>& injections,
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    int linePos,
    StateStackImpl* stack,
    int anchorPosition
) {
    // The lower the better
    int bestMatchRating = std::numeric_limits<int>::max();
    std::vector<IOnigCaptureIndex> bestMatchCaptureIndices;
    RuleId bestMatchRuleId = ruleIdFromNumber(-1);
    int bestMatchResultPriority = 0;
    bool foundMatch = false;

    std::vector<std::string> scopes = stack->contentNameScopesList->getScopeNames();

    for (size_t i = 0; i < injections.size(); i++) {
        const Injection& injection = injections[i];
        if (!injection.matcher(scopes)) {
            // Injection selector doesn't match stack
            continue;
        }

        Rule* rule = grammar->getRule(injection.ruleId);
        if (!rule) {
            continue;
        }

        CompiledRule* ruleScanner = nullptr;
        int findOptions = 0;
        prepareRuleSearch(
            rule,
            grammar,
            nullptr,
            isFirstLine,
            linePos == anchorPosition,
            ruleScanner,
            findOptions
        );

        IOnigMatch* matchResult = ruleScanner->scanner->findNextMatchSync(
            lineText,
            linePos,
            findOptions
        );

        if (!matchResult) {
            continue;
        }

        int matchRating = matchResult->captureIndices[0].start;
        if (matchRating >= bestMatchRating) {
            // Injections are sorted by priority, so previous injection had better priority
            delete matchResult;
            continue;
        }

        bestMatchRating = matchRating;
        bestMatchCaptureIndices = matchResult->captureIndices;
        bestMatchRuleId = ruleScanner->rules[matchResult->index];
        bestMatchResultPriority = injection.priority;
        foundMatch = true;

        delete matchResult;

        if (bestMatchRating == linePos) {
            // No more need to look at the rest of the injections
            break;
        }
    }

    if (foundMatch) {
        IMatchInjectionsResult* result = new IMatchInjectionsResult();
        result->priorityMatch = (bestMatchResultPriority == -1);
        result->captureIndices = bestMatchCaptureIndices;
        result->matchedRuleId = bestMatchRuleId;
        return result;
    }

    return nullptr;
}

// Handle captures
void handleCaptures(
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    StateStackImpl* stack,
    LineTokens* lineTokens,
    const std::vector<CaptureRule*>& captures,
    const std::vector<IOnigCaptureIndex>& captureIndices
) {
    if (captures.empty()) {
        return;
    }

    const std::string& lineTextContent = lineText->content();

    size_t len = std::min(captures.size(), captureIndices.size());
    std::vector<LocalStackElement> localStack;
    int maxEnd = captureIndices[0].end;

    for (size_t i = 0; i < len; i++) {
        const CaptureRule* captureRule = captures[i];
        if (captureRule == nullptr) {
            // Not interested
            continue;
        }

        const IOnigCaptureIndex& captureIndex = captureIndices[i];

        if (captureIndex.length == 0) {
            // Nothing really captured
            continue;
        }

        if (captureIndex.start > maxEnd) {
            // Capture going beyond consumed string
            break;
        }

        // Pop captures while needed
        while (!localStack.empty() &&
               localStack.back().endPos <= captureIndex.start) {
            // Pop!
            lineTokens->produceFromScopes(
                localStack.back().scopes,
                localStack.back().endPos
            );
            localStack.pop_back();
        }

        if (!localStack.empty()) {
            lineTokens->produceFromScopes(
                localStack.back().scopes,
                captureIndex.start
            );
        } else {
            lineTokens->produce(stack, captureIndex.start);
        }

        if (ruleIdToNumber(captureRule->retokenizeCapturedWithRuleId) != 0) {
            // The capture requires additional matching
            std::string* scopeName = captureRule->getName(
                &lineTextContent,
                &captureIndices
            );
            AttributedScopeStack* nameScopesList =
                stack->contentNameScopesList->pushAttributed(
                    scopeName ? *scopeName : "",
                    grammar
                );

            std::string* contentName = captureRule->getContentName(
                lineTextContent,
                captureIndices
            );
            AttributedScopeStack* contentNameScopesList =
                nameScopesList->pushAttributed(
                    contentName ? *contentName : "",
                    grammar
                );

            StateStackImpl* stackClone = stack->push(
                captureRule->retokenizeCapturedWithRuleId,
                captureIndex.start,
                -1,
                false,
                nullptr,
                nameScopesList,
                contentNameScopesList
            );

            std::string substring = lineTextContent.substr(0, captureIndex.end);
            OnigString* onigSubStr = grammar->createOnigString(substring);
            StackElement subResult = tokenizeString(
                grammar,
                onigSubStr,
                (isFirstLine && captureIndex.start == 0),
                captureIndex.start,
                stackClone,
                lineTokens,
                false,
                0  // No time limit
            );

            delete onigSubStr;
            delete scopeName;
            delete contentName;
            continue;
        }

        std::string* captureRuleScopeName = captureRule->getName(
            &lineTextContent,
            &captureIndices
        );

        if (captureRuleScopeName != nullptr && !captureRuleScopeName->empty()) {
            // Push
            AttributedScopeStack* base = !localStack.empty()
                ? localStack.back().scopes
                : stack->contentNameScopesList;
            AttributedScopeStack* captureRuleScopesList =
                base->pushAttributed(*captureRuleScopeName, grammar);
            localStack.push_back(
                LocalStackElement(captureRuleScopesList, captureIndex.end)
            );
        }

        delete captureRuleScopeName;
    }

    while (!localStack.empty()) {
        // Pop!
        lineTokens->produceFromScopes(
            localStack.back().scopes,
            localStack.back().endPos
        );
        localStack.pop_back();
    }
}

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
) {
    StackElement result;
    int lineLength = lineText->content().length();
    bool STOP = false;
    int anchorPosition = -1;

    if (checkWhileConditions) {
        IWhileCheckResult whileCheckResult = _checkWhileConditions(
            grammar,
            lineText,
            isFirstLine,
            linePos,
            stack,
            lineTokens
        );
        stack = whileCheckResult.stack;
        linePos = whileCheckResult.linePos;
        isFirstLine = whileCheckResult.isFirstLine;
        anchorPosition = whileCheckResult.anchorPosition;
    }

    auto startTime = std::chrono::steady_clock::now();

    // Main scanning loop
    while (!STOP) {

        // Check time limit
        if (timeLimit != 0) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - startTime
            ).count();
            if (elapsedTime > timeLimit) {
                result.stack = stack;
                result.stoppedEarly = true;
                return result;
            }
        }

        // scanNext logic
        IMatchResult* r = matchRuleOrInjections(
            grammar,
            lineText,
            isFirstLine,
            linePos,
            stack,
            anchorPosition
        );

        if (!r) {
            // No match
            lineTokens->produce(stack, lineLength);
            STOP = true;
            continue;
        }

        const std::vector<IOnigCaptureIndex>& captureIndices = r->captureIndices;
        RuleId matchedRuleId = r->matchedRuleId;

        bool hasAdvanced = !captureIndices.empty() &&
                          captureIndices[0].end > linePos;

        if (ruleIdToNumber(matchedRuleId) == ruleIdToNumber(END_RULE_ID)) {
            // We matched the `end` for this rule => pop it
            BeginEndRule* poppedRule = dynamic_cast<BeginEndRule*>(
                stack->getRule(grammar)
            );

            lineTokens->produce(stack, captureIndices[0].start);
            stack = stack->withContentNameScopesList(stack->nameScopesList);
            handleCaptures(
                grammar,
                lineText,
                isFirstLine,
                stack,
                lineTokens,
                poppedRule->endCaptures,
                captureIndices
            );
            lineTokens->produce(stack, captureIndices[0].end);

            // Pop
            StateStackImpl* popped = stack;
            stack = stack->parent;
            anchorPosition = popped->getAnchorPos();

            if (!hasAdvanced && popped->getEnterPos() == linePos) {
                // Grammar pushed & popped a rule without advancing
                // Assume this was a mistake and continue in this state
                stack = popped;
                lineTokens->produce(stack, lineLength);
                STOP = true;
                delete r;
                continue;
            }
        } else {
            // We matched a rule!
            Rule* _rule = grammar->getRule(matchedRuleId);

            lineTokens->produce(stack, captureIndices[0].start);

            StateStackImpl* beforePush = stack;

            // Push it on the stack rule
            std::string* scopeName = _rule->getName(
                &lineText->content(),
                &captureIndices
            );
            AttributedScopeStack* nameScopesList =
                stack->contentNameScopesList->pushAttributed(
                    scopeName ? *scopeName : "",
                    grammar
                );
            stack = stack->push(
                matchedRuleId,
                linePos,
                anchorPosition,
                captureIndices[0].end == lineLength,
                nullptr,
                nameScopesList,
                nameScopesList
            );

            delete scopeName;

            BeginEndRule* beginEndRule = dynamic_cast<BeginEndRule*>(_rule);
            if (beginEndRule) {
                handleCaptures(
                    grammar,
                    lineText,
                    isFirstLine,
                    stack,
                    lineTokens,
                    beginEndRule->beginCaptures,
                    captureIndices
                );
                lineTokens->produce(stack, captureIndices[0].end);
                anchorPosition = captureIndices[0].end;

                std::string* contentName = beginEndRule->getContentName(
                    lineText->content(),
                    captureIndices
                );
                AttributedScopeStack* contentNameScopesList =
                    nameScopesList->pushAttributed(
                        contentName ? *contentName : "",
                        grammar
                    );
                stack = stack->withContentNameScopesList(contentNameScopesList);

                delete contentName;

                if (beginEndRule->endHasBackReferences) {
                    std::string endRule = beginEndRule->getEndWithResolvedBackReferences(
                        lineText->content(),
                        captureIndices
                    );
                    stack = stack->withEndRule(endRule);
                }

                if (!hasAdvanced && beforePush->hasSameRuleAs(stack)) {
                    // Grammar pushed the same rule without advancing
                    stack = stack->pop();
                    lineTokens->produce(stack, lineLength);
                    STOP = true;
                    delete r;
                    continue;
                }
            } else {
                BeginWhileRule* beginWhileRule = dynamic_cast<BeginWhileRule*>(_rule);
                if (beginWhileRule) {
                    handleCaptures(
                        grammar,
                        lineText,
                        isFirstLine,
                        stack,
                        lineTokens,
                        beginWhileRule->beginCaptures,
                        captureIndices
                    );
                    lineTokens->produce(stack, captureIndices[0].end);
                    anchorPosition = captureIndices[0].end;

                    std::string* contentName = beginWhileRule->getContentName(
                        lineText->content(),
                        captureIndices
                    );
                    AttributedScopeStack* contentNameScopesList =
                        nameScopesList->pushAttributed(
                            contentName ? *contentName : "",
                            grammar
                        );
                    stack = stack->withContentNameScopesList(contentNameScopesList);

                    delete contentName;

                    if (beginWhileRule->whileHasBackReferences) {
                        std::string whileRule =
                            beginWhileRule->getWhileWithResolvedBackReferences(
                                lineText->content(),
                                captureIndices
                            );
                        stack = stack->withEndRule(whileRule);
                    }

                    if (!hasAdvanced && beforePush->hasSameRuleAs(stack)) {
                        // Grammar pushed the same rule without advancing
                        stack = stack->pop();
                        lineTokens->produce(stack, lineLength);
                        STOP = true;
                        delete r;
                        continue;
                    }
                } else {
                    // MatchRule
                    MatchRule* matchingRule = dynamic_cast<MatchRule*>(_rule);
                    if (matchingRule) {
                        handleCaptures(
                            grammar,
                            lineText,
                            isFirstLine,
                            stack,
                            lineTokens,
                            matchingRule->captures,
                            captureIndices
                        );
                        lineTokens->produce(stack, captureIndices[0].end);

                        // Pop rule immediately since it is a MatchRule
                        stack = stack->pop();

                        if (!hasAdvanced) {
                            // Grammar is not advancing, nor is it pushing/popping
                            stack = stack->safePop();
                            lineTokens->produce(stack, lineLength);
                            STOP = true;
                            delete r;
                            continue;
                        }
                    }
                }
            }
        }

        if (!captureIndices.empty() && captureIndices[0].end > linePos) {
            // Advance stream
            linePos = captureIndices[0].end;
            isFirstLine = false;
        }

        delete r;
    }

    result.stack = stack;
    result.stoppedEarly = false;
    result.linePos = linePos;
    result.anchorPosition = anchorPosition;
    return result;
}

} // namespace vscode_textmate
