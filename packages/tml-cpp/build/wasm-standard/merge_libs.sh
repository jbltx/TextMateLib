#!/bin/bash
set -e
rm -rf /Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/wasm-standard/wasm/merge_tmp
mkdir -p /Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/wasm-standard/wasm/merge_tmp/tml
mkdir -p /Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/wasm-standard/wasm/merge_tmp/onig
cd /Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/wasm-standard/wasm/merge_tmp/tml && /Users/mickaelbonfill/dev/emsdk/upstream/emscripten/emar x $1
cd /Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/wasm-standard/wasm/merge_tmp/onig && /Users/mickaelbonfill/dev/emsdk/upstream/emscripten/emar x $2
/Users/mickaelbonfill/dev/emsdk/upstream/emscripten/emar rcs $3 /Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/wasm-standard/wasm/merge_tmp/tml/*.o /Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/wasm-standard/wasm/merge_tmp/onig/*.o
/Users/mickaelbonfill/dev/emsdk/upstream/emscripten/emranlib $3
rm -rf /Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/wasm-standard/wasm/merge_tmp
