
macro(createAllOpenFOAMTests TESTNAME CMD)
  version_list_OF()
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
    add_test(
        NAME ${TESTNAME}${_ofvers}
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMD} ${_ofvers} ${ARGN}
    )
   endif()
  endforeach()
endmacro()


macro(createOFTest name)
    add_executable(testexe_openfoam_${name} test_${name}.cpp)
    linkToolkitVtk(testexe_openfoam_${name} Offscreen)
    createAllOpenFOAMTests(unit_openfoam_${name} testexe_openfoam_${name} "")
endmacro()


add_subdirectory(caseelements)

createOFTest(OFtransformation)
