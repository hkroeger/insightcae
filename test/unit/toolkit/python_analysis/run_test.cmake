# Driver script for the kitchensink PythonAnalysis CTest test.
# Required -D variables: ANALYZE_EXE, IST_FILE, WORKDIR, SHAREDDIR, PYTHON_SITE_PACKAGES

if(NOT DEFINED ANALYZE_EXE OR NOT DEFINED IST_FILE OR NOT DEFINED WORKDIR OR NOT DEFINED SHAREDDIR OR NOT DEFINED PYTHON_SITE_PACKAGES)
    message(FATAL_ERROR "ANALYZE_EXE, IST_FILE, WORKDIR, SHAREDDIR and PYTHON_SITE_PACKAGES must all be set")
endif()

file(REMOVE_RECURSE "${WORKDIR}")
file(MAKE_DIRECTORY "${WORKDIR}")

# Explicitly set PYTHONPATH to this build tree's own site-packages rather than
# relying on the calling shell's environment - a PYTHONPATH left over from a
# different (e.g. stale, out-of-tree) build makes "Insight" unimportable, which
# PythonAnalysis silently swallows (see pythonanalysis.cpp's exception-swallowing
# catch blocks) instead of surfacing as a clear error.
execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
        "INSIGHT_GLOBALSHAREDDIRS=${SHAREDDIR}"
        "PYTHONPATH=${PYTHON_SITE_PACKAGES}"
        "${ANALYZE_EXE}" --workdir "${WORKDIR}" -x "${IST_FILE}"
    RESULT_VARIABLE ANALYZE_RESULT
)

if(NOT ANALYZE_RESULT EQUAL 0)
    message(FATAL_ERROR "analyze exited with non-zero status ${ANALYZE_RESULT}")
endif()

set(SUCCESS_FILE "${WORKDIR}/SUCCESS")
if(NOT EXISTS "${SUCCESS_FILE}")
    message(FATAL_ERROR "analyze exited 0 but ${SUCCESS_FILE} was not written - the Python analysis script did not run to completion (its exceptions are swallowed by PythonAnalysis, so a 0 exit code alone does not prove success)")
endif()

file(READ "${SUCCESS_FILE}" SUCCESS_CONTENT)
message(STATUS "kitchensink analysis succeeded:\n${SUCCESS_CONTENT}")
