#ifdef WASM_2023_FEATURES

#include "wasm/exception_bindings.h"
#include <memory>
#include <mutex>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#endif

// Thread-local storage for last exception message
static thread_local std::string lastErrorMessage;
static thread_local std::mutex errorMutex;

const char* TextMateException::what() const noexcept {
    return std::runtime_error::what();
}

const char* ExceptionHandler::getLastErrorMessage() {
    std::lock_guard<std::mutex> lock(errorMutex);
    return lastErrorMessage.c_str();
}

void ExceptionHandler::clearLastError() {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastErrorMessage.clear();
}

#ifdef __EMSCRIPTEN__

// Exception translators for Emscripten
void translateException(intptr_t exceptionPtr) {
    std::exception_ptr exceptionPointer =
        std::exception_ptr(reinterpret_cast<std::exception_ptr>(exceptionPtr));

    try {
        std::rethrow_exception(exceptionPointer);
    } catch (const TextMateException& e) {
        {
            std::lock_guard<std::mutex> lock(errorMutex);
            lastErrorMessage = e.what();
        }
        emscripten::val::global("Error").new_(
            emscripten::val(std::string(e.what())));
    } catch (const std::exception& e) {
        {
            std::lock_guard<std::mutex> lock(errorMutex);
            lastErrorMessage = std::string("Exception: ") + e.what();
        }
        emscripten::val::global("Error").new_(
            emscripten::val(lastErrorMessage));
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(errorMutex);
            lastErrorMessage = "Unknown exception";
        }
        emscripten::val::global("Error").new_(
            emscripten::val(lastErrorMessage));
    }
}

// Emscripten bindings for exception types
EMSCRIPTEN_BINDINGS(exceptions) {
    emscripten::class_<TextMateException>("TextMateException")
        .constructor<std::string>()
        .function("what", &TextMateException::what);

    emscripten::class_<GrammarException, emscripten::base<TextMateException>>(
        "GrammarException")
        .constructor<std::string>();

    emscripten::class_<ParseException, emscripten::base<TextMateException>>(
        "ParseException")
        .constructor<std::string>();

    emscripten::class_<ThemeException, emscripten::base<TextMateException>>(
        "ThemeException")
        .constructor<std::string>();

    emscripten::class_<ExceptionHandler>("ExceptionHandler")
        .class_function("getLastErrorMessage",
                       &ExceptionHandler::getLastErrorMessage)
        .class_function("clearLastError",
                       &ExceptionHandler::clearLastError);
};

#endif // __EMSCRIPTEN__

#endif // WASM_2023_FEATURES
