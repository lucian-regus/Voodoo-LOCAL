add_library(infrastructure STATIC
        src/plugins.c
)

target_include_directories(infrastructure PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(infrastructure PUBLIC domain)