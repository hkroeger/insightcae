
set(randomizeVelocity_SOURCES randomizeVelocity.C)

project(randomizeVelocity16ext)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF16ext_LIBSRC_DIR}/turbulenceModels/incompressible/LES/lnInclude
  ${OF16ext_LIBSRC_DIR}/turbulenceModels/LES/LESfilters/lnInclude
  )
setup_exe_target_OF16ext(randomizeVelocity16ext "${randomizeVelocity_SOURCES}" "randomizeVelocity" "${include_directories}"
  ${OF16ext_LIB_DIR}/libfiniteVolume.so
  ${OF16ext_LIB_DIR}/libLESfilters.so
  ) 

project(randomizeVelocity22x)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF22x_LIBSRC_DIR}/meshTools/lnInclude
  ${OF22x_LIBSRC_DIR}/turbulenceModels/incompressible/LES/lnInclude
  ${OF22x_LIBSRC_DIR}/turbulenceModels/LES/LESfilters/lnInclude
  )
setup_exe_target_OF22x(randomizeVelocity22x "${randomizeVelocity_SOURCES}" "randomizeVelocity" "${include_directories}"
  ${OF22x_LIB_DIR}/libfiniteVolume.so
  ${OF22x_LIB_DIR}/libLESfilters.so
  ) 