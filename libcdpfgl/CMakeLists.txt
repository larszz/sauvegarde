cmake_minimum_required(VERSION 3.5)
project(Libcdpfgl C)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-listfiles/")

set(SOURCE_FILES
        libcdpfgl.c
        configuration.c
        communique.c
        files.c
        hashs.c
        unpacking.c
        packing.c
        database.c
        query.c
        clock.c
        compress.c
        options.c
        )

set(HEADER_FILES
        libcdpfgl.h
        configuration.h
        communique.h
        files.h
        hashs.h
        unpacking.h
        packing.h
        database.h
        query.h
        clock.h
        compress.h
        options.h

        ../config.h
        )


# add as library
add_library(libcdpfgl
        ${HEADER_FIELS}
        ${SOURCE_FILES}
        )


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

# gobject
target_link_libraries(libcdpfgl PRIVATE gobject-2.0)

target_link_libraries(libcdpfgl PRIVATE z)

# sqlite
target_link_libraries(libcdpfgl PRIVATE sqlite3)

# curl
target_link_libraries(libcdpfgl PRIVATE curl)

#MICROHTTPD
#find_package(libmicrohttpd)
#target_link_libraries(libcdpfgl PRIVATE libmicrohttpd::libmicrohttpd)

#target_include_directories(libcdpfgl PRIVATE /usr/include/glib-2.0/)

#set_target_properties(libcdpfgl PROPERTIES LINKER_LANGUAGE C)