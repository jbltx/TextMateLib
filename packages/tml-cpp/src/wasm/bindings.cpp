#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "main.h"
#include "registry.h"
#include "grammar.h"
#include "parseRawGrammar.h"
#include "parseRawTheme.h"
#include "theme.h"
#include "onigLib.h"
#include "utf16_utils.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>

using namespace emscripten;
using namespace tml;

namespace {

// RAII guard for batch tokenization episodes: installs a StackNodeArena as active for its
// lifetime, then frees every node it recorded on destruction. Nothing produced by a batch
// call escapes to JS (the rule stack is carried internally only), so the whole graph can be
// reclaimed. Exception-safe: the destructor runs on any exit path.
struct BatchArena {
    StackNodeArena arena;
    StackNodeArena* prev;
    BatchArena() : prev(tmlGetActiveArena()) { tmlSetActiveArena(&arena); }
    ~BatchArena() {
        tmlSetActiveArena(prev);
        arena.clear();
    }
};

// RAII guard for per-line tokenization: records nodes created during the line, then on
// destruction frees the intra-line garbage while preserving the rule-stack chain the caller
// must hand back to JS for the next line. Set `keep` to the returned rule stack before exit;
// if an exception unwinds before that happens, `keep` stays null and everything is freed.
struct PerLineArena {
    StackNodeArena arena;
    StackNodeArena* prev;
    StateStack* keep = nullptr;
    PerLineArena() : prev(tmlGetActiveArena()) { tmlSetActiveArena(&arena); }
    ~PerLineArena() {
        tmlSetActiveArena(prev);
        if (keep) arena.sweepKeeping({ reinterpret_cast<StateStackImpl*>(keep) });
        else arena.clear();
    }
};

} // namespace

// Wrapper classes to expose to JavaScript
class RegistryWrapper {
private:
    Registry* registry;
    IOnigLib* onigLib;

public:
    RegistryWrapper() {
        onigLib = new DefaultOnigLib();
        RegistryOptions options;
        options.onigLib = onigLib;
        registry = new Registry(options);
    }

    ~RegistryWrapper() {
        delete registry;
        delete onigLib;
    }

    // Load grammar from JSON string
    val loadGrammarFromContent(const std::string& content, const std::string& scopeName) {
        try {
            IRawGrammar* rawGrammar = parseJSONGrammar(content, nullptr);
            if (!rawGrammar) {
                return val::null();
            }

            Grammar* grammar = registry->addGrammar(rawGrammar);
            if (!grammar) {
                return val::null();
            }

            // Return a handle/pointer as a number
            return val(reinterpret_cast<uintptr_t>(grammar));
        } catch (...) {
            return val::null();
        }
    }

    // Set theme from JSON string
    bool setTheme(const std::string& themeContent) {
        try {
            IRawTheme* rawTheme = parseRawTheme(themeContent);
            if (!rawTheme) {
                return false;
            }

            registry->setTheme(rawTheme, nullptr);
            delete rawTheme;
            return true;
        } catch (...) {
            return false;
        }
    }

    // Get color map from the current theme
    val getColorMap() {
        try {
            std::vector<std::string> colorMap = registry->getColorMap();
            val jsColorMap = val::array();
            for (size_t i = 0; i < colorMap.size(); i++) {
                jsColorMap.set(i, colorMap[i]);
            }
            return jsColorMap;
        } catch (...) {
            return val::array();
        }
    }
};

class GrammarWrapper {
private:
    Grammar* grammar;

    // Persisted buffers backing the typed_memory_view returned by tokenizeLines2().
    // They must outlive the embind call so JS can read the view; the JS wrapper copies
    // the data out before the next batch call reuses these buffers.
    std::vector<uint32_t> batchTokens2;
    std::vector<uint32_t> batchLineTokenCounts2;

    // Persisted buffers for the flat scope batch path (tokenizeLinesScopeFlat).
    std::vector<uint32_t> batchScopeFlat;
    std::vector<uint32_t> batchScopeLineTokenCounts;

    // Persisted buffer for the flat per-line scope path (tokenizeLineScopeFlat). Reused on
    // each call; the JS wrapper reconstructs tokens synchronously before the next call.
    std::vector<uint32_t> lineScopeFlat;

public:
    GrammarWrapper(uintptr_t grammarPtr) {
        grammar = reinterpret_cast<Grammar*>(grammarPtr);
    }

    // Tokenize a single line and return as JavaScript object
    val tokenizeLine(const std::string& lineText, val ruleStackVal) {
        StateStack* ruleStack = nullptr;

        // If ruleStack is provided, convert from pointer
        if (!ruleStackVal.isNull() && !ruleStackVal.isUndefined()) {
            uintptr_t stackPtr = ruleStackVal.as<uintptr_t>();
            ruleStack = reinterpret_cast<StateStack*>(stackPtr);
        }

        PerLineArena pa;
        ITokenizeLineResult result = grammar->tokenizeLine(lineText, ruleStack);
        pa.keep = result.ruleStack;

        // Build byte-offset to UTF-16 index map (JS strings are UTF-16)
        auto map = tml::buildByteToUtf16Map(lineText.c_str(), lineText.size());

        // Convert result to JavaScript object
        val jsResult = val::object();

        // Convert tokens
        val jsTokens = val::array();
        for (size_t i = 0; i < result.tokens.size(); i++) {
            const auto& token = result.tokens[i];
            val jsToken = val::object();
            jsToken.set("startIndex", tml::mapByteToUtf16(map, token.startIndex));
            jsToken.set("endIndex", tml::mapByteToUtf16(map, token.endIndex));

            val jsScopes = val::array();
            for (size_t j = 0; j < token.scopes.size(); j++) {
                jsScopes.set(j, token.scopes[j]);
            }
            jsToken.set("scopes", jsScopes);

            jsTokens.set(i, jsToken);
        }
        jsResult.set("tokens", jsTokens);

        // Return ruleStack pointer for next line
        jsResult.set("ruleStack", reinterpret_cast<uintptr_t>(result.ruleStack));

        return jsResult;
    }

    // Flat single-line scope tokenization. Same scope information as tokenizeLine() but encoded
    // as a flat numeric buffer plus a per-line interned scope dictionary, instead of one
    // val::object per token + a val::array of scope strings. Layout of `tokens` (flat uint32):
    // per token [startIndexUtf16, endIndexUtf16, scopeCount, scopeId0, ...]. `scopeNames[id]`
    // resolves a scope id to its string (only the unique scopes seen on this line cross the
    // boundary). The JS wrapper rebuilds the rich token objects far more cheaply than embind.
    val tokenizeLineScopeFlat(const std::string& lineText, val ruleStackVal) {
        StateStack* ruleStack = nullptr;
        if (!ruleStackVal.isNull() && !ruleStackVal.isUndefined()) {
            uintptr_t stackPtr = ruleStackVal.as<uintptr_t>();
            ruleStack = reinterpret_cast<StateStack*>(stackPtr);
        }

        PerLineArena pa;
        ITokenizeLineResult result = grammar->tokenizeLine(lineText, ruleStack);
        pa.keep = result.ruleStack;

        auto map = tml::buildByteToUtf16Map(lineText.c_str(), lineText.size());

        lineScopeFlat.clear();
        std::unordered_map<std::string, uint32_t> scopeIds;
        std::vector<std::string> scopeNames;

        for (const auto& token : result.tokens) {
            lineScopeFlat.push_back(static_cast<uint32_t>(tml::mapByteToUtf16(map, token.startIndex)));
            lineScopeFlat.push_back(static_cast<uint32_t>(tml::mapByteToUtf16(map, token.endIndex)));
            lineScopeFlat.push_back(static_cast<uint32_t>(token.scopes.size()));
            for (const auto& s : token.scopes) {
                auto it = scopeIds.find(s);
                uint32_t id;
                if (it == scopeIds.end()) {
                    id = static_cast<uint32_t>(scopeNames.size());
                    scopeIds.emplace(s, id);
                    scopeNames.push_back(s);
                } else {
                    id = it->second;
                }
                lineScopeFlat.push_back(id);
            }
        }

        val jsScopeNames = val::array();
        for (size_t i = 0; i < scopeNames.size(); i++) jsScopeNames.set(i, scopeNames[i]);

        val out = val::object();
        out.set("tokens", val(typed_memory_view(lineScopeFlat.size(), lineScopeFlat.data())));
        out.set("scopeNames", jsScopeNames);
        out.set("ruleStack", reinterpret_cast<uintptr_t>(result.ruleStack));
        return out;
    }

    // Tokenize with binary format
    val tokenizeLine2(const std::string& lineText, val ruleStackVal) {
        StateStack* ruleStack = nullptr;

        if (!ruleStackVal.isNull() && !ruleStackVal.isUndefined()) {
            uintptr_t stackPtr = ruleStackVal.as<uintptr_t>();
            ruleStack = reinterpret_cast<StateStack*>(stackPtr);
        }

        PerLineArena pa;
        ITokenizeLineResult2 result = grammar->tokenizeLine2(lineText, ruleStack);
        pa.keep = result.ruleStack;

        // Build byte-offset to UTF-16 index map (JS strings are UTF-16)
        auto map = tml::buildByteToUtf16Map(lineText.c_str(), lineText.size());

        val jsResult = val::object();

        // Convert tokens (Uint32Array-like)
        // Encoded tokens are pairs: [startIndex, metadata, startIndex, metadata, ...]
        val jsTokens = val::array();
        for (size_t i = 0; i < result.tokens.size(); i++) {
            if (i % 2 == 0) {
                // Even indices are start offsets — convert to UTF-16
                jsTokens.set(i, static_cast<uint32_t>(tml::mapByteToUtf16(map, static_cast<int32_t>(result.tokens[i]))));
            } else {
                // Odd indices are metadata — pass through
                jsTokens.set(i, result.tokens[i]);
            }
        }
        jsResult.set("tokens", jsTokens);

        jsResult.set("ruleStack", reinterpret_cast<uintptr_t>(result.ruleStack));

        return jsResult;
    }

    // Tokenize a whole document in one call, carrying the rule stack across lines
    // internally. This avoids the per-line JS<->WASM round trip that dominates the
    // line-by-line API — the only cost is marshalling the strings in and the tokens out.
    // Returns an array (one entry per line), each entry an array of token objects.
    val tokenizeLines(val linesArray) {
        std::vector<std::string> lines = vecFromJSArray<std::string>(linesArray);

        BatchArena ba;
        val jsLines = val::array();
        StateStack* ruleStack = nullptr;
        for (size_t li = 0; li < lines.size(); li++) {
            const std::string& lineText = lines[li];
            ITokenizeLineResult result = grammar->tokenizeLine(lineText, ruleStack);
            ruleStack = result.ruleStack;

            auto map = tml::buildByteToUtf16Map(lineText.c_str(), lineText.size());

            val jsTokens = val::array();
            for (size_t i = 0; i < result.tokens.size(); i++) {
                const auto& token = result.tokens[i];
                val jsToken = val::object();
                jsToken.set("startIndex", tml::mapByteToUtf16(map, token.startIndex));
                jsToken.set("endIndex", tml::mapByteToUtf16(map, token.endIndex));

                val jsScopes = val::array();
                for (size_t j = 0; j < token.scopes.size(); j++) {
                    jsScopes.set(j, token.scopes[j]);
                }
                jsToken.set("scopes", jsScopes);

                jsTokens.set(i, jsToken);
            }
            jsLines.set(li, jsTokens);
        }
        return jsLines;
    }

    // Themed (binary) whole-document tokenization. All encoded tokens for the document
    // are packed into a single contiguous buffer and returned as one typed-array view,
    // eliminating both the per-line call overhead and the per-token val marshalling of
    // tokenizeLine2(). `lineTokenCounts[i]` is the number of uint32 entries (2 per token)
    // produced for line i, so callers can slice `tokens` back into per-line ranges.
    val tokenizeLines2(val linesArray) {
        std::vector<std::string> lines = vecFromJSArray<std::string>(linesArray);

        BatchArena ba;
        batchTokens2.clear();
        batchLineTokenCounts2.clear();
        batchLineTokenCounts2.reserve(lines.size());

        StateStack* ruleStack = nullptr;
        for (size_t li = 0; li < lines.size(); li++) {
            const std::string& lineText = lines[li];
            ITokenizeLineResult2 result = grammar->tokenizeLine2(lineText, ruleStack);
            ruleStack = result.ruleStack;

            auto map = tml::buildByteToUtf16Map(lineText.c_str(), lineText.size());

            const size_t count = result.tokens.size();
            batchLineTokenCounts2.push_back(static_cast<uint32_t>(count));
            for (size_t i = 0; i < count; i++) {
                if ((i & 1) == 0) {
                    // Even indices are start offsets — convert byte offset to UTF-16.
                    batchTokens2.push_back(static_cast<uint32_t>(
                        tml::mapByteToUtf16(map, static_cast<int32_t>(result.tokens[i]))));
                } else {
                    // Odd indices are metadata — pass through.
                    batchTokens2.push_back(static_cast<uint32_t>(result.tokens[i]));
                }
            }
        }

        val out = val::object();
        out.set("tokens", val(typed_memory_view(batchTokens2.size(), batchTokens2.data())));
        out.set("lineTokenCounts",
                val(typed_memory_view(batchLineTokenCounts2.size(), batchLineTokenCounts2.data())));
        return out;
    }

    // Flat scope batch: same scope-name information as tokenizeLines(), but encoded as a
    // single numeric buffer plus an interned string dictionary, instead of one val::object
    // per token and one val::array of strings per token. Layout of `tokens` (flat uint32):
    // per token [startIndexUtf16, endIndexUtf16, scopeCount, scopeId0, scopeId1, ...].
    // `lineTokenCounts[i]` is the number of tokens on line i; `scopeNames[id]` resolves a
    // scope id back to its string. The numeric buffer crosses as one typed_memory_view and
    // the unique scope strings cross once — eliminating the per-token/per-scope marshalling.
    val tokenizeLinesScopeFlat(val linesArray) {
        std::vector<std::string> lines = vecFromJSArray<std::string>(linesArray);

        BatchArena ba;
        batchScopeFlat.clear();
        batchScopeLineTokenCounts.clear();
        batchScopeLineTokenCounts.reserve(lines.size());

        std::unordered_map<std::string, uint32_t> scopeIds;
        std::vector<std::string> scopeNames;

        StateStack* ruleStack = nullptr;
        for (size_t li = 0; li < lines.size(); li++) {
            const std::string& lineText = lines[li];
            ITokenizeLineResult result = grammar->tokenizeLine(lineText, ruleStack);
            ruleStack = result.ruleStack;

            auto map = tml::buildByteToUtf16Map(lineText.c_str(), lineText.size());

            batchScopeLineTokenCounts.push_back(static_cast<uint32_t>(result.tokens.size()));
            for (const auto& token : result.tokens) {
                batchScopeFlat.push_back(static_cast<uint32_t>(tml::mapByteToUtf16(map, token.startIndex)));
                batchScopeFlat.push_back(static_cast<uint32_t>(tml::mapByteToUtf16(map, token.endIndex)));
                batchScopeFlat.push_back(static_cast<uint32_t>(token.scopes.size()));
                for (const auto& s : token.scopes) {
                    auto it = scopeIds.find(s);
                    uint32_t id;
                    if (it == scopeIds.end()) {
                        id = static_cast<uint32_t>(scopeNames.size());
                        scopeIds.emplace(s, id);
                        scopeNames.push_back(s);
                    } else {
                        id = it->second;
                    }
                    batchScopeFlat.push_back(id);
                }
            }
        }

        val jsScopeNames = val::array();
        for (size_t i = 0; i < scopeNames.size(); i++) jsScopeNames.set(i, scopeNames[i]);

        val out = val::object();
        out.set("tokens", val(typed_memory_view(batchScopeFlat.size(), batchScopeFlat.data())));
        out.set("lineTokenCounts",
                val(typed_memory_view(batchScopeLineTokenCounts.size(), batchScopeLineTokenCounts.data())));
        out.set("scopeNames", jsScopeNames);
        return out;
    }

    std::string getScopeName() const {
        return grammar->getScopeName();
    }
};

// Embind declarations
EMSCRIPTEN_BINDINGS(tml) {
    class_<RegistryWrapper>("Registry")
        .constructor<>()
        .function("loadGrammarFromContent", &RegistryWrapper::loadGrammarFromContent)
        .function("setTheme", &RegistryWrapper::setTheme)
        .function("getColorMap", &RegistryWrapper::getColorMap);

    class_<GrammarWrapper>("Grammar")
        .constructor<uintptr_t>()
        .function("tokenizeLine", &GrammarWrapper::tokenizeLine)
        .function("tokenizeLineScopeFlat", &GrammarWrapper::tokenizeLineScopeFlat)
        .function("tokenizeLine2", &GrammarWrapper::tokenizeLine2)
        .function("tokenizeLines", &GrammarWrapper::tokenizeLines)
        .function("tokenizeLines2", &GrammarWrapper::tokenizeLines2)
        .function("tokenizeLinesScopeFlat", &GrammarWrapper::tokenizeLinesScopeFlat)
        .function("getScopeName", &GrammarWrapper::getScopeName);
}
