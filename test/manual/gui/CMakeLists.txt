add_subdirectory(analysispreview3d)
add_subdirectory(graphprogressdisplayer)


set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Test CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)

add_executable(constrainedsketcher constrainedsketcher.cpp)
target_link_libraries(constrainedsketcher
 insightcad toolkit_gui
 Qt5::Test
 )
linkToolkitVtk(constrainedsketcher Onscreen)

add_executable(cadviewertest cadviewerwindow.h cadviewerwindow.cpp)

target_link_libraries(cadviewertest
      Qt5::Widgets
      toolkit_gui
      )
linkToolkitVtk(cadviewertest Onscreen)
