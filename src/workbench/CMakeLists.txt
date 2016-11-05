project(workbench)

find_package(Qt4 COMPONENTS QtCore QtGui QtWebKit REQUIRED)
find_package(Qwt REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${QWT_INCLUDE_DIR})

set(workbench_SRCS resultelementwrapper.cpp graphprogressdisplayer.cpp analysisform.cpp newanalysisdlg.cpp workbench.cpp main.cpp)
SET(workbench_FORMS newanalysisdlg.ui analysisform.ui)
QT4_WRAP_UI(workbench_FORMS_HEADERS ${workbench_FORMS})
qt4_automoc(${workbench_SRCS})
SET(workbench_RCCS workbench.qrc)

add_executable(workbench ${workbench_SRCS} ${workbench_FORMS_HEADERS} ${workbench_RCCS})
target_link_libraries(workbench ${Boost_LIBRARIES} toolkit toolkit_gui
 Qt4::QtCore Qt4::QtGui Qt4::QtWebKit
 ${QWT_LIBRARY} ${QWT_LIBRARIES}
 )

install(TARGETS workbench RUNTIME DESTINATION bin)
