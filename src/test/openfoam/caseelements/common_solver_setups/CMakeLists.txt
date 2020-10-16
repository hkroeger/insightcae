
add_library(openfoamcasewithcylindermesh openfoamcasewithcylindermesh.cpp)
target_link_libraries(openfoamcasewithcylindermesh openfoamtestcase)


macro(test_solver TESTCPPNAME)
    add_executable(testexe_${TESTCPPNAME} ${TESTCPPNAME}.cpp)
    target_link_libraries(testexe_${TESTCPPNAME} openfoamcasewithcylindermesh)
    createAllOpenFOAMTests(${TESTCPPNAME} testexe_${TESTCPPNAME} ${ARGN})
endmacro()



test_solver(buoyantPimpleFoam no)
createAllOpenFOAMTests(buoyantPimpleFoam_boussinesq testexe_buoyantPimpleFoam yes)

test_solver(buoyantSimpleFoam)
test_solver(interFoam)
test_solver(MRFSimpleFoam)
test_solver(pimpleFoam)
test_solver(potentialFoam)
test_solver(potentialFreeSurfaceFoam)
test_solver(reactingFoam)
test_solver(simpleFoam)
