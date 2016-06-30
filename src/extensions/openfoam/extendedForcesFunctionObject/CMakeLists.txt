set(PRJ extendedForcesFunctionObject)

set(SRC 
 ${PRJ}.C
)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
#  turbulenceModels/incompressible/LES turbulenceModels/LES/LESfilters transportModels
#  postProcessing/functionObjects/forces sampling fileFormats
#  meshTools triSurface sampling surfMesh
)

set(OF_LIBS 
#finiteVolume incompressibleRASModels 
#incompressibleTransportModels triSurface LESfilters 
#forces surfMesh sampling
)

set(INCLUDE_DIRS 
#   ${insight_INCLUDE_DIR}
#   ${ARMADILLO_INCLUDE_DIRS}
#   ${Boost_INCLUDE_DIRS}
#   ${toolkit_SOURCE_DIR}
#   ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS 
# toolkit
)

# set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
# add_subdirectory(faceQualityMarker)
set(OF_VERSIONS Fx31 OF22x OF23x)
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
 
