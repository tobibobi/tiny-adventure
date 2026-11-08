# tinyadv - terminal front end for advengine

set(TINYADV_DATA_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/tinyadv)

add_executable(tinyadv src/main.c)

target_link_libraries(tinyadv PRIVATE advengine project_options)

target_compile_definitions(tinyadv PRIVATE
    TINYADV_VERSION="${PROJECT_VERSION}"
    TINYADV_DATADIR="${CMAKE_INSTALL_PREFIX}/${TINYADV_DATA_INSTALL_DIR}")

# Keep a copy of the world next to the binary so it runs from the build tree.
add_custom_command(TARGET tinyadv POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/data/world.txt
            $<TARGET_FILE_DIR:tinyadv>/world.txt
    COMMENT "Copying world.txt next to tinyadv")

install(TARGETS tinyadv RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/world.txt DESTINATION ${TINYADV_DATA_INSTALL_DIR})

# Smoke test: play the sample world from start to finish.
if(BUILD_TESTING)
    add_test(NAME tinyadv.walkthrough
        COMMAND ${CMAKE_COMMAND}
            -DEXE=$<TARGET_FILE:tinyadv>
            -DWORLD=${CMAKE_CURRENT_SOURCE_DIR}/data/world.txt
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/test/walkthrough.txt
            -DEXPECT=You\ have\ won
            -P ${CMAKE_CURRENT_SOURCE_DIR}/test/run_script.cmake)
    add_test(NAME tinyadv.grue
        COMMAND ${CMAKE_COMMAND}
            -DEXE=$<TARGET_FILE:tinyadv>
            -DWORLD=${CMAKE_CURRENT_SOURCE_DIR}/data/world.txt
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/test/grue.txt
            -DEXPECT=eaten\ by\ a\ grue
            -P ${CMAKE_CURRENT_SOURCE_DIR}/test/run_script.cmake)
endif()
