project(unittests_gui)

if (INSIGHT_BUILD_CAD)

    set(CMAKE_AUTOMOC ON)
    SET(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt5Test CONFIG REQUIRED)

    add_executable(IQHierarchicalDataModel iqhierarchicaldatamodeltest.cpp)
    target_link_libraries(IQHierarchicalDataModel
    toolkit_gui genericmodules test_pdl_header
    Qt5::Test
    )
    add_dependencies(IQHierarchicalDataModel testexe_pdl)
    target_include_directories(IQHierarchicalDataModel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../toolkit)
    linkToolkitVtk(IQHierarchicalDataModel Offscreen)
    add_test(NAME gui_IQHierarchicalDataModel COMMAND IQHierarchicalDataModel)

    add_executable(IQParameterSetModel IQParameterSetModel.cpp)
    target_link_libraries(IQParameterSetModel
    toolkit_gui genericmodules test_pdl_header
    Qt5::Test
    )
    linkToolkitVtk(IQParameterSetModel Offscreen)
    add_test(NAME gui_IQParameterSetModel COMMAND IQParameterSetModel)

    add_executable(IQFilteredParameterSetModel IQFilteredParameterSetModel.cpp sampleparameterset.h)
    target_link_libraries(IQFilteredParameterSetModel
    toolkit_gui genericmodules
    Qt5::Test
    )
    linkToolkitVtk(IQFilteredParameterSetModel Offscreen)
    add_PDL(IQFilteredParameterSetModel "sampleparameterset.h")
    add_test(NAME gui_IQFilteredParameterSetModel COMMAND IQFilteredParameterSetModel nogui)


    add_executable(IQGroupingItemModel IQGroupingItemModel.cpp)
    target_link_libraries(IQGroupingItemModel
    insightcad toolkit_gui
    Qt5::Test
    )
    linkToolkitVtk(IQGroupingItemModel Offscreen)
    add_test(NAME gui_IQGroupingItemModel COMMAND IQGroupingItemModel nogui)

    add_executable(IQCADItemModel IQCADItemModel.cpp)
    target_link_libraries(IQCADItemModel
    insightcad toolkit_gui
    Qt5::Test
    )
    linkToolkitVtk(IQCADItemModel Offscreen)
    add_test(NAME gui_IQCADItemModel COMMAND IQCADItemModel)

    add_executable(IQResultSetModel IQResultSetModel.cpp)
    add_dependencies(IQResultSetModel testexe_pdl)
    target_link_libraries(IQResultSetModel
    test_pdl_header toolkit_gui genericmodules
    Qt5::Test
    )
    linkToolkitVtk(IQResultSetModel Offscreen)
    add_test(NAME gui_IQResultSetModel COMMAND IQResultSetModel)

endif()
