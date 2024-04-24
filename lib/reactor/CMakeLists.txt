add_library(reactor)

target_sources(reactor
    PRIVATE
        reactor.cc
        task.cc
)

target_link_libraries(reactor PUBLIC common)
target_include_directories(reactor INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(corey::reactor ALIAS reactor)
