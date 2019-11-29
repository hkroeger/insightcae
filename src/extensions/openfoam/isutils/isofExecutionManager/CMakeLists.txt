project(isofExecutionManager)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(KF5Parts)

set(isofExecutionManager_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    remoteparaview.cpp
    remoteparaview.h
    remoteparaview.ui
)

set(isofExecutionManager_LIBS
    ${Boost_LIBRARIES} toolkit Qt5::Widgets toolkit_gui
)

set(isofExecutionManager_RCCS
    isofexecutionmanager.qrc
)

if (KF5Parts_DIR)
    message(STATUS "Found KF5Parts: ${KF5Parts_DIR}")

    LIST(APPEND isofExecutionManager_SOURCES
        terminal.cpp
        terminal.h
    )
    LIST(APPEND isofExecutionManager_LIBS
        KF5::Parts
    )
    ADD_DEFINITIONS("-DHAVE_KF5")
else()
    message(STATUS "Did not found KF5Parts, disabling embedded terminal.")
endif()


add_executable(isofExecutionManager ${isofExecutionManager_SOURCES} ${isofExecutionManager_RCCS})
target_link_libraries(isofExecutionManager ${isofExecutionManager_LIBS})
target_include_directories(isofExecutionManager
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS isofExecutionManager RUNTIME DESTINATION bin)
