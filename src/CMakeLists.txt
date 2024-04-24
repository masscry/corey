
add_executable(demo)

target_sources(demo
    PRIVATE
        demo.cc
)

target_link_libraries(demo PRIVATE
    corey::reactor
    corey::utils
)

