

set(PRJ harmonic2)

set(SRC 
 harmonic2.C 
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

#set(OF_VERSIONS OF21x OF22x OF22eng OF23x)
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}")
