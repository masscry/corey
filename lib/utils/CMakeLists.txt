message(STATUS "Fetching fmtlib")
FetchContent_Declare(fmt
    URL https://github.com/fmtlib/fmt/releases/download/10.2.1/fmt-10.2.1.zip
)
FetchContent_MakeAvailable(fmt)

add_library(utils)

target_sources(utils
    PRIVATE
        log.cc
)

target_link_libraries(utils
    PUBLIC
        common
        fmt::fmt
)
target_include_directories(utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(corey::utils ALIAS utils)