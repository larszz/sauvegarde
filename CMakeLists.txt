cmake_minimum_required(VERSION 3.5)
project(sauvegarde C)

set(CMAKE_C_STANDARD 11)

set(EX_NAME "sgservertest")

include_directories(client)
#include_directories(libcdpfgl)
include_directories(restore)
include_directories(server)
include_directories(tools)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-listfiles/")

message("Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")
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


add_executable(
        ${EX_NAME}
        ${SERVER_SOURCES}
        ${SERVER_HEADERS}
        config.h
        )



set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${EX_NAME} PRIVATE Threads::Threads)


set_target_properties(${EX_NAME} PROPERTIES LINKER_LANGUAGE C)


target_include_directories(${EX_NAME} PRIVATE /usr/include/glib-2.0)
#target_include_directories(${EX_NAME} PRIVATE /usr/include/glib)

target_link_libraries(${EX_NAME} PRIVATE glib-2.0)
target_link_libraries(${EX_NAME} PRIVATE jansson)


# gio
target_link_libraries(${EX_NAME} PRIVATE gio-2.0)
target_include_directories(${EX_NAME} PRIVATE /usr/include/gio)


#MICROHTTPD
find_package(libmicrohttpd)
target_link_libraries(${EX_NAME} PRIVATE libmicrohttpd::libmicrohttpd)


# MONGOC
target_link_libraries(${EX_NAME} PRIVATE mongo::mongoc_shared)
find_package(mongoc-1.0 1.7 REQUIRED)

include_directories(${Libcdpfgl_SOURCE_DIR})
target_link_libraries(${EX_NAME} PRIVATE libcdpfgl)
