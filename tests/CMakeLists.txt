if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options( -Wno-error=restrict)
endif()

message(STATUS "Fetching Google Tests")
FetchContent_Declare(googletest
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)

FetchContent_MakeAvailable(googletest)

add_executable(base_test)

target_sources(base_test
    PRIVATE
        test_log.cc
        test_defer.cc
        test_future_promise.cc
        test_task.cc
        test_reactor.cc
        test_app.cc
        test_sync.cc
        test_socket.cc
        test_signal.cc
)

target_link_libraries(base_test
    PRIVATE
        GTest::gtest_main
        GTest::gmock_main
        corey::corey
)

include(GoogleTest)
gtest_discover_tests(base_test)

if (COREY_ENABLE_COVERAGE)
    setup_target_for_coverage_lcov(
        NAME coverage
        EXECUTABLE base_test
        DEPENDENCIES base_test
        EXCLUDE "/usr/*" "${CMAKE_SOURCE_DIR}/src/*" "${CMAKE_BINARY_DIR}/_deps/*" "${CMAKE_BINARY_DIR}/lib/reactor/io/*" "${CMAKE_SOURCE_DIR}/CMakeCCompilerId.c"
    )
endif()