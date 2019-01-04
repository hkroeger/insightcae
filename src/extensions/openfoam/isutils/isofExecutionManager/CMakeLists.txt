project(isofExecutionManager)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)

set(isofExecutionManager_SOURCES
    main.cpp
    mainwindow.cpp
    remotedirselector.cpp
)

add_executable(isofExecutionManager ${isofExecutionManager_SOURCES})
target_link_libraries(isofExecutionManager ${Boost_LIBRARIES} toolkit Qt5::Widgets toolkit_gui)

install(TARGETS isofExecutionManager RUNTIME DESTINATION bin)
