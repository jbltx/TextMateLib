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

using namespace emscripten;
using namespace tml;

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

        ITokenizeLineResult result = grammar->tokenizeLine(lineText, ruleStack);

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

    // Tokenize with binary format
    val tokenizeLine2(const std::string& lineText, val ruleStackVal) {
        StateStack* ruleStack = nullptr;

        if (!ruleStackVal.isNull() && !ruleStackVal.isUndefined()) {
            uintptr_t stackPtr = ruleStackVal.as<uintptr_t>();
            ruleStack = reinterpret_cast<StateStack*>(stackPtr);
        }

        ITokenizeLineResult2 result = grammar->tokenizeLine2(lineText, ruleStack);

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
        .function("tokenizeLine2", &GrammarWrapper::tokenizeLine2)
        .function("getScopeName", &GrammarWrapper::getScopeName);
}
