set(PRJ kOmegaHe)

set(SRC 
 kOmegaHe.C
)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  turbulenceModels
  turbulenceModels/incompressible/RAS
  transportModels
  dynamicMesh/dynamicMesh
  dynamicMesh/dynamicFvMesh
  dynamicMesh/meshMotion/tetDecompositionMotionSolver
  dynamicMesh/meshMotion/fvMotionSolver
  meshTools
  sampling
  triSurface
  surfMesh
)

set(OF_LIBS 
finiteVolume incompressibleRASModels incompressibleTransportModels
)

set(INCLUDE_DIRS 
  ${insight_INCLUDE_DIR}
  ${ARMADILLO_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${toolkit_SOURCE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS 
#toolkit
)

# set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
# add_subdirectory(wallFunctions/hybridOmegaWallFunction2)
# add_subdirectory(wallFunctions/nutHybridWallFunction2)

set(OF_VERSIONS OF22x OF22eng OF23x)
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}")
