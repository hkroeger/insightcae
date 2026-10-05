project(unittests_gui)

if (INSIGHT_BUILD_CAD)

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
    add_dependencies(IQParameterSetModel testexe_pdl)
    linkToolkitVtk(IQParameterSetModel Offscreen)
    add_test(NAME gui_IQParameterSetModel COMMAND IQParameterSetModel nogui)
    set_tests_properties(gui_IQParameterSetModel PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(CADParameterSetVisualizerScheduling CADParameterSetVisualizerScheduling.cpp)
    set_target_properties(CADParameterSetVisualizerScheduling PROPERTIES AUTOMOC ON)
    target_link_libraries(CADParameterSetVisualizerScheduling
    toolkit_gui genericmodules test_pdl_header
    Qt5::Test
    )
    add_dependencies(CADParameterSetVisualizerScheduling testexe_pdl)
    linkToolkitVtk(CADParameterSetVisualizerScheduling Offscreen)
    add_test(NAME gui_CADParameterSetVisualizerScheduling COMMAND CADParameterSetVisualizerScheduling)
    set_tests_properties(gui_CADParameterSetVisualizerScheduling PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 600)

    add_executable(ISCADScriptJobScheduling ISCADScriptJobScheduling.cpp)
    set_target_properties(ISCADScriptJobScheduling PROPERTIES AUTOMOC ON)
    target_link_libraries(ISCADScriptJobScheduling
    toolkit_gui
    Qt5::Test
    )
    linkToolkitVtk(ISCADScriptJobScheduling Offscreen)
    add_test(NAME gui_ISCADScriptJobScheduling COMMAND ISCADScriptJobScheduling)
    set_tests_properties(gui_ISCADScriptJobScheduling PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 600)

    add_executable(ParameterEditorWidget ParameterEditorWidget.cpp)
    target_link_libraries(ParameterEditorWidget
    toolkit_gui genericmodules test_pdl_header
    Qt5::Test
    )
    linkToolkitVtk(ParameterEditorWidget Offscreen)
    add_test(NAME gui_ParameterEditorWidget COMMAND ParameterEditorWidget)

    add_executable(IQFilteredParameterSetModel IQFilteredParameterSetModel.cpp sampleparameterset.h)
    target_link_libraries(IQFilteredParameterSetModel
    toolkit_gui genericmodules
    Qt5::Test
    )
    linkToolkitVtk(IQFilteredParameterSetModel Offscreen)
    target_add_PDL(IQFilteredParameterSetModel)
    add_test(NAME gui_IQFilteredParameterSetModel COMMAND IQFilteredParameterSetModel nogui)


    add_executable(IQParameterBinding IQParameterBinding.cpp)
    target_link_libraries(IQParameterBinding
    toolkit_gui
    Qt5::Test
    )
    linkToolkitVtk(IQParameterBinding Offscreen)
    add_test(NAME gui_IQParameterBinding COMMAND IQParameterBinding)
    set_tests_properties(gui_IQParameterBinding PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")


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
