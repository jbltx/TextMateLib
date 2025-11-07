cmake_minimum_required(VERSION 3.14)
project(tml VERSION 1.0.0 LANGUAGES CXX)

include(GenerateExportHeader)

# Set C++17 standard for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# WASM build option
option(USE_WASM_BUILD "Build for WebAssembly with Emscripten" OFF)
set(WASM_VARIANT "standard" CACHE STRING "WASM build variant: minimal, standard, full, debug")
set_property(CACHE WASM_VARIANT PROPERTY STRINGS "minimal" "standard" "full" "debug")

# WASM 2023 Configuration (when building for WebAssembly)
if(USE_WASM_BUILD)
    # WebAssembly 2023 target flags
    set(WASM_2023_FLAGS
        -target wasm32-unknown-emscripten
        -msimd128
        -fwasm-exceptions
        -mbulk-memory
        -mnontrapping-fptoint
        -msse4.2
        -sWASM_BIGINT
        -sSUPPORT_LONGJMP=wasm
    )

    # Emscripten runtime flags
    set(EMSCRIPTEN_FLAGS
        -s WASM=1
        -s MODULARIZE=1
        -s EXPORT_NAME='createTextMateModule'
        -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue']
        -s ALLOW_MEMORY_GROWTH=1
        -s ABORTING_MALLOC=0
    )

    # Memory configuration
    set(WASM_MEMORY
        -s INITIAL_MEMORY=33554432
        -s MAXIMUM_MEMORY=536870912
    )

    # Bulk memory operations
    set(WASM_BULK_MEMORY
        -mbulk-memory
        -s BULK_MEMORY=1
    )

    # Exception handling (compiler flag)
    set(EXCEPTION_FLAGS -fwasm-exceptions)

    # Linker exception flags (separate) - not used for static library builds
    set(EXCEPTION_LINKER_FLAGS
        -s EXCEPTION_CATCHING_ALLOWED='["std::exception"]'
        -sLIKE_FETCH=1
    )

    # Variant-specific flags
    if(WASM_VARIANT STREQUAL "full" OR WASM_VARIANT STREQUAL "debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWASM_2023_FEATURES=1")
    endif()

    # Combine all flags for compilation (excluding linker-only flags)
    string(REPLACE ";" " " WASM_2023_FLAGS_STR "${WASM_2023_FLAGS}")
    string(REPLACE ";" " " EXCEPTION_FLAGS_STR "${EXCEPTION_FLAGS}")

    # Compiler flags for object file generation suitable for archiving
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WASM_2023_FLAGS_STR} ${EXCEPTION_FLAGS_STR}")

    # Set output format - for static archives, skip JavaScript/WASM runtime flags
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O1 -s ASSERTIONS=1")
endif()

# Optimization flags (native builds)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT USE_WASM_BUILD)
    if(MSVC)
        # MSVC-specific flags
        set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /fp:fast")
        set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
    else()
        # GCC/Clang flags
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -ffast-math")
        set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rapidjson/include
)

# Source files
set(TEXTMATE_SOURCES
    src/main.cpp
    src/utils.cpp
    src/onigLib.cpp
    src/rawGrammar.cpp
    src/parseRawGrammar.cpp
    src/theme.cpp
    src/encodedTokenAttributes.cpp
    src/rule.cpp
    src/matcher.cpp
    src/grammarDependencies.cpp
    src/basicScopesAttributeProvider.cpp
    src/tokenizeString.cpp
    src/grammar.cpp
    src/registry.cpp
    src/c_api.cpp
    src/session.cpp
    src/syntax_highlighter.cpp
)

# Add Emscripten bindings for WASM builds
if(USE_WASM_BUILD)
    list(APPEND TEXTMATE_SOURCES src/wasm/bindings.cpp)
endif()

# Header files (for IDE support)
set(TEXTMATE_HEADERS
    src/types.h
    src/utils.h
    src/onigLib.h
    src/rawGrammar.h
    src/parseRawGrammar.h
    src/theme.h
    src/theme_c_api.h
    src/encodedTokenAttributes.h
    src/rule.h
    src/matcher.h
    src/grammarDependencies.h
    src/basicScopesAttributeProvider.h
    src/tokenizeString.h
    src/grammar.h
    src/registry.h
    src/main.h
    src/c_api.h
    src/session.h
    src/session_c_api.h
    src/syntax_highlighter.h
    src/syntax_highlighter_c_api.h
    src/wasm/simd_bindings.h
    src/wasm/bulk_memory_bindings.h
    src/wasm/exception_bindings.h
    src/wasm/bigint_bindings.h
)

include(ExternalProject)

# Find Oniguruma library (skip for WASM builds, will build for WASM)
if(NOT USE_WASM_BUILD)
    message(STATUS "Building native oniguruma from source")

    # Build oniguruma
    set(ONIG_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/oniguruma)

    # Build configuration for oniguruma - ensure static library, not shared
    set(ONIG_CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS=OFF
        -DENABLE_POSIX_API=OFF
        -DENABLE_BINARY_COMPATIBLE_POSIX_API=OFF
        -DENABLE_STATISTICS=OFF
        -DINSTALL_DOCUMENTATION=OFF
        -DINSTALL_EXAMPLES=OFF
        -DBUILD_TEST=OFF
        -DCMAKE_INSTALL_PREFIX=${ONIG_INSTALL_DIR}
        -DCMAKE_C_FLAGS=-DONIG_STATIC
    )

    # On Windows/MSVC, ensure matching runtime library
    if(MSVC)
        list(APPEND ONIG_CMAKE_ARGS -DMSVC_STATIC_RUNTIME=ON)
    endif()

    ExternalProject_Add(
        oniguruma
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/oniguruma
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/oniguruma/build
        CMAKE_ARGS ${ONIG_CMAKE_ARGS}
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
        INSTALL_COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install
    )
    set(ONIGURUMA_LIB ${ONIG_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onig${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(ONIGURUMA_INCLUDE_DIR ${ONIG_INSTALL_DIR}/include)

else()
    message(STATUS "Building for WASM - oniguruma will be built from source")

    # Build oniguruma for WASM
    set(ONIG_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/oniguruma-wasm)

    ExternalProject_Add(
        oniguruma_wasm
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/oniguruma
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/oniguruma/build_wasm
        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_CROSSCOMPILING_EMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}
            -DCMAKE_INSTALL_PREFIX=${ONIG_INSTALL_DIR}
        INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
    )

    set(ONIGURUMA_LIB ${ONIG_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onig${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(ONIGURUMA_INCLUDE_DIR ${ONIG_INSTALL_DIR}/include)
endif()

# Create output targets
if(USE_WASM_BUILD)
    # WASM builds dual targets:
    # 1. Static archive library (.a) for Unity 2021.2+ compatibility
    #    Contains WebAssembly Object Files (.o) - preferred over LLVM Bitcode
    add_library(tml STATIC
        ${TEXTMATE_SOURCES}
    )
    set_target_properties(tml PROPERTIES
        OUTPUT_NAME "tml-${WASM_VARIANT}"
        SUFFIX ".a"
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/wasm
    )

    # 2. Executable target for browser testing (.js + .wasm)
    #    Includes JavaScript bindings and Emscripten runtime
    add_executable(tml-browser
        ${TEXTMATE_SOURCES}
    )
    set_target_properties(tml-browser PROPERTIES
        OUTPUT_NAME "tml-${WASM_VARIANT}"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/browser
    )

    # Add Emscripten linker flags for executable target only
    # These flags are needed for the JavaScript binding to work
    set(EMSCRIPTEN_LINKER_FLAGS
        -s WASM=1
        -s MODULARIZE=1
        -s EXPORT_NAME='createTextMateModule'
        -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue']
        -s ALLOW_MEMORY_GROWTH=1
        -s ABORTING_MALLOC=0
        --bind
    )
    string(REPLACE ";" " " EMSCRIPTEN_LINKER_FLAGS_STR "${EMSCRIPTEN_LINKER_FLAGS}")
    set_target_properties(tml-browser PROPERTIES
        LINK_FLAGS "${EMSCRIPTEN_LINKER_FLAGS_STR}"
    )

    # Also add --bind flag to compiler flags for the executable target
    target_compile_options(tml-browser PRIVATE --bind)

else()
    # Native builds - default to static library for easier testing and linking
    # Can be overridden with: cmake -DBUILD_SHARED_LIBS=ON
    add_library(tml
        ${TEXTMATE_SOURCES}
        ${TEXTMATE_HEADERS}
    )

    # Ensure static linking with oniguruma on native builds
    target_compile_definitions(tml PRIVATE ONIG_STATIC)
endif()

# Generate export header for library
generate_export_header(tml
    EXPORT_MACRO_NAME TML_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/tml_export.h
    STATIC_DEFINE TEXTMATE_STATIC
)

# Include generated export header
target_include_directories(tml PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Link libraries
if(USE_WASM_BUILD)
    # Add dependency on oniguruma WASM build for both targets
    add_dependencies(tml oniguruma_wasm)
    add_dependencies(tml-browser oniguruma_wasm)

    # Link against oniguruma
    target_link_libraries(tml PUBLIC "${ONIGURUMA_LIB}")
    target_link_libraries(tml-browser PUBLIC "${ONIGURUMA_LIB}")

    # Include oniguruma headers
    target_include_directories(tml PUBLIC "${ONIGURUMA_INCLUDE_DIR}")
    target_include_directories(tml-browser PUBLIC "${ONIGURUMA_INCLUDE_DIR}")
else()
    # Add dependency on oniguruma build
    add_dependencies(tml oniguruma)

    # Link against oniguruma static library
    # Use target_link_libraries with the absolute path and PUBLIC to propagate to consumers
    target_link_libraries(tml PUBLIC "${ONIGURUMA_LIB}")

    # Include oniguruma headers
    target_include_directories(tml PUBLIC "${ONIGURUMA_INCLUDE_DIR}")
endif()

# Install targets
install(TARGETS tml
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${TEXTMATE_HEADERS}
    DESTINATION include/tml
)

# Enable testing (skip for WASM builds)
if(NOT USE_WASM_BUILD)
    enable_testing()

    # Add test subdirectory if it exists
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        add_subdirectory(tests)
    endif()
endif()

# Print configuration
message(STATUS "tml configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Source files: ${TEXTMATE_SOURCES}")
