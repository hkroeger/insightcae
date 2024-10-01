
# these binaries should go into common bin because they shall be shipped with the install package
add_subdirectory(installation)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})


if(INSIGHT_BUILD_TESTS)
    add_subdirectory(unit)
    add_subdirectory(integration)
    add_subdirectory(manual)
endif()
