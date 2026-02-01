#ifndef TEXTMATELIB_TML_C_H
#define TEXTMATELIB_TML_C_H

/**
 * @file tml_c.h
 * @brief C FFI API header for TextMateLib
 *
 * This header provides C bindings for language interoperability and testing.
 * It includes the complete C API for:
 * - Core registry and grammar management
 * - Session API for stateful tokenization
 * - Theme and styling
 * - Syntax highlighting
 *
 * Note: This header is designed for C code or C++ code that needs C FFI bindings.
 * For pure C++ usage, include tml.h instead.
 */

// ============================================================================
// C API Headers (FFI for language bindings)
// ============================================================================
#include "c_api.h"              // Core C FFI API for language bindings
#include "session_c_api.h"      // C API for Session
#include "syntax_highlighter_c_api.h" // C API for SyntaxHighlighter

#endif // TEXTMATELIB_TML_C_H
