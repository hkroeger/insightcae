
if (INSIGHT_BUILD_GUICOMPONENTS)
    add_subdirectory(generic)
endif()

if (INSIGHT_BUILD_OPENFOAM)
    add_subdirectory(testcases)
endif()
