#ifndef TEXTMATELIB_TML_H
#define TEXTMATELIB_TML_H

/**
 * @file tml.h
 * @brief Comprehensive public API header for TextMateLib
 *
 * This header includes all necessary components for using TextMateLib in C++ applications.
 * It provides a single point of inclusion for:
 * - Core types and interfaces
 * - Grammar processing and tokenization
 * - Theme and styling
 * - Registry and grammar management
 * - Session API for stateful tokenization
 * - Syntax highlighting utilities
 * - Regex engine interface
 * - Token encoding and attributes
 */

// ============================================================================
// Core Type Definitions
// ============================================================================
#include "types.h"              // Core types, interfaces, and enums

// ============================================================================
// Regex and Pattern Matching
// ============================================================================
#include "onigLib.h"            // Oniguruma regex engine wrapper
#include "matcher.h"            // Pattern matching utilities

// ============================================================================
// Grammar Processing Pipeline
// ============================================================================
#include "rawGrammar.h"         // Raw grammar data structures (JSON-based)
#include "parseRawGrammar.h"    // JSON grammar parser
#include "rule.h"               // Individual grammar rules
#include "grammarDependencies.h" // Grammar dependency resolution
#include "grammar.h"            // Compiled grammar with rule matching

// ============================================================================
// Tokenization
// ============================================================================
#include "tokenizeString.h"     // Core tokenization logic and state transitions
#include "encodedTokenAttributes.h" // Token encoding and attribute management

// ============================================================================
// Session API (Stateful Tokenization)
// ============================================================================
#include "session.h"            // High-level stateful editor session with caching

// ============================================================================
// Theme and Styling
// ============================================================================
#include "theme.h"              // Theme parsing and color/style application
#include "basicScopesAttributeProvider.h" // Scope-to-attribute mapping

// ============================================================================
// Registry and Management
// ============================================================================
#include "registry.h"           // Central grammar/theme manager and lookup

// ============================================================================
// Syntax Highlighting Convenience
// ============================================================================
#include "syntax_highlighter.h"  // High-level convenience wrapper combining grammar + theme

// ============================================================================
// Utilities
// ============================================================================
#include "utils.h"              // Helper functions and utilities

namespace tml {

// ============================================================================
// Public API Exports
// ============================================================================

// Core Types & Interfaces
using tml::Registry;
using tml::RegistryOptions;
using tml::IGrammarConfiguration;
using tml::Grammar;
using tml::IGrammar;
using tml::StateStack;
using tml::IToken;
using tml::ITokenizeLineResult;
using tml::ITokenizeLineResult2;
using tml::IRawGrammar;
using tml::IRawTheme;
using tml::IOnigLib;
using tml::DefaultOnigLib;

// Grammar Processing
using tml::parseRawGrammar;

// Tokenization
using tml::IToken;
using tml::EncodedTokenAttributes;

// Session API
using tml::SessionImpl;
using tml::SessionLine;
using tml::SessionMetadata;

// Theme
using tml::Theme;

// Utilities
using tml::Matcher;

// Syntax Highlighting
using tml::SyntaxHighlighter;
using tml::HighlightedToken;
using tml::HighlightedLine;
using tml::HighlighterCache;

// ============================================================================
// Constants & Initialization
// ============================================================================

// Initial state for tokenization
extern const StateStack* INITIAL;

} // namespace tml

#endif // TEXTMATELIB_TML_H
