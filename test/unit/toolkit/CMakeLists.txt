project(unittests_toolkit)

macro(add_toolkit_test SRC)
    add_executable(${SRC} ${SRC}.cpp)
    linkToolkitVtk(${SRC} Offscreen)
    add_test(NAME unit_toolkit_${SRC} COMMAND ${SRC} ${ARGN})
endmacro()


set(test_pdl_SOURCES test_pdl.h test_pdl.cpp)
add_toolkit_test(test_pdl)
add_PDL(test_pdl "${test_pdl_SOURCES}")

add_toolkit_test(dictparser)

add_toolkit_test(basicparameterset)
add_PDL(basicparameterset "basicparameterset.h")

add_toolkit_test(simplelatex)
add_toolkit_test(meminfo)
add_toolkit_test(execution)
add_toolkit_test(chartrenderer)
add_toolkit_test(multiregion)
add_toolkit_test(filecontainer)
add_toolkit_test(toUnixPath)
add_toolkit_test(remoteExecutionConfig)
add_toolkit_test(warningbox)
add_toolkit_test(integrate_trpz)
add_toolkit_test(zipfile)
add_toolkit_test(overlappingintervals)
add_toolkit_test(codeaster_coordinatesystems)
add_toolkit_test(SpatialTransformation)
add_toolkit_test(lsdynainputdeck)
add_toolkit_test(units)
add_toolkit_test(vtkrendering_readmultiregioncase ${CMAKE_CURRENT_SOURCE_DIR})
add_toolkit_test(ParallelTimeDirectories ${CMAKE_CURRENT_SOURCE_DIR})
add_toolkit_test(cacheableentity ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(analysis_parameterstudy)
