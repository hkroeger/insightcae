set(PRJ mappedWithSwirl)

set(SRC 
 mappedWithSwirlFixedValueFvPatchVectorField.C 
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
#   ${insight_INCLUDE_DIR}
#   ${ARMADILLO_INCLUDE_DIRS}
#   ${GSL_INCLUDE_DIR}
#   ${Boost_INCLUDE_DIRS}
#   ${toolkit_SOURCE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
#   ${fieldDataProviderOF23x_SOURCE_DIR}
)

set(LIBS 
# toolkit
)

set(IS_OF_LIBS 
#  fieldDataProvider
)

set(OF_VERSIONS OF23x)
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "${IS_OF_LIBS}")
