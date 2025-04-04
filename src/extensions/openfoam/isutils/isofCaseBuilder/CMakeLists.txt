project(isofCaseBuilder)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)

set(isofCaseBuilder_SRCS
    isofcasebuilder.cpp


    caseelementdata.h caseelementdata.cpp
    insertedcaseelement.cpp insertedcaseelement.h
    patch.cpp patch.h
    isofcasebuilderwindow.cpp isofcasebuilderwindow.h isofcasebuilderwindow.ui
    isofcasebuilder_actions.cpp isofcasebuilder_patches.cpp
    availablecaseelementsmodel.h availablecaseelementsmodel.cpp
    availablebcsmodel.h availablebcsmodel.cpp
    caseconfigurationmodel.h caseconfigurationmodel.cpp
    boundaryconfigurationmodel.h boundaryconfigurationmodel.cpp
)
SET(isofCaseBuilder_FORMS isofcasebuilderwindow.ui)
SET(isofCaseBuilder_RCCS isofCaseBuilder.qrc)

add_executable(isofCaseBuilder ${isofCaseBuilder_SRCS} ${isofCaseBuilder_FORMS} ${isofCaseBuilder_RCCS})
target_link_libraries(isofCaseBuilder
 toolkit_gui
 taskspoolermonitor
 Qt5::Widgets
)
linkToolkitVtk(isofCaseBuilder Onscreen)
target_include_directories(isofCaseBuilder
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isofCaseBuilder RUNTIME DESTINATION bin)
