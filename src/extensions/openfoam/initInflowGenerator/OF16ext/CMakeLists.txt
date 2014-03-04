 
project(initInflowGenerator16ext)

set(initInflowGenerator16ext_SOURCES ../initInflowGenerator.C)

include_directories(
  "${inflowGeneratorBC16ext_SOURCE_DIR}/.."
  ${insight_INCLUDE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  )

setup_exe_target_OF16ext(initInflowGenerator16ext "initInflowGenerator"
  OpenFOAM
  Pstream
  finiteVolume
  inflowGeneratorBC16ext
  ) 

#install(TARGETS convertPressure RUNTIME DESTINATION bin)