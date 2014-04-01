

set(initInflowGenerator_SOURCES initInflowGenerator.C)

if (OF16ext_FOUND)

project(initInflowGenerator16ext)
set(include_directories
  ${inflowGeneratorBC16ext_SOURCE_DIR}
  ${insight_INCLUDE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  )
setup_exe_target_OF16ext(initInflowGenerator16ext "${initInflowGenerator_SOURCES}" "initInflowGenerator" "${include_directories}"
  ${OF16ext_LIB_DIR}/libfiniteVolume.so
  inflowGeneratorBC16ext
  ) 
endif()

project(initInflowGenerator22x)
set(include_directories
  ${inflowGeneratorBC22x_SOURCE_DIR}
  ${insight_INCLUDE_DIR}
  ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF22x_LIBSRC_DIR}/meshTools/lnInclude
  )
setup_exe_target_OF22x(initInflowGenerator22x "${initInflowGenerator_SOURCES}" "initInflowGenerator" "${include_directories}"
  ${OF22x_LIB_DIR}/libfiniteVolume.so
  inflowGeneratorBC22x
  ) 