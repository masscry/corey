cmake_minimum_required(VERSION 3.22)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

project(corey
    VERSION 0.1
    LANGUAGES CXX
    DESCRIPTION "A coroutine-based game engine"
)

option(COREY_ENABLE_COVERAGE "Enable code coverage" OFF)

if (COREY_ENABLE_COVERAGE)
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif()

include(FetchContent)

enable_testing()

add_library(setup INTERFACE)
target_compile_features(setup INTERFACE cxx_std_20)
target_compile_options(setup
    INTERFACE
        -Wall -Wextra -Werror -g -fPIC
        $<$<CXX_COMPILER_ID:GNU>:-fdiagnostics-color=always;-Wno-error=parentheses>
        $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics>
)
target_include_directories(setup
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)
