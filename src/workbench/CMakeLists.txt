project(workbench)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qwt REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${QWT_INCLUDE_DIR})

set(workbench_SRCS resultelementwrapper.cpp graphprogressdisplayer.cpp analysisform.cpp newanalysisdlg.cpp workbench.cpp main.cpp)
SET(workbench_FORMS newanalysisdlg.ui analysisform.ui xml_display.ui)
SET(workbench_RCCS workbench.qrc)

add_executable(workbench ${workbench_SRCS} ${workbench_FORMS} ${workbench_RCCS})

target_link_libraries(workbench
 toolkit toolkit_gui
 Qt5::Widgets
 ${Boost_LIBRARIES}
 ${QWT_LIBRARY} ${QWT_LIBRARIES}
)

target_include_directories(workbench
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${QWT_INCLUDE_DIR}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS workbench RUNTIME DESTINATION bin)
