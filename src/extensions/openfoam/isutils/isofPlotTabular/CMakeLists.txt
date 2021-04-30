project(isofPlotTabular)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Charts CONFIG REQUIRED)

set(isofPlotTabular_SRCS
    main.cpp
    isofplottabularwindow.cpp
    isofplottabularwindow.h
    plotwidget.cpp
    plotwidget.h
)
SET(isofPlotTabular_FORMS isofplottabularwindow.ui plotwidget.ui)
SET(isofPlotTabular_RCCS
    #isofplottabular.qrc
    )

add_executable(isofPlotTabular ${isofPlotTabular_SRCS} ${isofPlotTabular_FORMS} ${isofPlotTabular_RCCS})
target_link_libraries(isofPlotTabular
 toolkit_gui
 Qt5::Widgets Qt5::Charts
)
linkToolkitVtk(isofPlotTabular Onscreen)
target_include_directories(isofPlotTabular
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isofPlotTabular RUNTIME DESTINATION bin)

install_script(insight_isofplottabular_aliases insight_isofplottabular_aliases.sh)
