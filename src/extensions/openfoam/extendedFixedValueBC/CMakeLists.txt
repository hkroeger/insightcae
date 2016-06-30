set(PRJ extendedFixedValueBC)

set(SRC 
 extendedFixedValueFvPatchFields.C 
)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
#  turbulenceModels/incompressible/LES turbulenceModels/LES/LESfilters transportModels
#  meshTools triSurface sampling surfMesh
)

set(OF_LIBS 
#finiteVolume incompressibleRASModels 
#incompressibleTransportModels triSurface LESfilters 
#surfMesh sampling
)

set(INCLUDE_DIRS 
  ${insight_INCLUDE_DIR}
  ${ARMADILLO_INCLUDE_DIRS}
  ${GSL_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${toolkit_SOURCE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
  ${fieldDataProviderOF23x_SOURCE_DIR}
)

set(LIBS 
toolkit
)

set(IS_OF_LIBS 
 fieldDataProvider
)

# set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
# add_subdirectory(turbulentStructures/turbulentStructure)
# add_subdirectory(turbulentStructures/hatSpot)
# add_subdirectory(turbulentStructures/gaussianSpot)
# add_subdirectory(turbulentStructures/decayingTurbulenceSpot)
# add_subdirectory(turbulentStructures/decayingTurbulenceVorton)
# add_subdirectory(turbulentStructures/anisotropicVorton)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "${IS_OF_LIBS}")
