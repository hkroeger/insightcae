project(test_toolkit)

macro(add_toolkit_test SRC)
    add_executable(${SRC} ${SRC}.cpp)
    linkToolkitVtk(${SRC} Offscreen)
    target_link_libraries(${SRC} insightcad)
    add_test(NAME ${SRC} COMMAND ${SRC})
endmacro()


add_toolkit_test(test_OCCtransformToOF)
add_toolkit_test(test_gmshLSDynaExport)
