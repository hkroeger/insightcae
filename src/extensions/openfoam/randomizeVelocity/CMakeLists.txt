set(SRC randomizeVelocity.C)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  turbulenceModels/incompressible/LES turbulenceModels/LES/LESfilters
  meshTools
)

set(OF_LIBS finiteVolume LESfilters)

set(INCLUDE_DIRS 
  ${insight_INCLUDE_DIR}
)

setup_exe_target_OF(randomizeVelocity "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "" "")
