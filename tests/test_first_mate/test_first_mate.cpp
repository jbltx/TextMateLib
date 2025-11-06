#include "../src/main.h"
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

using namespace tml;
using namespace rapidjson;

// Helper function to read file content
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to parse JSON file
Document parseJSONFile(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    IStreamWrapper isw(ifs);
    Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError()) {
        throw std::runtime_error("JSON parse error in file: " + filename);
    }

    return doc;
}

// Convert JSON array to vector of strings (for scopes)
std::vector<std::string> jsonArrayToStringVector(const Value& arr) {
    std::vector<std::string> result;
    if (!arr.IsArray()) return result;

    for (SizeType i = 0; i < arr.Size(); i++) {
        if (arr[i].IsString()) {
            result.push_back(arr[i].GetString());
        }
    }
    return result;
}

// Compare two scope arrays
bool compareScopesArrays(const std::vector<std::string>& expected, const std::vector<std::string>& actual) {
    if (expected.size() != actual.size()) {
        return false;
    }

    for (size_t i = 0; i < expected.size(); i++) {
        if (expected[i] != actual[i]) {
            return false;
        }
    }

    return true;
}

// Grammar holder - loads and holds grammars for a test
class GrammarHolder {
private:
    std::string basePath;
    // Map from scope name to raw grammar (owned by this holder)
    std::map<std::string, IRawGrammar*> grammarByScope;

public:
    GrammarHolder(const std::string& path) : basePath(path) {}

    ~GrammarHolder() {
        // Clean up all loaded grammars
        for (auto& pair : grammarByScope) {
            delete pair.second;
        }
    }

    // Load a grammar file and add to the map
    std::string loadGrammar(const std::string& grammarPath) {
        try {
            std::string fullPath = basePath + "/" + grammarPath;
            std::string grammarContent = readFile(fullPath);

            // Only JSON grammars are supported in C++ implementation
            if (grammarPath.find(".json") != std::string::npos) {
                IRawGrammar* grammar = parseJSONGrammar(grammarContent, nullptr);
                if (grammar) {
                    std::string scopeName = grammar->scopeName;
                    grammarByScope[scopeName] = grammar;
                    return scopeName;
                }
            } else {
                std::cerr << "Warning: Only JSON grammars are supported, skipping " << grammarPath << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading grammar " << grammarPath << ": " << e.what() << std::endl;
        }
        return "";
    }

    // Get grammar by scope name (called by Registry loadGrammar callback)
    IRawGrammar* getGrammarByScope(const std::string& scopeName) {
        auto it = grammarByScope.find(scopeName);
        if (it != grammarByScope.end()) {
            return it->second;
        }
        return nullptr;
    }

    void loadGrammars(const Value& grammarList) {
        if (!grammarList.IsArray()) return;

        for (SizeType i = 0; i < grammarList.Size(); i++) {
            if (grammarList[i].IsString()) {
                loadGrammar(grammarList[i].GetString());
            }
        }
    }
};

// Test a single line
bool testLine(Grammar* grammar,
              const std::string& line,
              const Value& expectedTokens,
              StateStack* prevState,
              StateStack** outState,
              int& tokensChecked) {

    ITokenizeLineResult result = grammar->tokenizeLine(line, prevState);

    if (outState) {
        *outState = result.ruleStack;
    }

    // Build a vector of expected tokens (filtering out empty tokens like TypeScript does)
    std::vector<std::pair<std::string, std::vector<std::string>>> expectedTokensList;

    for (SizeType i = 0; i < expectedTokens.Size(); i++) {
        const Value& token = expectedTokens[i];
        if (token.HasMember("value") && token.HasMember("scopes")) {
            std::string value = token["value"].GetString();
            // Skip empty tokens if line is non-empty (matching TypeScript behavior)
            if (line.length() > 0 && value.length() == 0) {
                continue;
            }
            std::vector<std::string> scopes = jsonArrayToStringVector(token["scopes"]);
            expectedTokensList.push_back(std::make_pair(value, scopes));
        }
    }

    // Compare tokens in order (like TypeScript's deepStrictEqual)
    if (result.tokens.size() != expectedTokensList.size()) {
        std::cerr << "      Token count mismatch" << std::endl;
        std::cerr << "        Expected: " << expectedTokensList.size() << " tokens" << std::endl;
        std::cerr << "        Actual: " << result.tokens.size() << " tokens" << std::endl;
        std::cerr << "      Actual tokens:" << std::endl;
        for (size_t i = 0; i < result.tokens.size(); i++) {
            const auto& token = result.tokens[i];
            std::string tokenText = line.substr(token.startIndex, token.endIndex - token.startIndex);
            std::cerr << "        [" << i << "] \"" << tokenText << "\" scopes: ";
            for (const auto& scope : token.scopes) {
                std::cerr << scope << " ";
            }
            std::cerr << std::endl;
        }
        return false;
    }

    for (size_t i = 0; i < result.tokens.size(); i++) {
        const auto& token = result.tokens[i];
        const auto& expected = expectedTokensList[i];

        std::string tokenText = line.substr(token.startIndex, token.endIndex - token.startIndex);

        // Check token value matches
        if (tokenText != expected.first) {
            std::cerr << "      Token " << i << " value mismatch" << std::endl;
            std::cerr << "        Expected: \"" << expected.first << "\"" << std::endl;
            std::cerr << "        Actual: \"" << tokenText << "\"" << std::endl;
            return false;
        }

        // Check token scopes match
        if (!compareScopesArrays(expected.second, token.scopes)) {
            std::cerr << "      Token " << i << " \"" << tokenText << "\" has wrong scopes" << std::endl;
            std::cerr << "        Expected: ";
            for (const auto& scope : expected.second) {
                std::cerr << scope << " ";
            }
            std::cerr << std::endl;
            std::cerr << "        Actual: ";
            for (const auto& scope : token.scopes) {
                std::cerr << scope << " ";
            }
            std::cerr << std::endl;
            return false;
        }

        tokensChecked++;
    }

    return true;
}

// Run a single test case
bool runTestCase(const Value& testCase, GrammarHolder& holder, IOnigLib* onigLib, int testNum) {
    std::string desc = testCase.HasMember("desc") ? testCase["desc"].GetString() : "Unknown";

    std::cout << "Running " << desc << "..." << std::endl;

    try {
        // Load ALL grammars from the test (matching TypeScript implementation lines 49-56)
        std::string mainGrammarScope;

        if (testCase.HasMember("grammars")) {
            const Value& grammars = testCase["grammars"];
            for (SizeType i = 0; i < grammars.Size(); i++) {
                if (grammars[i].IsString()) {
                    std::string grammarPath = grammars[i].GetString();
                    std::string scopeName = holder.loadGrammar(grammarPath);

                    // Check if this is the main grammar
                    if (mainGrammarScope.empty() && testCase.HasMember("grammarPath")) {
                        if (grammarPath == testCase["grammarPath"].GetString()) {
                            mainGrammarScope = scopeName;
                        }
                    }
                }
            }
        }

        // If grammarScopeName is specified, use it
        if (testCase.HasMember("grammarScopeName")) {
            mainGrammarScope = testCase["grammarScopeName"].GetString();
        }

        if (mainGrammarScope.empty()) {
            std::cerr << "  FAILED: No main grammar scope name" << std::endl;
            return false;
        }

        std::cout << "  Main grammar scope: " << mainGrammarScope << std::endl;

        // Collect grammar injections if specified
        std::vector<std::string> grammarInjectionsList;
        if (testCase.HasMember("grammarInjections")) {
            const Value& injections = testCase["grammarInjections"];
            if (injections.IsArray()) {
                for (SizeType i = 0; i < injections.Size(); i++) {
                    if (injections[i].IsString()) {
                        grammarInjectionsList.push_back(injections[i].GetString());
                    }
                }
            }
        }

        // Create registry with callback that returns from pre-loaded map (matching TS line 63)
        std::cout << "  Creating registry..." << std::endl;
        RegistryOptions options;
        options.onigLib = onigLib;
        options.loadGrammar = [&holder](const ScopeName& scopeName) -> IRawGrammar* {
            return holder.getGrammarByScope(scopeName);
        };

        // Configure getInjections callback (matching TS lines 64-68)
        options.getInjections = [mainGrammarScope, grammarInjectionsList](const ScopeName& scopeName) -> std::vector<ScopeName> {
            if (scopeName == mainGrammarScope) {
                return grammarInjectionsList;
            }
            return std::vector<ScopeName>();
        };

        Registry registry(options);
        std::cout << "  Registry created" << std::endl;

        // Load the main grammar through the registry (matching TS line 71)
        std::cout << "  Loading grammar through registry..." << std::endl;
        Grammar* grammar = registry.loadGrammar(mainGrammarScope);
        std::cout << "  Grammar loaded" << std::endl;

        if (!grammar) {
            std::cerr << "  FAILED: Could not create grammar object" << std::endl;
            return false;
        }

        // Test each line
        if (!testCase.HasMember("lines")) {
            std::cerr << "  FAILED: No lines in test case" << std::endl;
            return false;
        }

        const Value& lines = testCase["lines"];
        StateStack* prevState = nullptr;
        int totalTokensChecked = 0;

        std::cout << "  Testing " << lines.Size() << " lines..." << std::endl;

        for (SizeType i = 0; i < lines.Size(); i++) {
            const Value& lineTest = lines[i];

            if (!lineTest.HasMember("line") || !lineTest.HasMember("tokens")) {
                std::cerr << "  FAILED: Invalid line test at index " << i << std::endl;
                return false;
            }

            std::string line = lineTest["line"].GetString();
            const Value& expectedTokens = lineTest["tokens"];

            std::cout << "    Line " << i << ": \"" << line << "\"" << std::endl;

            StateStack* newState = nullptr;
            int tokensChecked = 0;

            if (!testLine(grammar, line, expectedTokens, prevState, &newState, tokensChecked)) {
                std::cerr << "  FAILED at line " << i << ": \"" << line << "\"" << std::endl;
                return false;
            }

            totalTokensChecked += tokensChecked;
            prevState = newState;
        }

        std::cout << "  PASSED (" << totalTokensChecked << " tokens checked)" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "  FAILED with exception: " << e.what() << std::endl;
        return false;
    }
}

// Helper to get tests.json path
std::string getTestsJsonPath() {
    return "./tests.json";
}

// Global test data
static Document* g_testsDoc = nullptr;
static IOnigLib* g_onigLib = nullptr;

// Initialize test data
void initializeTestData() {
    if (!g_testsDoc) {
        try {
            std::string testsJsonPath = getTestsJsonPath();
            g_testsDoc = new Document();
            *g_testsDoc = parseJSONFile(testsJsonPath);
            g_onigLib = new DefaultOnigLib();
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize test data: " << e.what() << std::endl;
        }
    }
}

// GTest fixture for First-Mate tests
class FirstMateTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        initializeTestData();
    }

    static void TearDownTestSuite() {
        if (g_onigLib) {
            delete g_onigLib;
            g_onigLib = nullptr;
        }
        if (g_testsDoc) {
            delete g_testsDoc;
            g_testsDoc = nullptr;
        }
    }
};

// Parameterized test class
class FirstMateParameterizedTest : public FirstMateTest,
                                   public ::testing::WithParamInterface<int> {
};

// Define parameterized tests
TEST_P(FirstMateParameterizedTest, FirstMateTests) {
    int testIndex = GetParam();
    ASSERT_NE(g_testsDoc, nullptr) << "Test data not initialized";
    ASSERT_LT(testIndex, static_cast<int>(g_testsDoc->Size()));

    const Value& testCase = (*g_testsDoc)[testIndex];
    GrammarHolder holder(".");

    bool result = runTestCase(testCase, holder, g_onigLib, testIndex + 1);
    EXPECT_TRUE(result) << "Test case " << testIndex + 1 << " failed";
}

// Instantiate parameterized tests - will auto-detect from tests.json
INSTANTIATE_TEST_CASE_P(
    FirstMateTestSuite,
    FirstMateParameterizedTest,
    ::testing::Range(0, 65));  // Adjust if test count changes

// Initialize test data at program start
namespace {
struct TestInitializer {
    TestInitializer() {
        initializeTestData();
    }
} test_initializer;
}
