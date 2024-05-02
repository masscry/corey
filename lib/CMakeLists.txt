add_subdirectory(common)
add_subdirectory(utils)
add_subdirectory(reactor)

message(STATUS "Fetching cxxopts")
FetchContent_Declare(cxxopts
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.2.1
)
FetchContent_MakeAvailable(cxxopts)

add_library(corey)

target_sources(corey
    PRIVATE
        corey.cc
)

target_link_libraries(corey
    PUBLIC
        cxxopts::cxxopts
        corey::common
        corey::reactor
        corey::utils
)

add_library(corey::corey ALIAS corey)
