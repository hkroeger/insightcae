if (INSIGHT_BUILD_ISCAD)

project(iscad)

set(iscad_SRCS 
    iscad.cpp
    iscadmainwindow.cpp
    iscadsyntaxhighlighter.cpp
    iscadmodel.cpp

    modelcomponentselectordlg.cpp
    insertfeaturedlg.cpp
    loadmodeldialog.cpp

    bgparsingthread.cpp
)

set(iscad_FORMS
    modelcomponentselectordlg.ui
    insertfeaturedlg.ui
    loadmodeldialog.ui
    )

set(iscad_RCCS
    iscad.qrc
    )



set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)


add_executable(iscad ${iscad_SRCS} ${iscad_FORMS} ${iscad_RCCS})

target_link_libraries(iscad
  Qt5::Widgets
  toolkit_gui insightcad_gui
  ${OCC_LIBRARIES} ${OCC_OCAF_LIBRARIES} ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES}
  ${DXFLIB_LIBRARIES}
  )
linkToolkitVtk(iscad Onscreen)

target_include_directories(iscad
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
 )

install(TARGETS iscad RUNTIME DESTINATION bin)
endif()
