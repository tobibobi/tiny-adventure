# advengine - the data-driven adventure engine (static library)

add_library(advengine STATIC
    src/game.c
    src/output.c
    src/parser.c
    src/world.c)

target_include_directories(advengine
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/headers
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(advengine PRIVATE project_options)
