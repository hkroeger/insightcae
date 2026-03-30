project(isofExecutionManager)



if (NOT WIN32)
    add_subdirectory(qtermwidget)
    list(APPEND isofExecutionManager_LIBS qtermwidget)
else()
    add_definitions(-DNO_TERMWIDGET)
endif()




add_executable(isofExecutionManager
    isofexecutionmanager.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    isofexecutionmanager.qrc
)
target_enable_qt(isofExecutionManager)
target_link_libraries(isofExecutionManager
    Qt5::Widgets toolkit_gui

    ${isofExecutionManager_LIBS})
target_include_directories(isofExecutionManager
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
linkToolkitVtk(isofExecutionManager Onscreen)

install(TARGETS isofExecutionManager RUNTIME DESTINATION bin)
