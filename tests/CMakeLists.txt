cmake_minimum_required(VERSION 3.10)

# RapidJSON include directory
set(RAPIDJSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/include")

# Google Test include directory
set(GTEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest/include")

# Google Test sources
set(GTEST_SOURCES
    "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest/src/gtest-all.cc"
    "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest/src/gtest_main.cc"
)

# First-Mate test suite executable (now with GTest)
add_executable(test_first_mate
    test_first_mate/test_first_mate.cpp
    "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest/src/gtest-all.cc"
    "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest/src/gtest_main.cc"
)

# Link against the library and its dependencies
target_link_libraries(test_first_mate PRIVATE
    tml
    ${ONIGURUMA_LIB}
)

# Include RapidJSON
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_first_mate PRIVATE ${RAPIDJSON_INCLUDE_DIR})
    message(STATUS "Using RapidJSON from: ${RAPIDJSON_INCLUDE_DIR}")
else()
    message(WARNING "RapidJSON not found at ${RAPIDJSON_INCLUDE_DIR}. test_first_mate may not compile.")
endif()

# Include GTest
if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_first_mate PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest"
    )
    # Disable gtest's regex to avoid conflict with Oniguruma's regex_t
    target_compile_definitions(test_first_mate PRIVATE GTEST_HAS_POSIX_RE=0)
else()
    message(WARNING "Google Test not found at ${GTEST_INCLUDE_DIR}. test_first_mate may not compile.")
endif()

# Set output directory to build root (same as library) to avoid DLL issues on Windows
set_target_properties(test_first_mate PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Add test - run from source directory so fixture paths work
add_test(NAME FirstMateTests COMMAND test_first_mate WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_first_mate)

# Copy fixtures folder to source test directory (for relative path access)
add_custom_command(TARGET test_first_mate POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/test_first_mate/fixtures"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_first_mate/fixtures"
    COMMENT "Ensuring test_first_mate fixtures exist"
)

# Copy tests.json file to source test directory
add_custom_command(TARGET test_first_mate POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/test_first_mate/tests.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_first_mate/tests.json"
    COMMENT "Ensuring test_first_mate tests.json exists"
)

# Benchmark executable for large files (standalone, no GTest needed)
add_executable(benchmark_large
    benchmark_large/benchmark_large.cpp
)

# Link against the library and its dependencies
target_link_libraries(benchmark_large PRIVATE
    tml
    ${ONIGURUMA_LIB}
)

# Include RapidJSON
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(benchmark_large PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

# Set output directory to build root (same as library) to avoid DLL issues on Windows
set_target_properties(benchmark_large PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


# Simple theme test executable
add_executable(test_theme_simple
    test_theme_simple/test_theme_simple.cpp
    ${GTEST_SOURCES}
)

# Link against the library and its dependencies
target_link_libraries(test_theme_simple PRIVATE
    tml
    ${ONIGURUMA_LIB}
)

# Include directories for test_theme_simple
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_theme_simple PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_theme_simple PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest"
    )
endif()

# Set output directory to build root
set_target_properties(test_theme_simple PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Add test - run from source directory so fixture paths work
add_test(NAME ThemeSimpleTests COMMAND test_theme_simple WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_theme_simple)

# Theme test executable
add_executable(test_theme
    test_theme/test_theme.cpp
    ${GTEST_SOURCES}
)

# Link against the library and its dependencies
target_link_libraries(test_theme PRIVATE
    tml
    ${ONIGURUMA_LIB}
)

# Include directories for test_theme
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_theme PRIVATE ${RAPIDJSON_INCLUDE_DIR})
else()
    message(WARNING "RapidJSON not found at ${RAPIDJSON_INCLUDE_DIR}. test_theme may not compile.")
endif()

if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_theme PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest"
    )
else()
    message(WARNING "Google Test not found at ${GTEST_INCLUDE_DIR}. test_theme may not compile.")
endif()

# Set output directory to build root
set_target_properties(test_theme PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Add test - run from source directory so fixture paths work
add_test(NAME ThemeTests COMMAND test_theme WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_theme)

# Debug theme test executable
add_executable(test_theme_debug
    test_theme_debug/test_theme_debug.cpp
    ${GTEST_SOURCES}
)

# Link against the library and its dependencies
target_link_libraries(test_theme_debug PRIVATE
    tml
    ${ONIGURUMA_LIB}
)

# Include directories for test_theme_debug
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_theme_debug PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_theme_debug PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest"
    )
endif()

# Set output directory to build root
set_target_properties(test_theme_debug PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Add test - run from source directory so fixture paths work
add_test(NAME ThemeDebugTests COMMAND test_theme_debug WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_theme_debug)

# File theme test executable
add_executable(test_theme_file
    test_theme_file/test_theme_file.cpp
    ${GTEST_SOURCES}
)

# Link against the library and its dependencies
target_link_libraries(test_theme_file PRIVATE
    tml
    ${ONIGURUMA_LIB}
)

# Include directories for test_theme_file
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_theme_file PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_theme_file PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest"
    )
endif()

# Set output directory to build root
set_target_properties(test_theme_file PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Add test - run from source directory so fixture paths work
add_test(NAME ThemeFileTests COMMAND test_theme_file WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_theme_file)

# Session test executable
add_executable(test_session
    test_session/test_session.cpp
    ${GTEST_SOURCES}
)

# Link against the library and its dependencies
target_link_libraries(test_session PRIVATE
    tml
    ${ONIGURUMA_LIB}
)

# Include directories for test_session
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_session PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_session PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest"
    )
    # Disable gtest's regex to avoid conflict with Oniguruma's regex_t
    target_compile_definitions(test_session PRIVATE GTEST_HAS_POSIX_RE=0)
endif()

# Set output directory to build root
set_target_properties(test_session PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Add test - run from source directory so fixture paths work
add_test(NAME SessionTests COMMAND test_session WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_session)

# Session benchmark executable (standalone, no GTest)
add_executable(benchmark_session
    benchmark_session/benchmark_session.cpp
)

# Link against the library and its dependencies
target_link_libraries(benchmark_session PRIVATE
    tml
    ${ONIGURUMA_LIB}
)

# Include directories for benchmark_session
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(benchmark_session PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

# Set output directory to build root
set_target_properties(benchmark_session PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Session comparison benchmark executable
add_executable(benchmark_session_comparison
    benchmark_session_comparison/benchmark_session_comparison.cpp
)

# Link against the library and its dependencies
target_link_libraries(benchmark_session_comparison PRIVATE
    tml
    ${ONIGURUMA_LIB}
)

# Include directories for benchmark_session_comparison
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(benchmark_session_comparison PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

# Set output directory to build root
set_target_properties(benchmark_session_comparison PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# SyntaxHighlighter test executable
add_executable(test_syntax_highlighter
    test_syntax_highlighter/test_syntax_highlighter.cpp
    ${GTEST_SOURCES}
)

# Link against the library and its dependencies
target_link_libraries(test_syntax_highlighter PRIVATE
    tml
    ${ONIGURUMA_LIB}
)

# Include directories for test_syntax_highlighter
if(EXISTS ${RAPIDJSON_INCLUDE_DIR})
    target_include_directories(test_syntax_highlighter PRIVATE ${RAPIDJSON_INCLUDE_DIR})
endif()

if(EXISTS ${GTEST_INCLUDE_DIR})
    target_include_directories(test_syntax_highlighter PRIVATE
        ${GTEST_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/thirdparty/gtest/googletest"
    )
    # Disable gtest's regex to avoid conflict with Oniguruma's regex_t
    target_compile_definitions(test_syntax_highlighter PRIVATE GTEST_HAS_POSIX_RE=0)
endif()

# Set output directory to build root
set_target_properties(test_syntax_highlighter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Add test - run from source directory so fixture paths work
add_test(NAME SyntaxHighlighterTests COMMAND test_syntax_highlighter WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test_syntax_highlighter)
