# cmake -DPATCH=<file> -DSOURCE=<dir> -P ApplyPatch.cmake
# A tree that already carries the patch passes untouched. One that fits it neither way stops the configure.
find_program(GIT_EXECUTABLE NAMES git REQUIRED)

execute_process(COMMAND ${GIT_EXECUTABLE} apply --check --reverse ${PATCH}
        WORKING_DIRECTORY ${SOURCE}
        RESULT_VARIABLE applied
        OUTPUT_QUIET ERROR_QUIET)

if (applied EQUAL 0)
    return()
endif ()

execute_process(COMMAND ${GIT_EXECUTABLE} apply ${PATCH}
        WORKING_DIRECTORY ${SOURCE}
        RESULT_VARIABLE result
        ERROR_VARIABLE error)

if (NOT result EQUAL 0)
    message(FATAL_ERROR "${PATCH} no longer fits ${SOURCE} and needs re-anchoring for this tag:\n${error}")
endif ()

message(STATUS "Patched ${SOURCE} with ${PATCH}")
