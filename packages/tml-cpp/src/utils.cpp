#include "utils.h"
#include "onigLib.h"
#include <algorithm>
#include <cctype>
#include <chrono>

namespace tml {

std::string basename(const std::string& path) {
    size_t idx = path.find_last_of("/\\");
    if (idx == std::string::npos) {
        return path;
    } else if (idx == path.length() - 1) {
        return basename(path.substr(0, path.length() - 1));
    } else {
        return path.substr(idx + 1);
    }
}

int strcmp_custom(const std::string& a, const std::string& b) {
    if (a < b) {
        return -1;
    }
    if (a > b) {
        return 1;
    }
    return 0;
}

int strArrCmp(const std::vector<std::string>* a, const std::vector<std::string>* b) {
    if (a == nullptr && b == nullptr) {
        return 0;
    }
    if (a == nullptr) {
        return -1;
    }
    if (b == nullptr) {
        return 1;
    }
    size_t len1 = a->size();
    size_t len2 = b->size();
    if (len1 == len2) {
        for (size_t i = 0; i < len1; i++) {
            int res = strcmp_custom((*a)[i], (*b)[i]);
            if (res != 0) {
                return res;
            }
        }
        return 0;
    }
    return static_cast<int>(len1) - static_cast<int>(len2);
}

bool isValidHexColor(const std::string& hex) {
    std::regex pattern1("^#[0-9a-fA-F]{6}$");    // #rrggbb
    std::regex pattern2("^#[0-9a-fA-F]{8}$");    // #rrggbbaa
    std::regex pattern3("^#[0-9a-fA-F]{3}$");    // #rgb
    std::regex pattern4("^#[0-9a-fA-F]{4}$");    // #rgba

    return std::regex_match(hex, pattern1) ||
           std::regex_match(hex, pattern2) ||
           std::regex_match(hex, pattern3) ||
           std::regex_match(hex, pattern4);
}

std::string escapeRegExpCharacters(const std::string& value) {
    std::string result;
    result.reserve(value.length() * 2);
    for (char c : value) {
        if (c == '-' || c == '\\' || c == '{' || c == '}' || c == '*' ||
            c == '+' || c == '?' || c == '|' || c == '^' || c == '$' ||
            c == '.' || c == ',' || c == '[' || c == ']' || c == '(' ||
            c == ')' || c == '#' || std::isspace(c)) {
            result += '\\';
        }
        result += c;
    }
    return result;
}

// RTL detection regex pattern
static std::regex* CONTAINS_RTL = nullptr;

static std::regex makeContainsRtl() {
    // Generated using https://github.com/alexdima/unicode-utils/blob/main/rtl-test.js
    return std::regex(
        "(?:[\u05BE\u05C0\u05C3\u05C6\u05D0-\u05F4\u0608\u060B\u060D\u061B-\u064A\u066D-\u066F"
        "\u0671-\u06D5\u06E5\u06E6\u06EE\u06EF\u06FA-\u0710\u0712-\u072F\u074D-\u07A5\u07B1-\u07EA"
        "\u07F4\u07F5\u07FA\u07FE-\u0815\u081A\u0824\u0828\u0830-\u0858\u085E-\u088E\u08A0-\u08C9"
        "\u200F\uFB1D\uFB1F-\uFB28\uFB2A-\uFD3D\uFD50-\uFDC7\uFDF0-\uFDFC\uFE70-\uFEFC])",
        std::regex_constants::ECMAScript
    );
}

bool containsRTL(const std::string& str) {
    if (CONTAINS_RTL == nullptr) {
        CONTAINS_RTL = new std::regex(makeContainsRtl());
    }
    return std::regex_search(str, *CONTAINS_RTL);
}

double performanceNow() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// RegexSource implementation

static std::regex CAPTURING_REGEX_SOURCE("\\$(\\d+)|\\$\\{(\\d+):\\/(downcase|upcase)\\}");

RegexSource::RegexSource(const std::string& regExpSource, RuleId ruleId_)
    : source(regExpSource), ruleId(ruleId_), hasAnchor(false), hasBackReferences(false) {

    // Check for anchors
    if (regExpSource.find("\\A") != std::string::npos ||
        regExpSource.find("\\G") != std::string::npos) {
        hasAnchor = true;
    }

    // Check for back references
    std::regex backRefPattern("\\\\(\\d+)");
    hasBackReferences = std::regex_search(regExpSource, backRefPattern);

    // Build anchor cache if needed
    if (hasAnchor) {
        buildAnchorCache();
    }
}

bool RegexSource::hasCaptures(const std::string* regexSource) {
    if (regexSource == nullptr) {
        return false;
    }
    return std::regex_search(*regexSource, CAPTURING_REGEX_SOURCE);
}

std::string RegexSource::replaceCaptures(const std::string& regexSource,
                                          const std::string& captureSource,
                                          const std::vector<IOnigCaptureIndex>& captureIndices) {
    std::string result;
    std::smatch match;
    std::string::const_iterator searchStart(regexSource.cbegin());

    while (std::regex_search(searchStart, regexSource.cend(), match, CAPTURING_REGEX_SOURCE)) {
        result += match.prefix();

        std::string indexStr = match[1].matched ? match[1].str() : match[2].str();
        std::string command = match[3].matched ? match[3].str() : "";

        int index = std::stoi(indexStr);

        if (index < static_cast<int>(captureIndices.size())) {
            const IOnigCaptureIndex& capture = captureIndices[index];
            if (capture.length > 0) {
                std::string captureText = captureSource.substr(capture.start, capture.length);

                // Remove leading dots
                while (!captureText.empty() && captureText[0] == '.') {
                    captureText = captureText.substr(1);
                }

                if (command == "downcase") {
                    std::transform(captureText.begin(), captureText.end(), captureText.begin(), ::tolower);
                } else if (command == "upcase") {
                    std::transform(captureText.begin(), captureText.end(), captureText.begin(), ::toupper);
                }

                result += captureText;
            } else {
                result += match.str();
            }
        } else {
            result += match.str();
        }

        searchStart = match.suffix().first;
    }

    result += std::string(searchStart, regexSource.cend());
    return result;
}

std::string RegexSource::resolveBackReferences(const std::string& lineText,
                                                const std::vector<IOnigCaptureIndex>& captureIndices) {
    std::regex backRefPattern("\\\\(\\d+)");
    std::string result;
    std::smatch match;
    std::string::const_iterator searchStart(source.cbegin());

    while (std::regex_search(searchStart, source.cend(), match, backRefPattern)) {
        result += match.prefix();

        int index = std::stoi(match[1].str());

        if (index < static_cast<int>(captureIndices.size())) {
            const IOnigCaptureIndex& capture = captureIndices[index];
            if (capture.length > 0) {
                std::string captureText = lineText.substr(capture.start, capture.length);
                result += escapeRegExpCharacters(captureText);
            }
        } else {
            result += match.str();
        }

        searchStart = match.suffix().first;
    }

    result += std::string(searchStart, source.cend());
    return result;
}

void RegexSource::buildAnchorCache() {
    // Build all 4 variants of the anchor cache
    // A0_G0: \A -> \uFFFF, \G -> \uFFFF (replace with char that never matches)
    // A0_G1: \A -> \uFFFF, \G -> \G
    // A1_G0: \A -> \A, \G -> \uFFFF
    // A1_G1: \A -> \A, \G -> \G

    // Start with copies of the source
    std::string A0_G0_result;
    std::string A0_G1_result;
    std::string A1_G0_result;
    std::string A1_G1_result;

    A0_G0_result.reserve(source.length());
    A0_G1_result.reserve(source.length());
    A1_G0_result.reserve(source.length());
    A1_G1_result.reserve(source.length());

    for (size_t i = 0; i < source.length(); i++) {
        char ch = source[i];

        // Default: copy character as-is
        A0_G0_result += ch;
        A0_G1_result += ch;
        A1_G0_result += ch;
        A1_G1_result += ch;

        if (ch == '\\' && i + 1 < source.length()) {
            char nextCh = source[i + 1];
            i++; // Skip the next character in the loop

            if (nextCh == 'A') {
                // Replace \A based on allowA flag
                // When allowA=false, replace with \uFFFF (a character that will never match)
                A0_G0_result += "\uFFFF";  // A=false, G=false
                A0_G1_result += "\uFFFF";  // A=false, G=true
                A1_G0_result += 'A';        // A=true, G=false
                A1_G1_result += 'A';        // A=true, G=true
            } else if (nextCh == 'G') {
                // Replace \G based on allowG flag
                A0_G0_result += "\uFFFF";  // A=false, G=false
                A0_G1_result += 'G';        // A=false, G=true
                A1_G0_result += "\uFFFF";  // A=true, G=false
                A1_G1_result += 'G';        // A=true, G=true
            } else {
                // Other escaped characters, keep as-is
                A0_G0_result += nextCh;
                A0_G1_result += nextCh;
                A1_G0_result += nextCh;
                A1_G1_result += nextCh;
            }
        }
    }

    anchorCache_A0_G0 = A0_G0_result;
    anchorCache_A0_G1 = A0_G1_result;
    anchorCache_A1_G0 = A1_G0_result;
    anchorCache_A1_G1 = A1_G1_result;
}

std::string RegexSource::resolveAnchors(bool allowA, bool allowG) const {
    if (!hasAnchor) {
        return source;
    }

    if (allowA) {
        if (allowG) {
            return anchorCache_A1_G1;
        } else {
            return anchorCache_A1_G0;
        }
    } else {
        if (allowG) {
            return anchorCache_A0_G1;
        } else {
            return anchorCache_A0_G0;
        }
    }
}

RegexSource* RegexSource::clone() const {
    return new RegexSource(source, ruleId);
}

} // namespace tml
