project(isofExecutionManager)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(qtermwidget)

find_package(Qt5Widgets CONFIG REQUIRED)

set(isofExecutionManager_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

set(isofExecutionManager_LIBS
    ${Boost_LIBRARIES} toolkit Qt5::Widgets toolkit_gui
)

set(isofExecutionManager_RCCS
    isofexecutionmanager.qrc
)


add_executable(isofExecutionManager ${isofExecutionManager_SOURCES} ${isofExecutionManager_RCCS})
target_link_libraries(isofExecutionManager ${isofExecutionManager_LIBS} qtermwidget)
target_include_directories(isofExecutionManager
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS isofExecutionManager RUNTIME DESTINATION bin)
