
add_library(openfoamcasewithcylindermesh openfoamcasewithcylindermesh.cpp)
target_link_libraries(openfoamcasewithcylindermesh openfoamtestcase)


macro(test_solver TESTCPPNAME)
    add_executable(testexe_${TESTCPPNAME} ${TESTCPPNAME}.cpp)
    target_link_libraries(testexe_${TESTCPPNAME} openfoamcasewithcylindermesh)
    createAllOpenFOAMTests(unit_openfoam_${TESTCPPNAME} testexe_${TESTCPPNAME} ${ARGN})
endmacro()


##
## Numerics
##

test_solver(numerics_potentialFoam)
test_solver(numerics_potentialFreeSurfaceFoam)
test_solver(numerics_laplacianFoam)
test_solver(numerics_buoyantPimpleFoam no)
createAllOpenFOAMTests(unit_openfoam_numerics_buoyantPimpleFoam_boussinesq testexe_numerics_buoyantPimpleFoam yes)
test_solver(numerics_buoyantSimpleFoam no)
createAllOpenFOAMTests(unit_openfoam_numerics_buoyantSimpleFoam_boussinesq testexe_numerics_buoyantSimpleFoam yes)
test_solver(numerics_interFoam)
test_solver(numerics_steadyIncompressible)
test_solver(numerics_unsteadyIncompressible)
test_solver(numerics_steadyCompressible)
test_solver(numerics_unsteadyCompressible)
test_solver(numerics_reactingFoam)
test_solver(numerics_reactingParcelFoam)
test_solver(numerics_magneticFoam)
test_solver(numerics_scalarTransportFoam)

test_solver(numerics_restartwrite)

##
## Basic
##
test_solver(basic_constantpressuregradientsource)
test_solver(basic_copyfiles)
test_solver(basic_customdictentries)
test_solver(basic_fixedvalueconstraint)
test_solver(basic_limitquantities)
test_solver(basic_minimumtimesteplimit)
test_solver(basic_mirrormesh)
test_solver(basic_passivescalar)
test_solver(basic_porouszone)
test_solver(basic_pressuregradientsource)
test_solver(basic_providefields)
test_solver(basic_rangeweightfield)
test_solver(basic_setfieldsconfiguration)
test_solver(basic_srfoption)
test_solver(basic_volumedrag)

##
## Dynamic mesh
##
test_solver(dymesh_displacementfvmotionsolver)
test_solver(dymesh_rigidbodymotiondynamicmesh)
test_solver(dymesh_solidbodymotiondynamicmesh)
test_solver(dymesh_velocitytetfemmotionsolver)

##
## Thermo
##
test_solver(thermo_detailedGasReactionThermodynamics)


##
## Evaluation CEs
##
test_solver(eval_fieldAveraging)
test_solver(eval_probes)
test_solver(eval_volumeIntegrate)
test_solver(eval_surfaceIntegrate)
test_solver(eval_fieldMinMax)
test_solver(eval_cuttingPlane)
test_solver(eval_twoPointCorrelation)
test_solver(eval_cylindricalTwoPointCorrelation)
test_solver(eval_forces)
test_solver(eval_extendedForces)
test_solver(eval_catalyst)
test_solver(eval_ComputeLengthScale)


##
## Turbulence models
##
test_solver(turbmodel_laminar)
test_solver(turbmodel_Smagorinsky)
test_solver(turbmodel_oneEqEddy)
test_solver(turbmodel_dynOneEqEddy)
test_solver(turbmodel_dynSmagorinsky)
test_solver(turbmodel_kOmegaSST)
test_solver(turbmodel_kEpsilon)
test_solver(turbmodel_realizablekEpsilo)
test_solver(turbmodel_SpalartAllmaras)
test_solver(turbmodel_LEMOSHybrid)
test_solver(turbmodel_kOmegaSST_LowRe)
test_solver(turbmodel_kOmegaSST2)
test_solver(turbmodel_kOmegaHe)
test_solver(turbmodel_LRR)
test_solver(turbmodel_WALE)

##
## BC elements
##
test_solver(bc_AdiabaticBC)
test_solver(bc_FixedTemperatureBC)
test_solver(bc_ExternalWallBC)
test_solver(bc_CAFSIBC)
test_solver(bc_uniformPhases)
test_solver(bc_uniformWallTiedPhases)
test_solver(bc_CompressibleInletBC)
test_solver(bc_CyclicGGIBC)
test_solver(bc_CyclicPairBC)
test_solver(bc_EmptyBC)
test_solver(bc_ExptDataInletBC)
test_solver(bc_GGIBC)
test_solver(bc_MappedVelocityInletBC)
test_solver(bc_MassflowBC)
test_solver(bc_MixingPlaneGGIBC)
test_solver(bc_OverlapGGIBC)
test_solver(bc_SuctionInletBC)
test_solver(bc_SymmetryBC)
test_solver(bc_TurbulentVelocityInletBC)



##
## OF tools
##
test_solver(tools_setfields_fieldToCellOperator)
test_solver(tools_setfields_cellToCellOperator)
test_solver(tools_createpatch_createCyclicOperator)
test_solver(tools_sample_uniformLine)
test_solver(tools_sample_circumferentialAveragedUniformLine)
test_solver(tools_sample_linearAveragedPolyLine)
test_solver(tools_sample_linearAveragedUniformLine)
test_solver(tools_patchIntegrate)
test_solver(tools_patchArea)
test_solver(tools_HomogeneousAveragedProfile)
test_solver(tools_setSet)
test_solver(tools_setsToZones)
test_solver(tools_copyPolyMesh)
test_solver(tools_linkPolyMesh)
test_solver(tools_copyFields)
test_solver(tools_mapFields)
test_solver(tools_convertPatchPairToCyclic)
test_solver(tools_mergeMeshes)
test_solver(tools_resetMeshToLatestTimestep)
test_solver(tools_runPotentialFoam)
test_solver(tools_extrude2DMesh)
test_solver(tools_rotateMesh)
test_solver(tools_createBaffles)
test_solver(tools_removeCellSetFromMesh)
test_solver(tools_createPrismLayers)
