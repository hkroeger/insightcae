set(SRC wallShearStressInter.C)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  transportModels 
  transportModels/twoPhaseMixture
  turbulenceModels
  turbulenceModels/incompressible/RAS
  turbulenceModels/incompressible/RAS/derivedFvPatchFields/wallFunctions/nutWallFunctions
  turbulenceModels/compressible/RAS/derivedFvPatchFields/wallFunctions/mutWallFunctions
  turbulenceModels/incompressible/LES 
  turbulenceModels/LES/LESfilters
  thermophysicalModels/basic
  meshTools sampling surfMesh
)

set(OF_LIBS 
finiteVolume
incompressibleTransportModels
incompressibleRASModels
meshTools
)

set(INCLUDE_DIRS 
  ${insight_INCLUDE_DIR}
)

setup_exe_target_OF(wallShearStressInter "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "" "")
