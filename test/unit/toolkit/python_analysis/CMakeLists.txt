set(KITCHENSINK_SCRIPT_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/python_modules/kitchensink_analysis.py")

configure_file(kitchensink.ist.in kitchensink.ist @ONLY)

add_test(
    NAME unit_python_analysis_kitchensink
    COMMAND ${CMAKE_COMMAND}
        -DANALYZE_EXE=$<TARGET_FILE:analyze>
        -DIST_FILE=${CMAKE_CURRENT_BINARY_DIR}/kitchensink.ist
        -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/kitchensink_workdir
        -DSHAREDDIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DPYTHON_SITE_PACKAGES=${INSIGHT_PYTHON_PATH}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_test.cmake
)
