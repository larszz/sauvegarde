cmake_minimum_required(VERSION 3.5)
project(Libcdpfgl C)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-listfiles/")

set(SOURCE_FILES
        libcdpfgl.c
        options.c
        compress.c
        clock.c
        query.c
        packing.c
        database.c
        communique.c
        hashs.c
        files.c
        configuration.c

        libcdpfgl.h
        options.h
        compress.h
        clock.h
        query.h
        packing.h
        database.h
        communique.h
        hashs.h
        files.h
        configuration.h

        ../config.h
        )


# add as library
add_library(libcdpfgl ${SOURCE_FILES})


# TEST
#find_package(PkgConfig REQUIRED)
#PKG_CHECK_MODULES(GLIB REQUIRED glib-2.0)
#
#include_directories(
#        src/include/
#        ${GLIB_INCLUDE_DIRS}
#)
#link_directories(${GLIB_LIBRARY_DIRS})
#target_link_libraries(${libcdpfgl} ${GLIB_LIBRARIES})

# add glib
target_include_directories(libcdpfgl PRIVATE /usr/include/glib-2.0)
target_link_libraries(libcdpfgl PRIVATE glib-2.0)


# add gio
target_include_directories(libcdpfgl PRIVATE /usr/include/gio-2.0)
target_link_libraries(libcdpfgl PRIVATE gio-2.0)

target_link_libraries(libcdpfgl PRIVATE gobject-2.0)
target_link_libraries(libcdpfgl PRIVATE z)

#MICROHTTPD
#find_package(libmicrohttpd)
#target_link_libraries(libcdpfgl PRIVATE libmicrohttpd::libmicrohttpd)

#target_include_directories(libcdpfgl PRIVATE /usr/include/glib-2.0/)

#set_target_properties(libcdpfgl PROPERTIES LINKER_LANGUAGE C)