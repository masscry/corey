add_subdirectory(io)

add_library(reactor)

target_sources(reactor
    PRIVATE
        reactor.cc
        task.cc
        timer.cc
        sync.cc
        signal.cc
)

target_link_libraries(reactor PUBLIC
    common
    corey::io
)
target_include_directories(reactor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(corey::reactor ALIAS reactor)
