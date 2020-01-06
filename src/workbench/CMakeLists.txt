project(workbench)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qwt REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${QWT_INCLUDE_DIR})

set(workbench_SRCS
    resultelementwrapper.cpp
    resultelementwrapper.h
    analysisform.cpp
    analysisform.h
    analysisform_queries.cpp
    analysisform_localactions.cpp
    analysisform_remoteactions.cpp
    newanalysisdlg.cpp
    newanalysisdlg.h
    workbench.cpp
    workbench.h
    main.cpp

    workbenchaction.cpp
    workbenchaction.h
    localrun.cpp
    localrun.h

)
SET(workbench_FORMS newanalysisdlg.ui analysisform.ui xml_display.ui)
SET(workbench_RCCS workbench.qrc)
set(workbench_LIBS
    toolkit toolkit_gui
    Qt5::Widgets
    ${Boost_LIBRARIES}
    ${QWT_LIBRARY} ${QWT_LIBRARIES}
)
if (wt_FOUND)
    message(STATUS "Found wt")

    LIST(APPEND workbench_SRCS
        remoterun.cpp
        remoterun.h
    )
    LIST(APPEND workbench_LIBS
        toolkit_remote
    )
    ADD_DEFINITIONS("-DHAVE_WT")
else()
    message(STATUS "Did not found wt, disabling remote execution functions.")
endif()

add_executable(workbench ${workbench_SRCS} ${workbench_FORMS} ${workbench_RCCS})

target_link_libraries(workbench ${workbench_LIBS})

target_include_directories(workbench
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${QWT_INCLUDE_DIR}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS workbench RUNTIME DESTINATION bin)
