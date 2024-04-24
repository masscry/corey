cmake_minimum_required(VERSION 3.22)

project(corey
    VERSION 0.1
    LANGUAGES CXX
    DESCRIPTION "An coroutine-based game engine"
)

include(FetchContent)

enable_testing()

add_library(setup INTERFACE)
target_compile_features(setup INTERFACE cxx_std_20)
target_compile_options(setup
    INTERFACE
        -Wall -Wextra -Werror
        $<$<CXX_COMPILER_ID:GNU>:-fdiagnostics-color=always>
        $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics>
)
target_include_directories(setup
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)

