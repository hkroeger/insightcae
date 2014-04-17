

set(PRJ LESFunctionObjects)

set(SRC 
)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  turbulenceModels/incompressible/LES turbulenceModels/LES/LESfilters transportModels
  meshTools triSurface sampling surfMesh lagrangian lagrangian/basic fileFormats
)

set(OF_LIBS 
finiteVolume incompressibleRASModels 
incompressibleTransportModels meshTools triSurface LESfilters 
surfMesh sampling)

set(INCLUDE_DIRS 
)

set(LIBS 
)

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
add_subdirectory(twoPointCorrelation)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}")

