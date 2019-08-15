set(PRJ volumeDragfvOption)

set(SRC 
 volumeDrag.C
)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
#  turbulenceModels/incompressible/LES turbulenceModels/LES/LESfilters transportModels
#  meshTools triSurface sampling surfMesh lagrangian lagrangian/basic fileFormats fvOptions
)

set(OF_LIBS 
#finiteVolume incompressibleRASModels 
#incompressibleTransportModels meshTools triSurface LESfilters 
#surfMesh sampling fvOptions
)

set(INCLUDE_DIRS 
 ${Boost_INCLUDE_DIR}
)

set(LIBS 
)

# set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
# add_subdirectory(twoPointCorrelation)

set(OF_VERSIONS OF22x OF22eng OF23x OFplus)
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")

