// Native C++ raw-engine reference benchmark for TextMateLib.
//
// Measures scope and themed tokenization throughput on the same fixture files
// + tm-grammars as bench-js.mjs. This is a RAW reference — it runs in-process
// with no JS<->WASM marshalling.
//
// Usage: bench-native [--theme <theme.json>] <scope> <grammar.json> <fixture> [...]
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <map>
#include "tml.h"
#include "parseRawTheme.h"

using namespace tml;

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Failed to open: " + path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static std::vector<std::string> splitLines(const std::string& content) {
    std::vector<std::string> lines;
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
    return lines;
}

static long long tokenizePass(Grammar* grammar, const std::vector<std::string>& lines, long long& tokenCount) {
    auto start = std::chrono::high_resolution_clock::now();
    StateStack* state = const_cast<StateStack*>(INITIAL);
    tokenCount = 0;
    for (const auto& line : lines) {
        auto result = grammar->tokenizeLine(line, state);
        state = result.ruleStack;
        tokenCount += result.tokens.size();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

static long long tokenizePass2(Grammar* grammar, const std::vector<std::string>& lines, long long& tokenCount) {
    auto start = std::chrono::high_resolution_clock::now();
    StateStack* state = const_cast<StateStack*>(INITIAL);
    tokenCount = 0;
    for (const auto& line : lines) {
        auto result = grammar->tokenizeLine2(line, state);
        state = result.ruleStack;
        tokenCount += result.tokens.size() / 2;
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

struct Row {
    std::string name;
    double mb;
    double mbps;
    long long tokens;
};

int main(int argc, char** argv) {
    const int warmup = 3;
    const int runs = 7;

    std::string themePath;
    int argStart = 1;
    if (argc >= 3 && std::string(argv[1]) == "--theme") {
        themePath = argv[2];
        argStart = 3;
    }

    int remaining = argc - argStart;
    if (remaining < 3 || remaining % 3 != 0) {
        std::cerr << "Usage: bench-native [--theme <theme.json>] <scope> <grammar.json> <fixture> [...]" << std::endl;
        return 1;
    }

    IOnigLib* onigLib = new DefaultOnigLib();
    std::vector<Row> scopeRows;
    std::vector<Row> themedRows;

    for (int i = argStart; i < argc; i += 3) {
        std::string scope = argv[i];
        std::string grammarPath = argv[i + 1];
        std::string fixturePath = argv[i + 2];

        try {
            std::string content = readFile(fixturePath);
            auto lines = splitLines(content);
            double mb = content.size() / 1024.0 / 1024.0;

            IRawGrammar* rawGrammar = parseJSONGrammar(readFile(grammarPath), nullptr);
            RegistryOptions options;
            options.onigLib = onigLib;
            std::map<std::string, IRawGrammar*> grammarMap;
            grammarMap[scope] = rawGrammar;
            options.loadGrammar = [&grammarMap](const std::string& s) -> IRawGrammar* {
                auto it = grammarMap.find(s);
                return it != grammarMap.end() ? it->second : nullptr;
            };
            Registry registry(options);
            std::vector<std::string> emptyDeps;
            registry.addGrammar(rawGrammar, emptyDeps, 0, nullptr);

            if (!themePath.empty()) {
                IRawTheme* rawTheme = parseRawTheme(readFile(themePath));
                if (rawTheme) registry.setTheme(rawTheme);
            }

            Grammar* grammar = registry.loadGrammar(scope);
            if (!grammar) {
                std::cerr << "FAILED to load grammar for " << scope << std::endl;
                continue;
            }

            // Scope pass
            {
                long long dummy;
                for (int w = 0; w < warmup; w++) tokenizePass(grammar, lines, dummy);
                std::vector<long long> samples;
                long long tokens = 0;
                for (int r = 0; r < runs; r++) samples.push_back(tokenizePass(grammar, lines, tokens));
                std::sort(samples.begin(), samples.end());
                long long medianNs = samples[samples.size() / 2];
                double mbps = mb / (medianNs / 1e9);
                scopeRows.push_back({fixturePath, mb, mbps, tokens});
                std::cerr << "  " << scope << " scope ok (" << mbps << " MB/s)" << std::endl;
            }

            // Themed pass (only if theme was loaded)
            if (!themePath.empty()) {
                long long dummy;
                for (int w = 0; w < warmup; w++) tokenizePass2(grammar, lines, dummy);
                std::vector<long long> samples;
                long long tokens = 0;
                for (int r = 0; r < runs; r++) samples.push_back(tokenizePass2(grammar, lines, tokens));
                std::sort(samples.begin(), samples.end());
                long long medianNs = samples[samples.size() / 2];
                double mbps = mb / (medianNs / 1e9);
                themedRows.push_back({fixturePath, mb, mbps, tokens});
                std::cerr << "  " << scope << " themed ok (" << mbps << " MB/s)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "  ERROR " << scope << ": " << e.what() << std::endl;
        }
    }
    delete onigLib;

    std::cout << "| File | Size | tml native C++ (scope) |" << std::endl;
    std::cout << "|------|------|------------------------|" << std::endl;
    for (const auto& r : scopeRows) {
        std::string base = r.name.substr(r.name.find_last_of("/\\") + 1);
        printf("| %s | %.2f MB | **%.1f MB/s** |\n", base.c_str(), r.mb, r.mbps);
    }

    if (!themedRows.empty()) {
        std::cout << std::endl;
        std::cout << "| File | Size | tml native C++ (themed) |" << std::endl;
        std::cout << "|------|------|-------------------------|" << std::endl;
        for (const auto& r : themedRows) {
            std::string base = r.name.substr(r.name.find_last_of("/\\") + 1);
            printf("| %s | %.2f MB | **%.1f MB/s** |\n", base.c_str(), r.mb, r.mbps);
        }
    }
    return 0;
}
