project(insightcad)

find_package(OpenCasCade REQUIRED)
find_package(OpenGL)

set(insightcad_SOURCES dxfwriter.cpp parser.cpp datum.cpp sketch.cpp geotest.cpp solidmodel.cpp )

include_directories(${toolkit_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OCC_INCLUDE_DIR})
link_directories(${OCC_LIBRARY_DIR})

add_library(insightcad SHARED ${insightcad_SOURCES})
target_link_libraries(insightcad toolkit ${OCC_LIBRARIES} dxflib)

install(TARGETS insightcad LIBRARY DESTINATION lib)

set(cadtest_SRCS test.cpp)
add_executable(cadtest ${cadtest_SRCS})
target_link_libraries(cadtest insightcad)

set(iscad_SRCS iscad.cpp iscadapplication.cpp qoccviewwidget.cpp qoccviewercontext.cpp)
find_package(Qt4 REQUIRED)
find_package(Qwt REQUIRED)
#find_package(Boost COMPONENTS system filesystem REQUIRED)
set(CMAKE_AUTOMOC ON)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
# include_directories(${toolkit_SOURCE_DIR})
# link_directories(${toolkit_BIN_DIR})

SET(iscad_FORMS ) #newanalysisdlg.ui analysisform.ui)
QT4_WRAP_UI(workbench_FORMS_HEADERS ${iscad_FORMS})
qt4_automoc(${iscad_SRCS})

add_executable(iscad ${iscad_SRCS} ${iscad_FORMS_HEADERS})
target_link_libraries(iscad insightcad ${OCC_LIBRARIES} ${OCC_OCAF_LIBRARIES} ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES} toolkit 
  ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} 
  ${QWT_LIBRARY} ${QWT_LIBRARIES}
  )
