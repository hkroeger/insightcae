project(unittests_toolkit)

macro(add_toolkit_test SRC)
    add_executable(testexe_${SRC} ${SRC}.cpp)
    target_include_directories(testexe_${SRC} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    linkToolkitVtk(testexe_${SRC} Offscreen)
    add_test(NAME unit_${SRC} COMMAND testexe_${SRC} ${ARGN})
endmacro()

add_library(test_pdl_header INTERFACE)
target_include_directories(test_pdl_header INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(test_pdl_header INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

set(test_pdl_SOURCES test_pdl.h test_pdl.cpp)
add_executable(testexe_pdl test_pdl.cpp test_pdl.h)
add_shared_file(testexe_pdl tests
 testLibrary.xml
)
#target_include_directories(testexe_pdl PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
linkToolkitVtk(testexe_pdl Offscreen)
add_test(NAME unit_pdl COMMAND testexe_pdl)
target_add_PDL(testexe_pdl)
target_link_libraries(testexe_pdl toolkit_cad)

add_toolkit_test(toolkit_dictparser)

add_toolkit_test(toolkit_parameterset)
add_dependencies(testexe_toolkit_parameterset testexe_pdl)
target_link_libraries(testexe_toolkit_parameterset toolkit_cad) # for cadsketchparameter

add_toolkit_test(toolkit_resultset)
add_dependencies(testexe_toolkit_resultset testexe_pdl)
target_link_libraries(testexe_toolkit_resultset toolkit_cad) # for cadsketchparameter

add_toolkit_test(toolkit_parametermodification)
add_toolkit_test(toolkit_simplelatex)
add_toolkit_test(toolkit_meminfo)
add_toolkit_test(toolkit_execution)
add_toolkit_test(toolkit_chartrenderer)
add_toolkit_test(toolkit_multiregion)
add_toolkit_test(toolkit_filecontainer)
add_toolkit_test(toolkit_tounixpath)
add_toolkit_test(toolkit_warningbox)
add_toolkit_test(toolkit_linearalgebra_integrate_trpz)
add_toolkit_test(toolkit_linearalgebra_convergencebyvariance)
add_toolkit_test(toolkit_zipfile)
add_toolkit_test(toolkit_openfoamcasedirectoryparameter)
add_toolkit_test(toolkit_overlappingintervals)
add_toolkit_test(toolkit_readpostprocessingfiles)
add_toolkit_test(toolkit_codeaster_coordinatesystems)
add_toolkit_test(toolkit_spatialtransformation)
add_toolkit_test(toolkit_framedvector)

# misuse of frame-tagged points/vectors has to be rejected by the compiler:
# each case is a separate object library, which is built by the test and expected to fail.
# The case NONE is the control: it contains only valid code and has to compile.
foreach(CASE NONE
        POINT_MINUS_OTHERFRAME VECTOR_PLUS_OTHERFRAME POINT_PLUS_POINT
        SCALAR_TIMES_POINT TRSF_WRONG_FRAME COMPOSE_MISMATCH)
    add_library(compilefail_framedvector_${CASE} OBJECT EXCLUDE_FROM_ALL toolkit_framedvector_compilefail.cpp)
    target_compile_definitions(compilefail_framedvector_${CASE} PRIVATE CASE_${CASE})
    linkToolkitVtk(compilefail_framedvector_${CASE} Offscreen) # usage requirements (include dirs) only, nothing is linked
    add_test(NAME unit_toolkit_framedvector_compilefail_${CASE}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target compilefail_framedvector_${CASE})
    if (NOT CASE STREQUAL "NONE")
        set_tests_properties(unit_toolkit_framedvector_compilefail_${CASE} PROPERTIES WILL_FAIL TRUE)
    endif()
endforeach()
add_toolkit_test(toolkit_lsdynainputdeck)
add_toolkit_test(toolkit_units)
add_toolkit_test(toolkit_vtkrendering_readmultiregioncase ${CMAKE_CURRENT_SOURCE_DIR})
add_toolkit_test(toolkit_ofpolymeshreader ${CMAKE_CURRENT_SOURCE_DIR})
add_toolkit_test(toolkit_paralleltimedirectories ${CMAKE_CURRENT_SOURCE_DIR})
add_toolkit_test(toolkit_cacheableentity ${CMAKE_CURRENT_SOURCE_DIR})
add_toolkit_test(toolkit_observer_ptr)
add_toolkit_test(toolkit_supplementedquantity)
add_toolkit_test(toolkit_resultantforce)
add_toolkit_test(toolkit_predefinedfilters)
add_toolkit_test(toolkit_blockmeshquality)
add_toolkit_test(toolkit_resolutionharmonizer)
add_toolkit_test(toolkit_patternsearch)
add_toolkit_test(toolkit_periodicfacesize)

add_library(toolkit_factory_lib_base SHARED  toolkit_factory_unit2.cpp toolkit_factory_unit2.h)
target_include_directories(toolkit_factory_lib_base PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(toolkit_factory_lib_base toolkitHeaders)

add_library(toolkit_factory_lib_ext SHARED toolkit_factory_unit2.h
    toolkit_factory_unit3.h
    toolkit_factory_unit3.cpp)
target_include_directories(toolkit_factory_lib_ext PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
# the extension uses the base class from toolkit_factory_lib_base
# (DLLs need all symbols resolved at link time)
target_link_libraries(toolkit_factory_lib_ext toolkitHeaders toolkit_factory_lib_base)

add_executable(toolkit_factory toolkit_factory.cpp toolkit_factory_unit2.h toolkit_factory_unit3.h)
target_include_directories(toolkit_factory PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(toolkit_factory toolkitHeaders toolkit_factory_lib_base)
add_dependencies(toolkit_factory toolkit_factory_lib_ext) # don't actually link
add_test(NAME unit_toolkit_factory COMMAND toolkit_factory ${ARGN})

add_subdirectory(analysis_parameterstudy)
add_subdirectory(python_analysis)
