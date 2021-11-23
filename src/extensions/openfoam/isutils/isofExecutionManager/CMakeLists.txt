project(isofExecutionManager)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(isofExecutionManager_LIBS
    ${Boost_LIBRARIES} Qt5::Widgets toolkit_gui
)

if (NOT WIN32)
    add_subdirectory(qtermwidget)
    list(APPEND isofExecutionManager_LIBS qtermwidget)
else()
    add_definitions(-DNO_TERMWIDGET)
endif()

find_package(Qt5Widgets CONFIG REQUIRED)

set(isofExecutionManager_SOURCES
    isofexecutionmanager.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)



set(isofExecutionManager_RCCS
    isofexecutionmanager.qrc
)


add_executable(isofExecutionManager ${isofExecutionManager_SOURCES} ${isofExecutionManager_RCCS})
target_link_libraries(isofExecutionManager ${isofExecutionManager_LIBS})
target_include_directories(isofExecutionManager
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
linkToolkitVtk(isofExecutionManager Onscreen)

install(TARGETS isofExecutionManager RUNTIME DESTINATION bin)
