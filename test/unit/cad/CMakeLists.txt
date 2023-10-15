project(unittests_cad)

if (INSIGHT_BUILD_CAD)


    macro(add_cad_test SRC)
        add_executable(${SRC} ${SRC}.cpp)
        linkToolkitVtk(${SRC} Offscreen)
        target_link_libraries(${SRC} insightcad)
        add_test(NAME unit_cad_${SRC} COMMAND ${SRC})
    endmacro(add_cad_test)

    add_cad_test(OCCtransformToOF)
    add_cad_test(gmshLSDynaExport)
    add_cad_test(sketchsolver)
    add_cad_test(parametricsketch_io)
endif()
