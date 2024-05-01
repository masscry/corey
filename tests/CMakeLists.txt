
message(STATUS "Fetching Google Tests")
FetchContent_Declare(googletest
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)

FetchContent_MakeAvailable(googletest)

add_executable(base_test)

target_sources(base_test
    PRIVATE
        test_base.cc
        test_future_promise.cc
        test_task.cc
        test_reactor.cc
)

target_link_libraries(base_test
    PRIVATE
        GTest::gtest_main
        GTest::gmock_main
        corey::utils
        corey::reactor
)

include(GoogleTest)
gtest_discover_tests(base_test)
