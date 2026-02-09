project(isofPlotTabular)



add_executable(isofPlotTabular
    main.cpp
    isofplottabularwindow.cpp
    isofplottabularwindow.h
    plotwidget.cpp
    plotwidget.h
    isofplottabularwindow.ui plotwidget.ui ${isofPlotTabular_RCCS}
)

find_package(Qt5Charts CONFIG REQUIRED)
target_enable_qt(isofPlotTabular)
target_link_libraries(isofPlotTabular
 toolkit_gui Qt5::Widgets Qt5::Charts
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
