find_package(Boost 1.74 REQUIRED
    COMPONENTS
        program_options
)

add_library(common)

target_sources(common
    PRIVATE
        console.cc
        sink.cc
)

target_link_libraries(common
    PUBLIC
        setup
        fmt::fmt
        Boost::boost
)
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(corey::common ALIAS common)
