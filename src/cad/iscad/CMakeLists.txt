if (INSIGHT_BUILD_WORKBENCH)

project(iscad)

set(iscad_SRCS 
 cadfeaturetransient.cpp  iscad.cpp                   pointertransient.cpp  qoccviewercontext.cpp
 datumlist.cpp            iscadmainwindow.cpp         qdatumitem.cpp        qoccviewwidget.cpp
 evaluationlist.cpp       iscadsyntaxhighlighter.cpp  qevaluationitem.cpp   qvariableitem.cpp
 iscadapplication.cpp     modelsteplist.cpp           qmodelstepitem.cpp    viewstate.cpp
)
find_package(Qt4 REQUIRED)
find_package(Qwt REQUIRED)
#find_package(Boost COMPONENTS system filesystem REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${toolkit_SOURCE_DIR})
# link_directories(${toolkit_BIN_DIR})

SET(iscad_FORMS ) #newanalysisdlg.ui analysisform.ui)

SET(iscad_RCCS iscad.qrc)

# QT4_WRAP_UI(workbench_FORMS_HEADERS ${iscad_FORMS})
qt4_automoc(${iscad_SRCS})

add_executable(iscad ${iscad_SRCS} ${iscad_FORMS_HEADERS} ${iscad_RCCS})
target_link_libraries(iscad insightcad ${OCC_LIBRARIES} ${OCC_OCAF_LIBRARIES} ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES} toolkit 
  ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} 
  ${QWT_LIBRARY} ${QWT_LIBRARIES}
  ${DXFLIB_LIBRARIES}
  )
install(TARGETS iscad RUNTIME DESTINATION bin)  
endif()