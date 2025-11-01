#ifndef VSCODE_TEXTMATE_EXCEPTION_BINDINGS_H
#define VSCODE_TEXTMATE_EXCEPTION_BINDINGS_H

#ifdef WASM_2023_FEATURES

#include <string>
#include <exception>
#include <stdexcept>

/**
 * WASM Exception handling utilities
 *
 * Provides native WASM exception support with proper error propagation
 * to JavaScript. Uses -fwasm-exceptions for zero-overhead error handling.
 */

class TextMateException : public std::runtime_error {
public:
    explicit TextMateException(const std::string& message)
        : std::runtime_error(message) {}

    const char* what() const noexcept override;
};

class GrammarException : public TextMateException {
public:
    explicit GrammarException(const std::string& message)
        : TextMateException("Grammar error: " + message) {}
};

class ParseException : public TextMateException {
public:
    explicit ParseException(const std::string& message)
        : TextMateException("Parse error: " + message) {}
};

class ThemeException : public TextMateException {
public:
    explicit ThemeException(const std::string& message)
        : TextMateException("Theme error: " + message) {}
};

/**
 * Exception handler for WASM runtime
 * Converts C++ exceptions to JavaScript exceptions
 */
class ExceptionHandler {
public:
    /**
     * Safely execute a function with exception handling
     */
    template<typename Func>
    static auto wrapFunction(Func fn) noexcept {
        try {
            return fn();
        } catch (const TextMateException& e) {
            // Re-throw as is for proper WASM exception propagation
            throw;
        } catch (const std::exception& e) {
            throw TextMateException(std::string("Unexpected error: ") + e.what());
        } catch (...) {
            throw TextMateException("Unknown error occurred");
        }
    }

    /**
     * Get the last exception message
     */
    static const char* getLastErrorMessage();

    /**
     * Clear the last exception
     */
    static void clearLastError();
};

#endif // WASM_2023_FEATURES

#endif // VSCODE_TEXTMATE_EXCEPTION_BINDINGS_H
