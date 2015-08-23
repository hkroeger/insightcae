set(PRJ viscousForceProfile)

set(SRC ${PRJ}.C)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  meshTools
#   transportModels
#   turbulenceModels
#   turbulenceModels/LES/LESdeltas
#   thermophysicalModels/basic

)

set(OF_LIBS 
  finiteVolume
#   incompressibleTransportModels
#   incompressibleRASModels
#   incompressibleLESModels
#   basicThermophysicalModels
#   specie
#   compressibleRASModels
#   compressibleLESModels
)

set(INCLUDE_DIRS 
  ${insight_INCLUDE_DIR}
)

setup_exe_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "" "")
