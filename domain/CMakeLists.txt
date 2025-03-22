find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)

include_directories(${GLIB_INCLUDE_DIRS})

add_library(domain STATIC
        src/database.c
)

target_include_directories(domain PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(domain
        odbc
        ${GLIB_LIBRARIES}
)
