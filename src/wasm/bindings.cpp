#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "main.h"
#include "registry.h"
#include "grammar.h"
#include "parseRawGrammar.h"
#include "onigLib.h"
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

        // Convert result to JavaScript object
        val jsResult = val::object();

        // Convert tokens
        val jsTokens = val::array();
        for (size_t i = 0; i < result.tokens.size(); i++) {
            const auto& token = result.tokens[i];
            val jsToken = val::object();
            jsToken.set("startIndex", token.startIndex);
            jsToken.set("endIndex", token.endIndex);

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

        val jsResult = val::object();

        // Convert tokens (Uint32Array-like)
        val jsTokens = val::array();
        for (size_t i = 0; i < result.tokens.size(); i++) {
            jsTokens.set(i, result.tokens[i]);
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
        .function("loadGrammarFromContent", &RegistryWrapper::loadGrammarFromContent);

    class_<GrammarWrapper>("Grammar")
        .constructor<uintptr_t>()
        .function("tokenizeLine", &GrammarWrapper::tokenizeLine)
        .function("tokenizeLine2", &GrammarWrapper::tokenizeLine2)
        .function("getScopeName", &GrammarWrapper::getScopeName);
}
