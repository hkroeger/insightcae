
add_library(openfoamblockmesh_template_testcase openfoamblockmesh_template_testcase.cpp)
target_link_libraries(openfoamblockmesh_template_testcase openfoamtestcase)

macro(test_blockMeshTemplate classname)
    add_executable(testexe_${classname} test_${classname}.cpp)
    target_link_libraries(testexe_${classname} openfoamblockmesh_template_testcase)
    createAllOpenFOAMTests(${classname} testexe_${classname})
endmacro()


test_blockMeshTemplate(blockMeshDict_Cylinder)
test_blockMeshTemplate(blockMeshDict_Box)
test_blockMeshTemplate(blockMeshDict_Sphere)
