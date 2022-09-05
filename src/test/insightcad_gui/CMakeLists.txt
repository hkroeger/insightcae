set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Test CONFIG REQUIRED)

add_executable(test_IQCADItemModel test_IQCADItemModel.cpp)
target_link_libraries(test_IQCADItemModel
 insightcad insightcad_gui
 Qt5::Test
 )
linkToolkitVtk(test_IQCADItemModel Offscreen)
add_test(NAME test_IQCADItemModel COMMAND test_IQCADItemModel)
