#ifndef VSCODE_TEXTMATE_ONIGLIB_H
#define VSCODE_TEXTMATE_ONIGLIB_H

#include "types.h"
#include <string>
#include <vector>
#include <memory>

// Include Oniguruma header
extern "C" {
#include "oniguruma.h"
}

namespace vscode_textmate {

// IOnigCaptureIndex interface
struct IOnigCaptureIndex {
    int start;
    int end;
    int length;

    IOnigCaptureIndex() : start(0), end(0), length(0) {}
    IOnigCaptureIndex(int s, int e) : start(s), end(e), length(e - s) {}
};

// IOnigMatch interface
struct IOnigMatch {
    int index;  // Which pattern matched (for scanner with multiple patterns)
    std::vector<IOnigCaptureIndex> captureIndices;

    IOnigMatch() : index(0) {}
};

// FindOption enum
enum FindOption {
    None = 0,
    NotBeginString = 1,      // ONIG_OPTION_NOT_BEGIN_STRING
    NotEndString = 2,        // ONIG_OPTION_NOT_END_STRING
    NotBeginPosition = 4,    // ONIG_OPTION_NOT_BEGIN_POSITION
    DebugCall = 8            // For debugging purposes
};

// OnigString class - wraps a string for use with Oniguruma
class OnigString {
private:
    std::string _content;
    const UChar* _utf8Ptr;

public:
    explicit OnigString(const std::string& str);
    ~OnigString();

    const std::string& content() const { return _content; }
    const UChar* utf8Ptr() const { return _utf8Ptr; }
    size_t utf8Length() const { return _content.length(); }

    void dispose();
};

// OnigScanner class - wraps Oniguruma scanner
class OnigScanner {
private:
    std::vector<std::string> _sources;
    std::vector<OnigRegex> _regexes;
    OnigRegSet* _regSet;
    bool _disposed;

public:
    explicit OnigScanner(const std::vector<std::string>& sources);
    ~OnigScanner();

    IOnigMatch* findNextMatchSync(const std::string& string,
                                   int startPosition,
                                   OrMask<FindOption> options);

    IOnigMatch* findNextMatchSync(OnigString* string,
                                   int startPosition,
                                   OrMask<FindOption> options);

    void dispose();

private:
    bool compilePatterns();
};

// IOnigLib interface - factory for creating OnigScanner and OnigString
class IOnigLib {
public:
    virtual ~IOnigLib() {}

    virtual OnigScanner* createOnigScanner(const std::vector<std::string>& sources) = 0;
    virtual OnigString* createOnigString(const std::string& str) = 0;
};

// Default OnigLib implementation
class DefaultOnigLib : public IOnigLib {
public:
    DefaultOnigLib();
    ~DefaultOnigLib();

    OnigScanner* createOnigScanner(const std::vector<std::string>& sources) override;
    OnigString* createOnigString(const std::string& str) override;
};

// Helper function to dispose OnigString
void disposeOnigString(OnigString* str);

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_ONIGLIB_H
