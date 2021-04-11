include(GNUInstallDirs)

# Based on the description of Dominik Berner, see https://dominikberner.ch/cmake-find-library/

set(PATH_PF "${PROJECT_SOURCE_DIR}/prebuilt-libs/")
message("Path: ${PATH_PF}")

find_library(
        LIBMICROHTTPD_LIBRARY
        NAMES libmicrohttpd.a
#        HINTS ${PROJECT_BINARY_DIR}/prebuilt-libs/
        HINTS ${PATH_PF}
        PATH_SUFFIXES lib/
)

find_path(LIBMICROHTTPD_INCLUDE_DIR
        NAMES microhttpd.h
#        HINTS ${PROJECT_BINARY_DIR}/prebuilt-libs/
        HINTS ${PATH_PF}
        PATH_SUFFIXES include/
        )

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
        libmicrohttpd
        DEFAULT_MSG
        LIBMICROHTTPD_LIBRARY
        LIBMICROHTTPD_INCLUDE_DIR
)

mark_as_advanced(LIBMICROHTTPD_LIBRARY LIBMICROHTTPD_INCLUDE_DIR)

if(LIBMICROHTTPD_FOUND AND NOT TARGET libmicrohttpd::libmicrohttpd)
    add_library(libmicrohttpd::libmicrohttpd SHARED IMPORTED)
    set_target_properties(
            libmicrohttpd::libmicrohttpd
            PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LIBMICROHTTPD_INCLUDE_DIR}"
            IMPORTED_LOCATION ${LIBMICROHTTPD_LIBRARY})
endif()