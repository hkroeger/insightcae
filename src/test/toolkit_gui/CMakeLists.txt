set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Test CONFIG REQUIRED)

add_executable(test_IQParameterSetModel test_IQParameterSetModel.cpp)
target_link_libraries(test_IQParameterSetModel
 toolkit_gui genericmodules
 Qt5::Test
 )
linkToolkitVtk(test_IQParameterSetModel Offscreen)
add_test(NAME test_IQParameterSetModel COMMAND test_IQParameterSetModel)


add_executable(test_sketchSolver test_sketchSolver.cpp)
target_link_libraries(test_sketchSolver toolkit_gui)
linkToolkitVtk(test_sketchSolver Offscreen)
add_test(NAME test_sketchSolver COMMAND test_sketchSolver)
