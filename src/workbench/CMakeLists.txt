project(workbench)



set(workbench_LIBS
    toolkit_gui
    Qt5::Widgets
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


add_executable(workbench
    workbench.cpp

    iqcasedirectorystate.h iqcasedirectorystate.cpp
    iqworkbenchremoteexecutionstate.h iqworkbenchremoteexecutionstate.cpp
    iqexecutionworkspace.h iqexecutionworkspace.cpp

    newanalysisform.h newanalysisform.cpp
    newanalysisdlg.h newanalysisdlg.cpp

    analysisform.h analysisform.cpp
    analysisform_gui.cpp
    analysisform_localactions.cpp
    analysisform_remoteactions.cpp
    workbenchwindow.h workbenchwindow.cpp

    workbenchaction.h workbenchaction.cpp
    localrun.h localrun.cpp

    qexecutionworkspacedialog.h qexecutionworkspacedialog.cpp

    workbench.qrc

    newanalysisdlg.ui
    analysisform.ui
    xml_display.ui
    qsetupremotedialog.ui
    qexecutionworkspacedialog.ui
    newanalysisform.ui

    ${workbench_SRCS}
)
target_enable_qt(workbench)
target_link_libraries(workbench ${workbench_LIBS})
linkToolkitVtk(workbench Onscreen)

target_include_directories(workbench
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_setup_gettext(workbench workbench)

install(TARGETS workbench RUNTIME DESTINATION bin)
