# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/../../thirdparty/oniguruma"
  "/Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/thirdparty/oniguruma/build"
  "/Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/oniguruma-prefix"
  "/Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/oniguruma-prefix/tmp"
  "/Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/oniguruma-prefix/src/oniguruma-stamp"
  "/Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/oniguruma-prefix/src"
  "/Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/oniguruma-prefix/src/oniguruma-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/oniguruma-prefix/src/oniguruma-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/mickaelbonfill/dev/totogit/thirdparty/TextMateLib/packages/tml-cpp/build/oniguruma-prefix/src/oniguruma-stamp${cfgdir}") # cfgdir has leading slash
endif()
