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
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True
    }
    exports_sources = (
        "CMakeLists.txt",
        "src/*",
        "thirdparty/rapidjson/*",
        "thirdparty/oniguruma/*"
    )

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

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
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["tml"]
        self.cpp_info.set_property("cmake_file_name", "TextMateLib")
        self.cpp_info.set_property("cmake_target_name", "TextMateLib::tml")

        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs = ["pthread"]
