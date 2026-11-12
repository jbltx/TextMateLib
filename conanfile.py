from conan import ConanFile
from conan.errors import ConanException
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy
import os


class TextMateLibConan(ConanFile):
    name = "textmatelib"
    version = "0.2.1"
    license = "MIT"
    author = "Mickael Bonfill <jbltx@protonmail.com>"
    url = "https://github.com/jbltx/TextMateLib"
    description = "TextMate syntax highlighting library - a C++ implementation of TextMate grammar parsing and tokenization"
    topics = ("textmate", "syntax", "highlighting", "grammar", "tokenizer", "parsing")
    package_type = "library"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True
    }
    # The monorepo layout is kept in the Conan cache, so the CMake default
    # THIRDPARTY_DIR (../../thirdparty from packages/tml-cpp) resolves as is.
    exports_sources = (
        "LICENSE",
        "packages/tml-cpp/CMakeLists.txt",
        "packages/tml-cpp/src/*",
        "thirdparty/oniguruma/*",
        "thirdparty/rapidjson/include/*",
    )

    # Files that only exist once the git submodules are checked out.
    _submodule_sentinels = (
        "thirdparty/oniguruma/CMakeLists.txt",
        "thirdparty/rapidjson/include/rapidjson/rapidjson.h",
    )

    def export_sources(self):
        # Only checks the submodules are there; exports_sources does the copy.
        # Without this, empty thirdparty/ dirs fail late inside CMake.
        missing = [p for p in self._submodule_sentinels
                   if not os.path.isfile(os.path.join(self.recipe_folder, p))]
        if missing:
            raise ConanException(
                "Missing git submodules ({}). Run: git submodule update --init "
                "thirdparty/oniguruma thirdparty/rapidjson".format(", ".join(missing))
            )

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self, src_folder="packages/tml-cpp")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.variables["USE_WASM_BUILD"] = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", src=self.export_sources_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()
        # Oniguruma is built via ExternalProject_Add and is not part of cmake --install.
        # Copy the static library and public headers so that consumers can link all symbols
        # (libtml.a has undefined references to onig_* that the consumer's linker must resolve)
        # and so that installed headers like onigLib.h (which include "oniguruma.h") still compile.
        onig_install = os.path.join(self.build_folder, "oniguruma")
        copy(self, "*.a", src=os.path.join(onig_install, "lib"),
             dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.lib", src=os.path.join(onig_install, "lib"),
             dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.h", src=os.path.join(onig_install, "include"),
             dst=os.path.join(self.package_folder, "include"), keep_path=False)

    def package_info(self):
        # tml depends on onig at link time; list both so consumers get all symbols resolved.
        # Order matters for static linking: tml first (it references onig), then onig.
        self.cpp_info.libs = ["tml", "onig"]
        self.cpp_info.set_property("cmake_file_name", "TextMateLib")
        self.cpp_info.set_property("cmake_target_name", "TextMateLib::tml")

        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs = ["pthread"]
