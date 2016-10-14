
add_subdirectory(generic)
if (INSIGHT_BUILD_OPENFOAM)
    add_subdirectory(testcases)
endif()
