
set(setVortexVelocity_SOURCES setVortexVelocity.C)

project(setVortexVelocity16ext)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  )
setup_exe_target_OF16ext(setVortexVelocity16ext "${setVortexVelocity_SOURCES}" "setVortexVelocity" "${include_directories}"
  ${OF16ext_LIB_DIR}/libfiniteVolume.so
  )

project(setVortexVelocity22x)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
  )
setup_exe_target_OF22x(setVortexVelocity22x "${setVortexVelocity_SOURCES}" "setVortexVelocity" "${include_directories}"
  ${OF22x_LIB_DIR}/libfiniteVolume.so
  ) 