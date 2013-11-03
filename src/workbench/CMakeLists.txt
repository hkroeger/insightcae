project(workbench)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
set(CMAKE_AUTOMOC ON)
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g")

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
include_directories(/home/hannes/Projekte/insight/src/toolkit)
link_directories(/home/hannes/Projekte/insight/src/toolkit/build)

set(workbench_SRCS parameterwrapper.cpp analysisform.cpp newanalysisdlg.cpp workbench.cpp main.cpp)
SET(workbench_FORMS newanalysisdlg.ui analysisform.ui)
QT4_WRAP_UI(workbench_FORMS_HEADERS ${workbench_FORMS})
qt4_automoc(${workbench_SRCS})

add_executable(workbench ${workbench_SRCS} ${workbench_FORMS_HEADERS})
target_link_libraries(workbench ${Boost_LIBRARIES} toolkit ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
install(TARGETS workbench RUNTIME DESTINATION bin)
