

add_library(openfoamtestcase openfoamtestcase.cpp)
linkToolkitVtk(openfoamtestcase Offscreen)



add_subdirectory(blockmesh_templates)
add_subdirectory(common_solver_setups)
