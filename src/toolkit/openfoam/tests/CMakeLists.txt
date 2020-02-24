
casebuilder_test_OF(test_steadyIncompressibleNumerics       test_steadyIncompressibleNumerics.iscb)
casebuilder_test_OF(test_unsteadyIncompressibleNumerics     test_unsteadyIncompressibleNumerics.iscb)
casebuilder_test_OF(test_multiphaseNumerics                 test_multiphaseNumerics.iscb)

casebuilder_test_OF(test_unsteadyCompressibleNumerics_sonicFoam         test_unsteadyCompressibleNumerics_sonicFoam.iscb)
casebuilder_test_OF(test_unsteadyCompressibleNumerics_rhoPimpleFoam     test_unsteadyCompressibleNumerics_rhoPimpleFoam.iscb)


add_executable(test_remoteexecution test_remoteexecution.cpp)
target_link_libraries(test_remoteexecution toolkit)
add_test(NAME test_remoteexecution COMMAND test_remoteexecution)
