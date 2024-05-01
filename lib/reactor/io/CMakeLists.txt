project(corey-io
    VERSION 0.1
    LANGUAGES C CXX
    DESCRIPTION "Corey IO library"
)

include(ExternalProject)

message(STATUS "Get liburing")
ExternalProject_Add(liburing
    GIT_REPOSITORY https://github.com/axboe/liburing.git
    GIT_TAG liburing-2.5
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    BUILD_IN_SOURCE TRUE
    UPDATE_COMMAND ""
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/liburing-src
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/liburing-install
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=/ --cc=${CMAKE_C_COMPILER} --cxx=${CMAKE_CXX_COMPILER}
    BUILD_COMMAND ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} make -C <SOURCE_DIR> -j8
    INSTALL_COMMAND make DESTDIR=<INSTALL_DIR> -C <SOURCE_DIR> install
    BYPRODUCTS
        <INSTALL_DIR>/lib/liburing.a
        <INSTALL_DIR>/include
        <INSTALL_DIR>/include/liburing.h
)

ExternalProject_Get_Property(liburing INSTALL_DIR)

add_library(uring STATIC IMPORTED)
set_property(
    TARGET
        uring
    PROPERTY
        IMPORTED_LOCATION ${INSTALL_DIR}/lib/liburing.a
)

add_dependencies(uring liburing)

message(STATUS "${INSTALL_DIR}")

add_library(io)
target_sources(io PRIVATE io.cc)
target_link_libraries(io PUBLIC uring corey::reactor)
target_include_directories(io
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        ${INSTALL_DIR}/include
)

add_library(corey::io ALIAS io)