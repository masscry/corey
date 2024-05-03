
add_executable(demo-cp)
target_sources(demo-cp
    PRIVATE
        demo-cp.cc
)
target_link_libraries(demo-cp PRIVATE
    corey::corey
)

add_executable(demo-echo)
target_sources(demo-echo
    PRIVATE
        demo-echo.cc
)
target_link_libraries(demo-echo PRIVATE
    corey::corey
)
