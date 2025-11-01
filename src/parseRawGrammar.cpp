#include "parseRawGrammar.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include <stdexcept>

using namespace rapidjson;

namespace vscode_textmate {

// Helper function to get string from JSON value
static std::string* getStringPtr(const Value& val) {
    if (val.IsString()) {
        return new std::string(val.GetString(), val.GetStringLength());
    }
    return nullptr;
}

// Forward declarations
static IRawRule* parseRule(const Value& ruleObj);
static std::vector<IRawRule*>* parsePatterns(const Value& patternsArray);

// Helper function to parse captures
static IRawCaptures* parseCaptures(const Value& capturesObj) {
    if (!capturesObj.IsObject()) {
        return nullptr;
    }

    IRawCaptures* captures = new IRawCaptures();
    for (auto it = capturesObj.MemberBegin(); it != capturesObj.MemberEnd(); ++it) {
        std::string key(it->name.GetString(), it->name.GetStringLength());
        IRawRule* rule = new IRawRule();

        if (it->value.IsObject()) {
            const Value& ruleObj = it->value;

            if (ruleObj.HasMember("name") && ruleObj["name"].IsString()) {
                rule->name = getStringPtr(ruleObj["name"]);
            }
            if (ruleObj.HasMember("contentName") && ruleObj["contentName"].IsString()) {
                rule->contentName = getStringPtr(ruleObj["contentName"]);
            }
            if (ruleObj.HasMember("patterns") && ruleObj["patterns"].IsArray()) {
                // Parse patterns for this capture
                rule->patterns = parsePatterns(ruleObj["patterns"]);
            }
        }

        captures->captures[key] = rule;
    }

    return captures;
}

// Helper function to parse patterns array
static std::vector<IRawRule*>* parsePatterns(const Value& patternsArray) {
    if (!patternsArray.IsArray()) {
        return nullptr;
    }

    std::vector<IRawRule*>* patterns = new std::vector<IRawRule*>();
    for (SizeType i = 0; i < patternsArray.Size(); i++) {
        const Value& patternObj = patternsArray[i];
        if (patternObj.IsObject()) {
            patterns->push_back(parseRule(patternObj));
        }
    }

    return patterns;
}

// Helper function to parse repository
static IRawRepository* parseRepository(const Value& repoObj) {
    if (!repoObj.IsObject()) {
        return nullptr;
    }

    IRawRepository* repository = new IRawRepository();

    for (auto it = repoObj.MemberBegin(); it != repoObj.MemberEnd(); ++it) {
        std::string key(it->name.GetString(), it->name.GetStringLength());

        if (key == "$self") {
            if (it->value.IsObject()) {
                repository->selfRule = parseRule(it->value);
            }
        } else if (key == "$base") {
            if (it->value.IsObject()) {
                repository->baseRule = parseRule(it->value);
            }
        } else {
            if (it->value.IsObject()) {
                repository->rules[key] = parseRule(it->value);
            }
        }
    }

    return repository;
}

// Parse a single rule
static IRawRule* parseRule(const Value& ruleObj) {
    if (!ruleObj.IsObject()) {
        return nullptr;
    }

    IRawRule* rule = new IRawRule();

    // Parse include
    if (ruleObj.HasMember("include") && ruleObj["include"].IsString()) {
        rule->include = getStringPtr(ruleObj["include"]);
    }

    // Parse name
    if (ruleObj.HasMember("name") && ruleObj["name"].IsString()) {
        rule->name = getStringPtr(ruleObj["name"]);
    }

    // Parse contentName
    if (ruleObj.HasMember("contentName") && ruleObj["contentName"].IsString()) {
        rule->contentName = getStringPtr(ruleObj["contentName"]);
    }

    // Parse match
    if (ruleObj.HasMember("match") && ruleObj["match"].IsString()) {
        rule->match = getStringPtr(ruleObj["match"]);
    }

    // Parse captures
    if (ruleObj.HasMember("captures") && ruleObj["captures"].IsObject()) {
        rule->captures = parseCaptures(ruleObj["captures"]);
    }

    // Parse begin
    if (ruleObj.HasMember("begin") && ruleObj["begin"].IsString()) {
        rule->begin = getStringPtr(ruleObj["begin"]);
    }

    // Parse beginCaptures
    if (ruleObj.HasMember("beginCaptures") && ruleObj["beginCaptures"].IsObject()) {
        rule->beginCaptures = parseCaptures(ruleObj["beginCaptures"]);
    }

    // Parse end
    if (ruleObj.HasMember("end") && ruleObj["end"].IsString()) {
        rule->end = getStringPtr(ruleObj["end"]);
    }

    // Parse endCaptures
    if (ruleObj.HasMember("endCaptures") && ruleObj["endCaptures"].IsObject()) {
        rule->endCaptures = parseCaptures(ruleObj["endCaptures"]);
    }

    // Parse while
    if (ruleObj.HasMember("while") && ruleObj["while"].IsString()) {
        rule->whilePattern = getStringPtr(ruleObj["while"]);
    }

    // Parse whileCaptures
    if (ruleObj.HasMember("whileCaptures") && ruleObj["whileCaptures"].IsObject()) {
        rule->whileCaptures = parseCaptures(ruleObj["whileCaptures"]);
    }

    // Parse patterns
    if (ruleObj.HasMember("patterns") && ruleObj["patterns"].IsArray()) {
        rule->patterns = parsePatterns(ruleObj["patterns"]);
    }

    // Parse repository
    if (ruleObj.HasMember("repository") && ruleObj["repository"].IsObject()) {
        rule->repository = parseRepository(ruleObj["repository"]);
    }

    // Parse applyEndPatternLast
    if (ruleObj.HasMember("applyEndPatternLast") && ruleObj["applyEndPatternLast"].IsBool()) {
        rule->applyEndPatternLast = new bool(ruleObj["applyEndPatternLast"].GetBool());
    }

    return rule;
}

// Parse JSON grammar
IRawGrammar* parseJSONGrammar(const std::string& content, const std::string* filename) {
    Document doc;

    // Parse JSON document
    ParseResult result = doc.Parse(content.c_str());
    if (!result) {
        std::string error = "JSON parse error: ";
        error += GetParseError_En(result.Code());
        error += " at offset " + std::to_string(result.Offset());
        throw std::runtime_error(error);
    }

    if (!doc.IsObject()) {
        throw std::runtime_error("Grammar JSON must be an object");
    }

    IRawGrammar* grammar = new IRawGrammar();

    // Parse scopeName (required)
    if (doc.HasMember("scopeName") && doc["scopeName"].IsString()) {
        grammar->scopeName = std::string(doc["scopeName"].GetString(),
                                         doc["scopeName"].GetStringLength());
    } else {
        delete grammar;
        throw std::runtime_error("Grammar must have a scopeName");
    }

    // Parse patterns (required)
    if (doc.HasMember("patterns") && doc["patterns"].IsArray()) {
        const Value& patternsArray = doc["patterns"];
        // IRawGrammar now inherits from IRawRule, so patterns is a pointer
        grammar->patterns = new std::vector<IRawRule*>();
        for (SizeType i = 0; i < patternsArray.Size(); i++) {
            if (patternsArray[i].IsObject()) {
                grammar->patterns->push_back(parseRule(patternsArray[i]));
            }
        }
    }

    // Parse repository
    if (doc.HasMember("repository") && doc["repository"].IsObject()) {
        grammar->repository = parseRepository(doc["repository"]);
    }

    // Parse injections
    if (doc.HasMember("injections") && doc["injections"].IsObject()) {
        grammar->injections = new std::map<std::string, IRawRule*>();
        const Value& injectionsObj = doc["injections"];

        for (auto it = injectionsObj.MemberBegin(); it != injectionsObj.MemberEnd(); ++it) {
            std::string key(it->name.GetString(), it->name.GetStringLength());
            if (it->value.IsObject()) {
                (*grammar->injections)[key] = parseRule(it->value);
            }
        }
    }

    // Parse injectionSelector
    if (doc.HasMember("injectionSelector") && doc["injectionSelector"].IsString()) {
        grammar->injectionSelector = getStringPtr(doc["injectionSelector"]);
    }

    // Parse fileTypes
    if (doc.HasMember("fileTypes") && doc["fileTypes"].IsArray()) {
        grammar->fileTypes = new std::vector<std::string>();
        const Value& fileTypesArray = doc["fileTypes"];
        for (SizeType i = 0; i < fileTypesArray.Size(); i++) {
            if (fileTypesArray[i].IsString()) {
                grammar->fileTypes->push_back(
                    std::string(fileTypesArray[i].GetString(),
                               fileTypesArray[i].GetStringLength())
                );
            }
        }
    }

    // Parse name
    if (doc.HasMember("name") && doc["name"].IsString()) {
        grammar->name = getStringPtr(doc["name"]);
    }

    // Parse firstLineMatch
    if (doc.HasMember("firstLineMatch") && doc["firstLineMatch"].IsString()) {
        grammar->firstLineMatch = getStringPtr(doc["firstLineMatch"]);
    }

    return grammar;
}

// Parse raw grammar (determines format and calls appropriate parser)
IRawGrammar* parseRawGrammar(const std::string& content, const std::string* filePath) {
    // For now, we only support JSON format
    // PLIST support could be added later if needed

    bool isJSON = true;
    if (filePath != nullptr) {
        // Check file extension
        size_t dotPos = filePath->find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string ext = filePath->substr(dotPos);
            isJSON = (ext == ".json");
        }
    }

    if (isJSON) {
        return parseJSONGrammar(content, filePath);
    } else {
        throw std::runtime_error("Only JSON grammar format is currently supported");
    }
}

} // namespace vscode_textmate
