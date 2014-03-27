
set(PRJ perturbU)
set(${PRJ}_SOURCES perturbU.C)

project(${PRJ}16ext)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF16ext_LIBSRC_DIR}/turbulenceModels/incompressible/LES/lnInclude
  ${OF16ext_LIBSRC_DIR}/turbulenceModels/LES/LESfilters/lnInclude
  ${OF16ext_LIBSRC_DIR}/meshTools/lnInclude  
  ${OF16ext_LIBSRC_DIR}/sampling/lnInclude  
  ${OF16ext_LIBSRC_DIR}/surfMesh/lnInclude  
  )
setup_exe_target_OF16ext(${PRJ}16ext "${${PRJ}_SOURCES}" "${PRJ}" "${include_directories}"
  ${OF16ext_LIB_DIR}/libfiniteVolume.so
  ${OF16ext_LIB_DIR}/libLESfilters.so
  ${OF16ext_LIB_DIR}/libsampling.so
  ) 

project(${PRJ}22x)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF22x_LIBSRC_DIR}/meshTools/lnInclude
  ${OF22x_LIBSRC_DIR}/turbulenceModels/incompressible/LES/lnInclude
  ${OF22x_LIBSRC_DIR}/turbulenceModels/LES/LESfilters/lnInclude
  ${OF22x_LIBSRC_DIR}/sampling/lnInclude  
  ${OF22x_LIBSRC_DIR}/surfMesh/lnInclude  
  )
setup_exe_target_OF22x(${PRJ}22x "${${PRJ}_SOURCES}" "${PRJ}" "${include_directories}"
  ${OF22x_LIB_DIR}/libfiniteVolume.so
  ${OF22x_LIB_DIR}/libLESfilters.so
  ${OF22x_LIB_DIR}/libsampling.so
)

project(${PRJ}23x)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF23x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF23x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF23x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF23x_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF23x_LIBSRC_DIR}/meshTools/lnInclude
  ${OF23x_LIBSRC_DIR}/turbulenceModels/incompressible/LES/lnInclude
  ${OF23x_LIBSRC_DIR}/turbulenceModels/LES/LESfilters/lnInclude
  ${OF23x_LIBSRC_DIR}/sampling/lnInclude  
  ${OF23x_LIBSRC_DIR}/surfMesh/lnInclude  
  )
setup_exe_target_OF23x(${PRJ}23x "${${PRJ}_SOURCES}" "${PRJ}" "${include_directories}"
  ${OF23x_LIB_DIR}/libfiniteVolume.so
  ${OF23x_LIB_DIR}/libLESfilters.so
  ${OF23x_LIB_DIR}/libsampling.so
) 