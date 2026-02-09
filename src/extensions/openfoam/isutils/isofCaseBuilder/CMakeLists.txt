project(isofCaseBuilder)




add_executable(isofCaseBuilder
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

    isofcasebuilderwindow.ui
    isofCaseBuilder.qrc
)
target_enable_qt(isofCaseBuilder)
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
