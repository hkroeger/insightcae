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


# everything except main() goes into a static library,
# so that tests can link against the workbench classes (e.g. RemoteRun)
add_library(workbench_core STATIC
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

    newanalysisdlg.ui
    analysisform.ui
    xml_display.ui
    qsetupremotedialog.ui
    qexecutionworkspacedialog.ui
    newanalysisform.ui

    ${workbench_SRCS}
)
target_enable_qt(workbench_core)
if (wt_FOUND)
    # consumers (e.g. tests) must see the same class layout and inline code
    target_compile_definitions(workbench_core PUBLIC HAVE_WT)
endif()
set_target_properties(workbench_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(workbench_core PUBLIC ${workbench_LIBS})
target_include_directories(workbench_core
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/workbench_core_autogen/include
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
# workbench.cpp is not part of the library, but shares its translation domain
target_setup_gettext(workbench_core workbench workbench.cpp)


# resources stay in the executable: their static initializers
# would be dropped by the linker, if they were in a static library
add_executable(workbench
    workbench.cpp
    workbench.qrc
)
target_enable_qt(workbench)
target_link_libraries(workbench workbench_core)
linkToolkitVtk(workbench Onscreen)

target_include_directories(workbench
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(workbench PRIVATE
    "GETTEXT_DOMAIN=\"workbench\""
    "GETTEXT_OUTPUT_DIR=\"locale\"" )

install(TARGETS workbench RUNTIME DESTINATION bin)
