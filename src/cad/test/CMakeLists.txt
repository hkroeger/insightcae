
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})


if(INSIGHT_BUILD_TESTS)

    set(cadviewertest_SRC
        cadviewerwindow.h cadviewerwindow.cpp
    )
    set(CMAKE_AUTOMOC ON)
    SET(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt5Widgets CONFIG REQUIRED)

    add_executable(cadviewertest ${cadviewertest_SRC})

    target_link_libraries(cadviewertest
      Qt5::Widgets
      toolkit_gui
      )
    linkToolkitVtk(cadviewertest Onscreen)



    macro(add_cad_test SRC)
        add_executable(${SRC} ${SRC}.cpp)
        linkToolkitVtk(${SRC} Offscreen)
        target_link_libraries(${SRC}
          insightcad
          )
        add_test(NAME ${SRC} COMMAND ${SRC} ${ARGN})
    endmacro()

    add_cad_test(parametricsketch_io)


endif()
