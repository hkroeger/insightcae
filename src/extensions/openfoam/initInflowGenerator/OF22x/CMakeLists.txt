 
project(initInflowGenerator22x)

set(initInflowGenerator22x_SOURCES ../initInflowGenerator.C)

include_directories(
  "${inflowGeneratorBC22x_SOURCE_DIR}/.."
  ${insight_INCLUDE_DIR}
  ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF22x_LIBSRC_DIR}/meshTools/lnInclude
  )

setup_exe_target_OF22x(initInflowGenerator22x "initInflowGenerator"
  OpenFOAM
  Pstream
  finiteVolume
  inflowGeneratorBC22x
  ) 

#install(TARGETS convertPressure RUNTIME DESTINATION bin)