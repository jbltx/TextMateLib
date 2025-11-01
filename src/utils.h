#ifndef VSCODE_TEXTMATE_UTILS_H
#define VSCODE_TEXTMATE_UTILS_H

#include "types.h"
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <functional>

namespace vscode_textmate {

// Forward declaration
struct IOnigCaptureIndex;

// Clone functions
template<typename T>
T clone(const T& something);

std::string mergeObjects(const std::string& target, const std::vector<std::string>& sources);

// Basename function
std::string basename(const std::string& path);

// String comparison functions
int strcmp_custom(const std::string& a, const std::string& b);
int strArrCmp(const std::vector<std::string>* a, const std::vector<std::string>* b);

// Hex color validation
bool isValidHexColor(const std::string& hex);

// Escape regular expression characters
std::string escapeRegExpCharacters(const std::string& value);

// Returns true if str contains any Unicode character classified as "R" or "AL"
bool containsRTL(const std::string& str);

// Performance timing
double performanceNow();

// RegexSource class
class RegexSource {
public:
    std::string source;
    RuleId ruleId;
    bool hasAnchor;
    bool hasBackReferences;
    std::string anchorCache_A0_G0;
    std::string anchorCache_A0_G1;
    std::string anchorCache_A1_G0;
    std::string anchorCache_A1_G1;

    RegexSource(const std::string& regExpSource, RuleId ruleId_);

    static bool hasCaptures(const std::string* regexSource);
    static std::string replaceCaptures(const std::string& regexSource,
                                       const std::string& captureSource,
                                       const std::vector<IOnigCaptureIndex>& captureIndices);

    std::string resolveBackReferences(const std::string& lineText,
                                      const std::vector<IOnigCaptureIndex>& captureIndices);

    void buildAnchorCache();
    std::string resolveAnchors(bool allowA, bool allowG) const;

    RegexSource* clone() const;
};

// CachedFn template class
template<typename TKey, typename TValue>
class CachedFn {
private:
    std::map<TKey, TValue> cache;
    std::function<TValue(const TKey&)> fn;

public:
    CachedFn(std::function<TValue(const TKey&)> func) : fn(func) {}

    TValue get(const TKey& key) {
        auto it = cache.find(key);
        if (it != cache.end()) {
            return it->second;
        }
        TValue value = fn(key);
        cache[key] = value;
        return value;
    }
};

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_UTILS_H
