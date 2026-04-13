from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy
import os


class TextMateLibConan(ConanFile):
    name = "textmatelib"
    version = "0.1.0"
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
    # Only include sources within this package directory; thirdparty deps are
    # handled by export_sources() to avoid Conan 2's restriction on ".." patterns.
    exports_sources = (
        "CMakeLists.txt",
        "src/*",
    )

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def export_sources(self):
        # Copy vendored thirdparty dependencies (git submodules) from the monorepo
        # root into the Conan export-sources folder so that `conan create` works
        # without requiring ".." patterns in exports_sources (which Conan 2 rejects).
        thirdparty_root = os.path.normpath(
            os.path.join(self.recipe_folder, "..", "..", "thirdparty")
        )
        export_thirdparty = os.path.join(self.export_sources_folder, "thirdparty")
        copy(self, "rapidjson/*", src=thirdparty_root, dst=export_thirdparty)
        copy(self, "oniguruma/*", src=thirdparty_root, dst=export_thirdparty)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.variables["USE_WASM_BUILD"] = False
        # Determine the thirdparty directory.  Two cases:
        # 1. conan create  – thirdparty was exported into source_folder/thirdparty
        # 2. conan install (local/monorepo) – thirdparty lives at ../../thirdparty
        #    relative to the package folder (packages/tml-cpp).
        thirdparty_in_source = os.path.join(self.source_folder, "thirdparty")
        if os.path.isdir(thirdparty_in_source):
            thirdparty_dir = thirdparty_in_source
        else:
            thirdparty_dir = os.path.normpath(
                os.path.join(self.source_folder, "..", "..", "thirdparty")
            )
        tc.variables["THIRDPARTY_DIR"] = thirdparty_dir.replace("\\", "/")
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["tml"]
        self.cpp_info.set_property("cmake_file_name", "TextMateLib")
        self.cpp_info.set_property("cmake_target_name", "TextMateLib::tml")

        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs = ["pthread"]
