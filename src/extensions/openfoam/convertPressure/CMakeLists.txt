
set(convertPressure_SOURCES convertPressure.C)

project(convertPressure22x)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
  )
setup_exe_target_OF22x(convertPressure22x ${convertPressure_SOURCES} "convertPressure" "${include_directories}"
  ${OF22x_LIB_DIR}/libfiniteVolume.so
  ) 

project(convertPressure16ext)
set(include_directories
  ${insight_INCLUDE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  )
setup_exe_target_OF16ext(convertPressure16ext ${convertPressure_SOURCES} "convertPressure" "${include_directories}"
  ${OF16ext_LIB_DIR}/libfiniteVolume.so
  ) 

