project(unittests_toolkit)

macro(add_toolkit_test SRC)
    add_executable(${SRC} ${SRC}.cpp)
    target_include_directories(${SRC} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    linkToolkitVtk(${SRC} Offscreen)
    add_test(NAME unit_${SRC} COMMAND ${SRC} ${ARGN})
endmacro()


set(test_pdl_SOURCES test_pdl.h test_pdl.cpp)
add_toolkit_test(test_pdl)
add_PDL(test_pdl "${test_pdl_SOURCES}")
target_link_libraries(test_pdl insightcad)

add_toolkit_test(toolkit_dictparser)

add_toolkit_test(toolkit_parameterset)
add_dependencies(toolkit_parameterset test_pdl)
target_link_libraries(toolkit_parameterset insightcad) # for cadsketchparameter

add_toolkit_test(toolkit_simplelatex)
add_toolkit_test(toolkit_meminfo)
add_toolkit_test(toolkit_execution)
add_toolkit_test(toolkit_chartrenderer)
add_toolkit_test(toolkit_multiregion)
add_toolkit_test(toolkit_filecontainer)
add_toolkit_test(toolkit_tounixpath)
add_toolkit_test(toolkit_remoteexecutionconfig)
add_toolkit_test(toolkit_warningbox)
add_toolkit_test(toolkit_linearalgebra_integrate_trpz)
add_toolkit_test(toolkit_zipfile)
add_toolkit_test(toolkit_overlappingintervals)
add_toolkit_test(toolkit_codeaster_coordinatesystems)
add_toolkit_test(toolkit_spatialtransformation)
add_toolkit_test(toolkit_lsdynainputdeck)
add_toolkit_test(toolkit_units)
add_toolkit_test(toolkit_vtkrendering_readmultiregioncase ${CMAKE_CURRENT_SOURCE_DIR})
add_toolkit_test(toolkit_paralleltimedirectories ${CMAKE_CURRENT_SOURCE_DIR})
add_toolkit_test(toolkit_cacheableentity ${CMAKE_CURRENT_SOURCE_DIR})
add_toolkit_test(toolkit_observer_ptr)

add_subdirectory(analysis_parameterstudy)
