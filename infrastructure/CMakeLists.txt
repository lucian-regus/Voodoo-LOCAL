cmake_minimum_required(VERSION 3.28)
project(infrastructure C)

add_library(infrastructure STATIC
        src/plugin_loader.c
)

target_link_libraries(infrastructure PUBLIC domain)