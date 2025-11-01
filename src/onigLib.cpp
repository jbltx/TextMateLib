#include "onigLib.h"
#include <cstring>
#include <iostream>

namespace vscode_textmate {

// OnigString implementation

OnigString::OnigString(const std::string& str)
    : _content(str), _utf8Ptr(reinterpret_cast<const UChar*>(_content.c_str())) {
}

OnigString::~OnigString() {
    dispose();
}

void OnigString::dispose() {
    // Nothing to dispose in this implementation
    // The string content is managed by std::string
}

// OnigScanner implementation

OnigScanner::OnigScanner(const std::vector<std::string>& sources)
    : _sources(sources), _regSet(nullptr), _disposed(false) {
    _regexes.resize(sources.size(), nullptr);
    compilePatterns();
}

OnigScanner::~OnigScanner() {
    dispose();
}

bool OnigScanner::compilePatterns() {
    OnigEncoding encoding = ONIG_ENCODING_UTF8;
    OnigSyntaxType* syntax = ONIG_SYNTAX_DEFAULT;

    for (size_t i = 0; i < _sources.size(); i++) {
        const std::string& pattern = _sources[i];
        OnigRegex reg = nullptr;

        const UChar* patternPtr = reinterpret_cast<const UChar*>(pattern.c_str());
        const UChar* patternEnd = patternPtr + pattern.length();

        OnigErrorInfo einfo;
        int r = onig_new(&reg, patternPtr, patternEnd, ONIG_OPTION_CAPTURE_GROUP,
                        encoding, syntax, &einfo);

        if (r != ONIG_NORMAL) {
            // Error compiling pattern
            OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
            onig_error_code_to_str(s, r, &einfo);
            // In production, we should handle this error properly
            return false;
        }

        _regexes[i] = reg;
    }

    // Create regex set for efficient multi-pattern matching
    if (!_regexes.empty()) {
        int n = _regexes.size();
        OnigRegex* regArray = new OnigRegex[n];
        for (int i = 0; i < n; i++) {
            regArray[i] = _regexes[i];
        }

        int r = onig_regset_new(&_regSet, n, regArray);
        delete[] regArray;

        if (r != ONIG_NORMAL) {
            return false;
        }
    }

    return true;
}

IOnigMatch* OnigScanner::findNextMatchSync(const std::string& string,
                                            int startPosition,
                                            OrMask<FindOption> options) {
    OnigString onigStr(string);
    return findNextMatchSync(&onigStr, startPosition, options);
}

IOnigMatch* OnigScanner::findNextMatchSync(OnigString* string,
                                            int startPosition,
                                            OrMask<FindOption> options) {
    if (_disposed || _regSet == nullptr || string == nullptr) {
        return nullptr;
    }

    const UChar* strPtr = string->utf8Ptr();
    const UChar* strEnd = strPtr + string->utf8Length();
    const UChar* start = strPtr + startPosition;

    if (start > strEnd) {
        return nullptr;
    }

    // Convert options to Oniguruma options
    OnigOptionType onigOptions = ONIG_OPTION_NONE;
    if (options & NotBeginString) {
        onigOptions |= ONIG_OPTION_NOT_BEGIN_STRING;
    }
    if (options & NotEndString) {
        onigOptions |= ONIG_OPTION_NOT_END_STRING;
    }
    if (options & NotBeginPosition) {
        onigOptions |= ONIG_OPTION_NOT_BEGIN_POSITION;
    }

    // Search using regex set
    OnigRegSetLead lead = ONIG_REGSET_POSITION_LEAD;
    int matchPos = -1;
    int regIndex = onig_regset_search(_regSet, strPtr, strEnd, start, strEnd,
                                       lead, onigOptions, &matchPos);

    if (regIndex < 0) {
        // No match found
        return nullptr;
    }

    // Get the matched regex
    OnigRegex matchedReg = _regexes[regIndex];
    if (matchedReg == nullptr) {
        return nullptr;
    }

    // Create a region for capturing match info
    OnigRegion* region = onig_region_new();

    // Perform the match to get capture groups
    int r = onig_search(matchedReg, strPtr, strEnd, start, strEnd, region, onigOptions);
    if (r < 0) {
        onig_region_free(region, 1);
        return nullptr;
    }

    // Create match result
    IOnigMatch* match = new IOnigMatch();
    match->index = regIndex;

    // Extract capture groups
    for (int i = 0; i < region->num_regs; i++) {
        int captureStart = region->beg[i];
        int captureEnd = region->end[i];

        if (captureStart >= 0 && captureEnd >= 0) {
            match->captureIndices.push_back(IOnigCaptureIndex(captureStart, captureEnd));
        } else {
            match->captureIndices.push_back(IOnigCaptureIndex(0, 0));
        }
    }

    // Free the region
    onig_region_free(region, 1);

    return match;
}

void OnigScanner::dispose() {
    if (_disposed) {
        return;
    }

    if (_regSet != nullptr) {
        // ✅ FIX: onig_regset_free() already frees all the individual regexes
        // So we must clear the _regexes pointers to avoid double-free
        onig_regset_free(_regSet);
        _regSet = nullptr;

        // Clear the regex pointers since they were already freed by onig_regset_free
        for (size_t i = 0; i < _regexes.size(); i++) {
            _regexes[i] = nullptr;
        }
    } else {
        // No regset, need to free individual regexes manually
        for (size_t i = 0; i < _regexes.size(); i++) {
            if (_regexes[i] != nullptr) {
                onig_free(_regexes[i]);
                _regexes[i] = nullptr;
            }
        }
    }

    _disposed = true;
}

// DefaultOnigLib implementation

DefaultOnigLib::DefaultOnigLib() {
    // Initialize Oniguruma
    onig_init();
}

DefaultOnigLib::~DefaultOnigLib() {
    // End Oniguruma
    onig_end();
}

OnigScanner* DefaultOnigLib::createOnigScanner(const std::vector<std::string>& sources) {
    return new OnigScanner(sources);
}

OnigString* DefaultOnigLib::createOnigString(const std::string& str) {
    return new OnigString(str);
}

// Helper functions

void disposeOnigString(OnigString* str) {
    if (str != nullptr) {
        str->dispose();
    }
}

} // namespace vscode_textmate
