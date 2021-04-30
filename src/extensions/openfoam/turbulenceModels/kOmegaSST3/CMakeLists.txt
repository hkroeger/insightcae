set(PRJ kOmegaSST3)

set(SRC
 kOmegaSST3.C
)

set(OF_INCLUDE_DIRS
  turbulenceModels
  turbulenceModels/incompressible/RAS
  transportModels
  dynamicMesh/dynamicMesh
  dynamicMesh/dynamicFvMesh
  dynamicMesh/meshMotion/tetDecompositionMotionSolver
  dynamicMesh/meshMotion/fvMotionSolver
)

set(OF_LIBS
)

set(INCLUDE_DIRS
  ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS
 uniof
)

set(OF_VERSIONS OFesi1806)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
