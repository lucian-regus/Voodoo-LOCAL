add_executable(presentation
        src/main.c
        src/configuration/configuration.c
)

target_link_libraries(presentation infrastructure)