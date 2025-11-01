#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <map>
#include "../src/main.h"
#include "../src/parseRawGrammar.h"

using namespace vscode_textmate;

struct BenchmarkCase {
    std::string name;
    std::string filePath;
    std::string grammarPath;
    std::string scopeName;
};

struct BenchmarkResult {
    std::string testName;
    int lineCount;
    int tokenCount;
    int charCount;
    long long elapsedMs;

    double linesPerSecond() const {
        return lineCount / (elapsedMs / 1000.0);
    }

    double tokensPerSecond() const {
        return tokenCount / (elapsedMs / 1000.0);
    }

    double mbPerSecond() const {
        return (charCount / 1024.0 / 1024.0) / (elapsedMs / 1000.0);
    }
};

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> splitLines(const std::string& content) {
    std::vector<std::string> lines;
    std::stringstream ss(content);
    std::string line;

    while (std::getline(ss, line)) {
        lines.push_back(line);
    }

    return lines;
}

// Wrap long lines to simulate text wrap at specified column width
std::vector<std::string> wrapLines(const std::vector<std::string>& lines, int wrapColumn) {
    if (wrapColumn <= 0) {
        return lines;  // No wrapping
    }

    std::vector<std::string> wrappedLines;
    for (const auto& line : lines) {
        if ((int)line.length() <= wrapColumn) {
            wrappedLines.push_back(line);
        } else {
            // Split long line into multiple wrapped lines
            for (size_t i = 0; i < line.length(); i += wrapColumn) {
                wrappedLines.push_back(line.substr(i, wrapColumn));
            }
        }
    }
    return wrappedLines;
}

long long runBenchmark(Grammar* grammar, const std::vector<std::string>& lines, int& tokenCount) {
    auto start = std::chrono::high_resolution_clock::now();

    StateStack* state = const_cast<StateStack*>(INITIAL);
    tokenCount = 0;

    for (const auto& line : lines) {
        auto result = grammar->tokenizeLine(line, state);
        state = result.ruleStack;
        tokenCount += result.tokens.size();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    return duration.count();
}

// Helper to resolve paths that work from both project root and build directory
std::string resolvePath(const std::string& path) {
    std::ifstream testFile(path);
    if (testFile.good()) {
        return path;
    }
    return "../../" + path;
}

int main(int argc, char** argv) {
    std::cout << "TextMate Large File Benchmark (C++)" << std::endl;
    std::cout << "====================================" << std::endl << std::endl;

    // Parse command-line arguments
    int wrapColumn = 0;  // 0 means no wrapping
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--wrap" && i + 1 < argc) {
            wrapColumn = std::atoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: benchmark_large [OPTIONS]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --wrap <columns>   Enable text wrapping at specified column width (e.g., 100)" << std::endl;
            std::cout << "  --help             Show this help message" << std::endl;
            return 0;
        }
    }

    if (wrapColumn > 0) {
        std::cout << "Text wrap enabled: " << wrapColumn << " characters per line" << std::endl << std::endl;
    }

    // Determine benchmark path (try project root first, then build directory)
    std::string benchmarkPath = "benchmark";
    std::ifstream testFile(benchmarkPath + "/large.js.txt");
    if (!testFile.good()) {
        benchmarkPath = "../../benchmark";
    }

    std::vector<BenchmarkCase> benchmarkCases = {
        {
            "jQuery v2.0.3",
            benchmarkPath + "/large.js.txt",
            benchmarkPath + "/JavaScript.tmLanguage.json",
            "source.js"
        },
        {
            "vscode.d.ts",
            benchmarkPath + "/vscode.d.ts.txt",
            resolvePath("test-cases/themes/syntaxes/TypeScript.tmLanguage.json"),
            "source.ts"
        },
        {
            "Bootstrap CSS v3.1.1",
            benchmarkPath + "/bootstrap.css.txt",
            resolvePath("test-cases/first-mate/fixtures/css.json"),
            "source.css"
        },
        {
            "Bootstrap CSS minified",
            benchmarkPath + "/bootstrap.min.css.txt",
            resolvePath("test-cases/first-mate/fixtures/css.json"),
            "source.css"
        },
        {
            "jQuery minified",
            benchmarkPath + "/large.min.js.txt",
            benchmarkPath + "/JavaScript.tmLanguage.json",
            "source.js"
        },
        {
            "Bootstrap multi-byte minified",
            benchmarkPath + "/main.08642f99.css.txt",
            resolvePath("test-cases/first-mate/fixtures/css.json"),
            "source.css"
        },
        {
            "Simple minified JS",
            benchmarkPath + "/minified.js.txt",
            benchmarkPath + "/JavaScript.tmLanguage.json",
            "source.js"
        }
    };

    int warmupRuns = 1;
    int benchmarkRuns = 3;

    std::cout << "Warmup runs: " << warmupRuns << ", Benchmark runs: " << benchmarkRuns << std::endl << std::endl;

    std::vector<BenchmarkResult> results;

    // Create Oniguruma library
    IOnigLib* onigLib = new DefaultOnigLib();

    for (const auto& testCase : benchmarkCases) {
        std::cout << "Benchmarking " << testCase.name << "... " << std::flush;

        try {
            // Read file
            std::string content = readFile(testCase.filePath);
            std::vector<std::string> lines = splitLines(content);

            // Apply text wrapping if enabled
            if (wrapColumn > 0) {
                lines = wrapLines(lines, wrapColumn);
            }

            int charCount = content.length();

            // Parse grammar
            std::string grammarContent = readFile(testCase.grammarPath);
            IRawGrammar* rawGrammar = parseJSONGrammar(grammarContent, nullptr);

            // Create registry
            RegistryOptions options;
            options.onigLib = onigLib;

            // Simple grammar loader that returns our grammar
            std::map<std::string, IRawGrammar*> grammarMap;
            grammarMap[testCase.scopeName] = rawGrammar;

            options.loadGrammar = [&grammarMap](const std::string& scopeName) -> IRawGrammar* {
                auto it = grammarMap.find(scopeName);
                if (it != grammarMap.end()) {
                    return it->second;
                }
                return nullptr;
            };

            Registry registry(options);

            // Load grammar using addGrammar then loadGrammar
            std::vector<std::string> emptyDeps;
            registry.addGrammar(rawGrammar, emptyDeps, 0, nullptr);
            Grammar* grammar = registry.loadGrammar(testCase.scopeName);
            if (!grammar) {
                std::cout << "FAILED (grammar not found)" << std::endl;
                continue;
            }

            // Warmup runs
            for (int i = 0; i < warmupRuns; i++) {
                int dummyTokens;
                runBenchmark(grammar, lines, dummyTokens);
            }

            // Benchmark runs
            std::vector<long long> times;
            int tokenCount = 0;

            for (int i = 0; i < benchmarkRuns; i++) {
                long long elapsed = runBenchmark(grammar, lines, tokenCount);
                times.push_back(elapsed);
            }

            // Use median time
            std::sort(times.begin(), times.end());
            long long medianTime = times[times.size() / 2];

            results.push_back({
                testCase.name,
                (int)lines.size(),
                tokenCount,
                charCount,
                medianTime
            });

            std::cout << "✓ (" << medianTime << " ms)" << std::endl;

        } catch (const std::exception& e) {
            std::cout << "✗ ERROR: " << e.what() << std::endl;
        }
    }

    delete onigLib;

    // Display results
    std::cout << std::endl;
    std::cout << std::string(110, '=') << std::endl;
    printf("%-35s %-10s %-12s %-8s %-10s %-10s %-12s %-8s\n",
           "Test Name", "Lines", "Tokens", "MB", "Time(ms)", "Lines/s", "Tokens/s", "MB/s");
    std::cout << std::string(110, '=') << std::endl;

    long long totalTime = 0;
    int totalLines = 0;
    int totalTokens = 0;
    int totalChars = 0;

    for (const auto& result : results) {
        double mb = result.charCount / 1024.0 / 1024.0;
        printf("%-35s %-10d %-12d %-8.2f %-10lld %-10.0f %-12.0f %-8.2f\n",
               result.testName.c_str(),
               result.lineCount,
               result.tokenCount,
               mb,
               result.elapsedMs,
               result.linesPerSecond(),
               result.tokensPerSecond(),
               result.mbPerSecond());

        totalTime += result.elapsedMs;
        totalLines += result.lineCount;
        totalTokens += result.tokenCount;
        totalChars += result.charCount;
    }

    std::cout << std::string(110, '=') << std::endl;
    double totalMB = totalChars / 1024.0 / 1024.0;
    printf("%-35s %-10d %-12d %-8.2f %-10lld\n",
           "TOTAL", totalLines, totalTokens, totalMB, totalTime);
    std::cout << std::string(110, '=') << std::endl;

    double overallLinesPerSec = totalLines / (totalTime / 1000.0);
    double overallTokensPerSec = totalTokens / (totalTime / 1000.0);
    double overallMBPerSec = totalMB / (totalTime / 1000.0);

    std::cout << std::endl;
    std::cout << "Overall Performance:" << std::endl;
    printf("  Lines/sec:  %.0f\n", overallLinesPerSec);
    printf("  Tokens/sec: %.0f\n", overallTokensPerSec);
    printf("  MB/sec:     %.2f\n", overallMBPerSec);
    std::cout << std::endl;

    return 0;
}
