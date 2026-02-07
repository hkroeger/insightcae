if (INSIGHT_BUILD_ISCAD)

project(iscad)


add_executable(iscad
    iscad.cpp

    iqiscadmainwindow.cpp iqiscadmainwindow.h
    iqiscadmodelwindow.h iqiscadmodelwindow.cpp
    iqiscadmodelscriptedit.cpp iqiscadmodelscriptedit.h
    iqiscadsyntaxhighlighter.cpp iqiscadsyntaxhighlighter.h
    iqiscadbackgroundthread.cpp iqiscadbackgroundthread.h

    modelcomponentselectordlg.cpp modelcomponentselectordlg.h
    insertfeaturedlg.cpp insertfeaturedlg.h
    loadmodeldialog.cpp loadmodeldialog.h
    drawingviewsmodel.h drawingviewsmodel.cpp
    viewdefinitiondialog.h viewdefinitiondialog.cpp
    insertdrawingdialog.h insertdrawingdialog.cpp

    modelcomponentselectordlg.ui
    insertfeaturedlg.ui
    loadmodeldialog.ui
    insertdrawingdialog.ui
    viewdefinitiondialog.ui

    iscad.qrc
)
target_enable_qt(iscad)
target_link_libraries(iscad
  Qt5::Widgets
  toolkit_gui
  ${Boost_LIBRARIES}
  ${DXFLIB_LIBRARIES}
  )
linkToolkitVtk(iscad Onscreen)

target_setup_gettext(iscad iscad)

target_include_directories(iscad
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
 )

install(TARGETS iscad RUNTIME DESTINATION bin)
endif()
