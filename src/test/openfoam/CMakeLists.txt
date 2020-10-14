
macro(createAllOpenFOAMTests TESTNAME cmd)
  version_list_OF()
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
    add_test(
        NAME ${TESTNAME}${_ofvers}
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${cmd} ${_ofvers}
    )
   endif()
  endforeach()
endmacro()


add_subdirectory(LESmodels)
add_subdirectory(simpleFoam)

add_subdirectory(caseelements)
