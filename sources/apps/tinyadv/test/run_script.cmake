# Feed a scripted session to tinyadv and check the output.
#
#   cmake -DEXE=<tinyadv> -DWORLD=<world.txt> -DINPUT=<commands.txt>
#         -DEXPECT=<text that must appear> [-DEXPECT_ERROR=ON] -P run_script.cmake
#
# Normally tinyadv must exit with 0 and EXPECT must appear on stdout. With
# EXPECT_ERROR, tinyadv must exit with an error and EXPECT must appear on
# stderr.

foreach(var EXE WORLD INPUT EXPECT)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "run_script.cmake: ${var} is not set")
    endif()
endforeach()

execute_process(
    COMMAND ${EXE} ${WORLD}
    INPUT_FILE ${INPUT}
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    RESULT_VARIABLE rc)

if(EXPECT_ERROR)
    if(rc EQUAL 0)
        message(FATAL_ERROR "tinyadv should have failed but exited with 0\n${out}")
    endif()
    set(checked "${err}")
else()
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "tinyadv exited with ${rc}\n${err}\n${out}")
    endif()
    set(checked "${out}")
endif()

string(FIND "${checked}" "${EXPECT}" pos)
if(pos EQUAL -1)
    message(FATAL_ERROR "Expected \"${EXPECT}\" in output:\n${checked}")
endif()
