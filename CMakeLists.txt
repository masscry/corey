cmake_minimum_required(VERSION 3.22)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

project(corey
    VERSION 0.1
    LANGUAGES CXX
    DESCRIPTION "A coroutine-based game engine"
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(COREY_ENABLE_COVERAGE "Enable code coverage" OFF)

if (COREY_ENABLE_COVERAGE)
    if (COREY_GCOV_TOOL)
        find_program(GCOV_PATH ${COREY_GCOV_TOOL})
    endif()
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif()

include(FetchContent)

enable_testing()

add_library(setup INTERFACE)
target_compile_features(setup INTERFACE cxx_std_20)
target_compile_options(setup
    INTERFACE
        -Wall -Wextra -Werror -g -fPIC -fsanitize=address -fsanitize=leak -fsanitize=undefined
        $<$<CXX_COMPILER_ID:GNU>:-fdiagnostics-color=always>
        $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics>
)
target_include_directories(setup
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
target_link_options(setup INTERFACE -rdynamic -fsanitize=address -fsanitize=leak -fsanitize=undefined)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)
