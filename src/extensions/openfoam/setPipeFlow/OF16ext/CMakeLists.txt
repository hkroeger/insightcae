 
project(setPipeFlow16ext)

set(setPipeFlow16ext_SOURCES ../setPipeFlow.C)

include_directories(
  ${insight_INCLUDE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  )

setup_exe_target_OF16ext(setPipeFlow16ext "setPipeFlow"
  OpenFOAM
  Pstream
  finiteVolume
  ) 

#install(TARGETS convertPressure RUNTIME DESTINATION bin)