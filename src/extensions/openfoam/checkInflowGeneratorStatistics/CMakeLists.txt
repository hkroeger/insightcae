set(PRJ checkInflowGeneratorStatistics)

set(SRC checkInflowGeneratorStatistics.C)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  turbulenceModels/incompressible/LES turbulenceModels/LES/LESfilters
  meshTools sampling surfMesh
)

set(OF_LIBS finiteVolume LESfilters sampling)

set(INCLUDE_DIRS 
  ${ARMADILLO_INCLUDE_DIRS}
  ${toolkit_SOURCE_DIR}
  ${testcases_SOURCE_DIR}
  ${refdata_SOURCE_DIR}
  ${inflowGeneratorBCOF23x_SOURCE_DIR}
  ${fieldDataProviderOF23x_SOURCE_DIR}
  ${insight_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
)

set(LIBS
 testcases
 refdata
)
set(OF16ext_LIBS inflowGeneratorBCOF16ext)
set(OF21x_LIBS inflowGeneratorBCOF21x)
set(OF22x_LIBS inflowGeneratorBCOF22x)
set(OF22eng_LIBS inflowGeneratorBCOF22eng)
set(OF23x_LIBS inflowGeneratorBCOF23x)

setup_exe_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}")
