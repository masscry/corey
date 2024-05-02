
add_executable(demo-cp)

target_sources(demo-cp
    PRIVATE
        demo-cp.cc
)

target_link_libraries(demo-cp PRIVATE
    corey::corey
)

