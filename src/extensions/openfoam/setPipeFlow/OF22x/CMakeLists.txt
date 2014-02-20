 
project(setPipeFlow22x)

set(setPipeFlow22x_SOURCES ../setPipeFlow.C)

include_directories(
  ${insight_INCLUDE_DIR}
  ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
  )

setup_exe_target_OF22x(setPipeFlow22x "setPipeFlow"
  OpenFOAM
  Pstream
  finiteVolume
  ) 

#install(TARGETS convertPressure RUNTIME DESTINATION bin)