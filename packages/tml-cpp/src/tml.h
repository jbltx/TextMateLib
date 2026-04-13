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

/// @defgroup cpp_api C++ Public API
/// @{
/// Main public API for C++ applications using TextMateLib.
/// These type aliases and constants provide the core components for syntax highlighting.

/// @defgroup core_types Core Types and Interfaces
/// Fundamental types for grammar management, tokenization state, and token representation.

/// @brief Central registry for managing grammars and themes
/// @ingroup core_types
/// @see Registry, RegistryOptions
using tml::Registry;

/// @brief Configuration options for the registry
/// @ingroup core_types
/// @see Registry
using tml::RegistryOptions;

/// @brief Interface for customizing grammar configuration behavior
/// @ingroup core_types
/// @see Grammar
using tml::IGrammarConfiguration;

/// @brief Compiled grammar representation for tokenization
/// @ingroup core_types
/// @details Grammar objects handle rule matching and token generation.
///          Created by Registry::loadGrammar() after registering grammar definitions.
/// @see Grammar, Registry
using tml::Grammar;

/// @brief Abstract interface for grammar implementations
/// @ingroup core_types
/// @see Grammar
using tml::IGrammar;

/// @brief Immutable parsing state representing position in a grammar rule hierarchy
/// @ingroup core_types
/// @details StateStack encapsulates the parsing state at the end of a line.
///          Two StateStacks that are equal() indicate the same parsing position,
///          enabling incremental tokenization optimizations.
/// @see ITokenizeLineResult::ruleStack
using tml::StateStack;

/// @brief Single token with scope information
/// @ingroup core_types
/// @details Represents a region of text with its associated scope hierarchy.
/// @see ITokenizeLineResult
using tml::IToken;

/// @brief Result of tokenizing a single line
/// @ingroup core_types
/// @details Contains tokens and the state to continue on the next line.
/// @see Grammar::tokenizeLine()
using tml::ITokenizeLineResult;

/// @brief Result of tokenizing a line with encoded tokens
/// @ingroup core_types
/// @details More compact representation than ITokenizeLineResult.
/// @see Grammar::tokenizeLine2()
using tml::ITokenizeLineResult2;

/// @brief Raw (uncompiled) grammar data structure from JSON parsing
/// @ingroup core_types
/// @see parseRawGrammar()
using tml::IRawGrammar;

/// @brief Raw (uncompiled) theme data structure from JSON parsing
/// @ingroup core_types
/// @see Theme
using tml::IRawTheme;

/// @brief Interface to the Oniguruma regex engine
/// @ingroup core_types
/// @details Handles pattern matching for rule evaluation during tokenization.
using tml::IOnigLib;

/// @brief Default Oniguruma implementation
/// @ingroup core_types
/// @see Registry
using tml::DefaultOnigLib;

/// @defgroup grammar_processing Grammar Processing
/// Grammar definition parsing and compilation utilities.

/// @brief Parse a TextMate grammar from raw JSON data
/// @ingroup grammar_processing
/// @see Grammar, Registry
using tml::parseRawGrammar;

/// @defgroup tokenization Tokenization and Tokens
/// Tokenization results and token attribute encoding.

/// @brief Single token with scope information
/// @ingroup tokenization
/// @details Maps a text range to a scope hierarchy for styling.
using tml::IToken;

/// @brief Compact 32-bit encoding of token attributes
/// @ingroup tokenization
/// @details Used in ITokenizeLineResult2 for space-efficient token representation.
using tml::EncodedTokenAttributes;

/// @defgroup session_api Session API
/// @{
/// Stateful incremental tokenization with line caching for editor integration.
///
/// **Types:**
/// - SessionImpl - High-level session manager for incremental document tokenization
/// - SessionLine - Represents a cached line in the session
/// - SessionMetadata - Metadata and statistics for a session

/// @brief High-level session manager for incremental document tokenization
/// @details Handles line-by-line tokenization with automatic state caching
///          and early stopping optimization. Ideal for editor integration.
/// @see SessionLine, SessionMetadata
using tml::SessionImpl;

/// @brief Represents a cached line in the session
/// @see SessionImpl
using tml::SessionLine;

/// @brief Metadata and statistics for a session
/// @see SessionImpl
using tml::SessionMetadata;

/// @}

/// @defgroup theme_api Theme and Styling
/// Theme parsing and scope-to-color mapping.

/// @brief Theme containing scope-to-color-and-style mappings
/// @ingroup theme_api
/// @details Applies colors and font styles to tokens based on their scopes.
///          Created by loading a TextMate theme definition.
using tml::Theme;

/// @defgroup utilities Utilities
/// @{
/// Helper utilities for pattern matching and operations.
///
/// **Types:**
/// - Matcher - Pattern matching utilities and cache

/// @brief Pattern matching utilities and cache
/// @details Used internally for regex pattern caching and optimization.
using tml::Matcher;

/// @}

/// @defgroup syntax_highlighting Syntax Highlighting Convenience API
/// @{
/// High-level syntax highlighting combining grammar + theme + caching.
///
/// **Types:**
/// - SyntaxHighlighter - High-level API combining grammar, theme, and session
/// - HighlightedToken - Single highlighted token with computed style attributes
/// - HighlightedLine - Result of highlighting a single line
/// - HighlighterCache - Cache for highlighted lines

/// @brief High-level API combining grammar, theme, and session
/// @details Provides a convenient single point for tokenization and styling.
///          Automatically handles state management and caching.
using tml::SyntaxHighlighter;

/// @brief Single highlighted token with computed style attributes
/// @see SyntaxHighlighter, HighlightedLine
using tml::HighlightedToken;

/// @brief Result of highlighting a single line
/// @details Contains tokens with computed foreground/background colors and styles.
/// @see SyntaxHighlighter
using tml::HighlightedLine;

/// @brief Cache for highlighted lines
/// @details Optimizes repeated highlighting of unchanged lines.
/// @see SyntaxHighlighter
using tml::HighlighterCache;

/// @}

/// @defgroup constants Constants and Initialization
/// Global constants and initialization functions.

/// @brief Initial parsing state for the first line of a document
/// @ingroup constants
/// @details Used as the prevState argument when tokenizing the first line
///          of any document. Read-only; do not delete.
extern const StateStack* INITIAL;

/// @}

} // namespace tml

#endif // TEXTMATELIB_TML_H
