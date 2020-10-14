
add_library(openfoamcasewithcylindermesh openfoamcasewithcylindermesh.cpp)
target_link_libraries(openfoamcasewithcylindermesh openfoamtestcase)


macro(test_solver classname)
    add_executable(testexe_${classname} ${classname}.cpp)
    target_link_libraries(testexe_${classname} openfoamcasewithcylindermesh)
    createAllOpenFOAMTests(${classname} testexe_${classname})
endmacro()


test_solver(simpleFoam)
