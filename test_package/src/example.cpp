#include "tml/c_api.h"

int main() {
    TextMateOnigLib lib = textmate_oniglib_create();
    if (!lib) {
        return 1;
    }

    TextMateRegistry registry = textmate_registry_create(lib);
    if (!registry) {
        textmate_oniglib_dispose(lib);
        return 1;
    }

    textmate_registry_dispose(registry);
    textmate_oniglib_dispose(lib);
    return 0;
}
