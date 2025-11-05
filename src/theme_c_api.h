#ifndef TEXTMATELIB_THEME_C_API_H
#define TEXTMATELIB_THEME_C_API_H

#include "theme.h"
#include <cstdint>
#include <string>

namespace tml {

/**
 * Helper class to manage theme resources and provide C API implementation
 *
 * NOTE: We do NOT delete the Theme object in the destructor because the
 * Theme class destructor has issues (from the ported C++ implementation).
 * This is a workaround - the Theme object will leak when disposed, but
 * this is preferable to hanging. See PHASE1B_FINDINGS.md for details.
 */
class ManagedTheme {
public:
    Theme* theme;
    StyleAttributes* defaults;

    ManagedTheme(Theme* theme_, StyleAttributes* defaults_)
        : theme(theme_), defaults(defaults_) {}

    ~ManagedTheme() {
        // NOTE: NOT deleting theme due to Theme destructor hanging issue
        // Workaround: Theme will be leaked, but this prevents hanging
        // if (theme) {
        //     delete theme;  // DISABLED - causes hang
        // }

        // Only delete defaults if it's not owned by Theme
        // (In this case it is, but the workaround is to leak both)
        // if (defaults) {
        //     delete defaults;  // DISABLED - defaults is cleaned up by theme destructor
        // }
    }
};

// Note: hexColorToUint32 and parseJsonTheme are defined as static in c_api.cpp


} // namespace tml

#endif // TEXTMATELIB_THEME_C_API_H
