project(isofCaseBuilder)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)

set(isofCaseBuilder_SRCS
    main.cpp
    insertedcaseelement.cpp
    patch.cpp
    isofcasebuilderwindow.cpp
    isofcasebuilder_actions.cpp
)
SET(isofCaseBuilder_FORMS isofcasebuilderwindow.ui)
SET(isofCaseBuilder_RCCS isofCaseBuilder.qrc)

add_executable(isofCaseBuilder ${isofCaseBuilder_SRCS} ${isofCaseBuilder_FORMS} ${isofCaseBuilder_RCCS})
target_link_libraries(isofCaseBuilder
 toolkit toolkit_gui
 taskspoolermonitor
 Qt5::Widgets
)
target_include_directories(isofCaseBuilder
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isofCaseBuilder RUNTIME DESTINATION bin)
