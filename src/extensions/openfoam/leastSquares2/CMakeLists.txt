

set(PRJ leastSquares2)

set(SRC 
 leastSquares2Grads.C 
 leastSquares2Vectors.C
)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  meshTools triSurface sampling surfMesh
)

set(OF_LIBS 
finiteVolume incompressibleRASModels 
incompressibleTransportModels triSurface LESfilters 
surfMesh sampling)

set(INCLUDE_DIRS 
)

set(LIBS 
)

# set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
# add_subdirectory(turbulentStructures/turbulentStructure)
# add_subdirectory(turbulentStructures/hatSpot)
# add_subdirectory(turbulentStructures/homogeneousTurbulenceVorton)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
