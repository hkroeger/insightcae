
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


add_subdirectory(caseelements)
