
set(setPipeFlow_SOURCES setPipeFlow.C)

project(setPipeFlow16ext)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  )
setup_exe_target_OF16ext(setPipeFlow16ext "${setPipeFlow_SOURCES}" "setPipeFlow" "${include_directories}"
  ${OF16ext_LIB_DIR}/libfiniteVolume.so
  )

project(setPipeFlow22x)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
  )
setup_exe_target_OF22x(setPipeFlow22x "${setPipeFlow_SOURCES}" "setPipeFlow" "${include_directories}"
  ${OF22x_LIB_DIR}/libfiniteVolume.so
  ) 