project(unittests_gui)

if (INSIGHT_BUILD_CAD)

    set(CMAKE_AUTOMOC ON)
    SET(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt5Test CONFIG REQUIRED)

    add_executable(IQParameterSetModel IQParameterSetModel.cpp)
    target_link_libraries(IQParameterSetModel
    toolkit_gui genericmodules
    Qt5::Test
    )
    linkToolkitVtk(IQParameterSetModel Offscreen)
    add_test(NAME gui_IQParameterSetModel COMMAND IQParameterSetModel)


    add_executable(IQGroupingItemModel IQGroupingItemModel.cpp)
    target_link_libraries(IQGroupingItemModel
    insightcad toolkit_gui
    Qt5::Test
    )
    linkToolkitVtk(test_IQGroupingItemModel Onscreen)
    add_test(NAME gui_IQGroupingItemModel COMMAND IQGroupingItemModel nogui)

    add_executable(IQCADItemModel IQCADItemModel.cpp)
    target_link_libraries(IQCADItemModel
    insightcad toolkit_gui
    Qt5::Test
    )
    linkToolkitVtk(IQCADItemModel Offscreen)
    add_test(NAME gui_IQCADItemModel COMMAND IQCADItemModel)


endif()
