add_subdirectory(analysispreview3d)
add_subdirectory(graphprogressdisplayer)
add_subdirectory(views)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Test CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)

add_executable(test_gui_constrainedsketcher constrainedsketcher.cpp)
target_link_libraries(test_gui_constrainedsketcher
 insightcad toolkit_gui
 Qt5::Test
 )
linkToolkitVtk(test_gui_constrainedsketcher Onscreen)

add_executable(cadviewertest cadviewerwindow.h cadviewerwindow.cpp)

target_link_libraries(cadviewertest
      Qt5::Widgets
      toolkit_gui
      )
linkToolkitVtk(cadviewertest Onscreen)



add_executable(fileselectiondialog fileselectiondialog.cpp)

target_link_libraries(fileselectiondialog
      Qt5::Widgets
      toolkit_gui
      )
linkToolkitVtk(fileselectiondialog Onscreen)
