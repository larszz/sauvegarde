cmake_minimum_required(VERSION 3.5)
project(sauvegarde C)

set(CMAKE_C_STANDARD 11)

include_directories(client)
#include_directories(libcdpfgl)
include_directories(restore)
include_directories(server)
include_directories(tools)


add_subdirectory(libcdpfgl)


set(SERVER_SOURCES
        server/server.c
        server/stats.c
        server/backend.c
        server/options.c

        )

set(SERVER_HEADERS
        server/server.h
        server/stats.h
        server/backend.h
        server/options.h
        )


add_executable(sgservertest
        ${SERVER_SOURCES}
        ${SERVER_HEADERS}
        config.h
        )


set_target_properties(sgservertest PROPERTIES LINKER_LANGUAGE C)


target_include_directories(sgservertest PRIVATE /usr/include/glib-2.0)
#target_include_directories(sgservertest PRIVATE /usr/include/glib)

target_link_libraries(sgservertest PRIVATE glib-2.0)
target_link_libraries(sgservertest PRIVATE jansson)


#MICROHTTPD
#find_package(libmicrohttpd)
include(ExternalProject)
ExternalProject_Add(
        project_libmicrohttpd
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libmicrohttpd
        URL https://mirrors.kernel.org/gnu/libmicrohttpd/libmicrohttpd-latest.tar.gz
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make
        INSTALL_COMMAND make install
        PREFIX=${CMAKE_CURRENT_BINARY_DIR}/libmicrohttpd
)
ExternalProject_Get_Property(project_libmicrohttpd install_dir)
message("Install dir of project_libmicrohttpd = ${install_dir}")


add_library(libmicrohttpd STATIC IMPORTED)
set_property(TARGET libmicrohttpd PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libmicrohttpd.a)
message("Added libmicrohttpd imported_location")

add_dependencies(libmicrohttpd project_libmicrohttpd)
include_directories(${install_dir}/include/libmicrohttpd)
target_link_libraries(sgservertest PRIVATE libmicrohttpd)

# gio
target_link_libraries(sgservertest PRIVATE gio-2.0)
target_include_directories(sgservertest PRIVATE /usr/include/gio)


# MONGOC
target_link_libraries(sgservertest PRIVATE mongo::mongoc_shared)
find_package(mongoc-1.0 1.7 REQUIRED)

include_directories(${Libcdpfgl_SOURCE_DIR})
target_link_libraries(sgservertest PRIVATE libcdpfgl)
