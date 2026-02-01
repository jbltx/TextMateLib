#ifndef TEXTMATELIB_MAIN_H
#define TEXTMATELIB_MAIN_H

// Main public API header for tml C++ port

#include "types.h"
#include "onigLib.h"
#include "rawGrammar.h"
#include "parseRawGrammar.h"
#include "theme.h"
#include "registry.h"
#include "grammar.h"
#include "encodedTokenAttributes.h"

namespace tml {

// Export all public types and classes
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
using tml::parseRawGrammar;

// INITIAL constant
extern const StateStack* INITIAL;

} // namespace tml

#endif // TEXTMATELIB_MAIN_H
