project(isofPlotTabular)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qwt REQUIRED)

set(isofPlotTabular_SRCS
    main.cpp
    isofplottabularwindow.cpp
    plotwidget.cpp
)
SET(isofPlotTabular_FORMS isofplottabularwindow.ui plotwidget.ui)
SET(isofPlotTabular_RCCS
    #isofplottabular.qrc
    )

add_executable(isofPlotTabular ${isofPlotTabular_SRCS} ${isofPlotTabular_FORMS} ${isofPlotTabular_RCCS})
target_link_libraries(isofPlotTabular
 toolkit toolkit_gui
 ${QWT_LIBRARY} ${QWT_LIBRARIES}
 Qt5::Widgets
)
target_include_directories(isofPlotTabular
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${QWT_INCLUDE_DIR}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isofPlotTabular RUNTIME DESTINATION bin)
