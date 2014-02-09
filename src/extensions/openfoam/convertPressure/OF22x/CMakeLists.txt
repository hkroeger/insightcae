 
project(convertPressure22x)

set(convertPressure22x_SOURCES ../convertPressure.C)

include_directories(
  ${insight_INCLUDE_DIR}
  ${journalbearingcommon22x_SOURCE_DIR}
  ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
  )

setup_exe_target_OF22x(convertPressure22x "convertPressure"
  OpenFOAM
  Pstream
  finiteVolume
  ) 

#install(TARGETS convertPressure RUNTIME DESTINATION bin)
