cmake_minimum_required(VERSION 3.28)
project(presentation C)

add_executable(presentation
        src/main.c
)

target_link_libraries(presentation PUBLIC infrastructure)