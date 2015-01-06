
set(SRC applyBL.C)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  turbulenceModels/incompressible/LES turbulenceModels/LES/LESfilters
  meshTools sampling surfMesh 
  transportModels transportModels/incompressible
  turbulenceModels turbulenceModels/incompressible/RAS
)

set(OF_LIBS 
finiteVolume LESfilters sampling 
incompressibleTransportModels incompressibleRASModels
)

set(INCLUDE_DIRS 
  ${insight_INCLUDE_DIR}
)

setup_exe_target_OF(applyBL "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "")