# Feed a scripted session to tinyadv and check the output.
#
#   cmake -DEXE=<tinyadv> -DWORLD=<world.txt> -DINPUT=<commands.txt>
#         -DEXPECT=<text that must appear> -P run_script.cmake

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

if(NOT rc EQUAL 0)
    message(FATAL_ERROR "tinyadv exited with ${rc}\n${err}\n${out}")
endif()
string(FIND "${out}" "${EXPECT}" pos)
if(pos EQUAL -1)
    message(FATAL_ERROR "Expected \"${EXPECT}\" in output:\n${out}")
endif()
