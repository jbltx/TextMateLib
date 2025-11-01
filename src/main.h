#ifndef VSCODE_TEXTMATE_MAIN_H
#define VSCODE_TEXTMATE_MAIN_H

// Main public API header for vscode-textmate C++ port

#include "types.h"
#include "onigLib.h"
#include "rawGrammar.h"
#include "parseRawGrammar.h"
#include "theme.h"
#include "registry.h"
#include "grammar.h"
#include "encodedTokenAttributes.h"

namespace vscode_textmate {

// Export all public types and classes
using vscode_textmate::Registry;
using vscode_textmate::RegistryOptions;
using vscode_textmate::IGrammarConfiguration;
using vscode_textmate::Grammar;
using vscode_textmate::IGrammar;
using vscode_textmate::StateStack;
using vscode_textmate::IToken;
using vscode_textmate::ITokenizeLineResult;
using vscode_textmate::ITokenizeLineResult2;
using vscode_textmate::IRawGrammar;
using vscode_textmate::IRawTheme;
using vscode_textmate::IOnigLib;
using vscode_textmate::DefaultOnigLib;
using vscode_textmate::parseRawGrammar;

// INITIAL constant
extern const StateStack* INITIAL;

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_MAIN_H
