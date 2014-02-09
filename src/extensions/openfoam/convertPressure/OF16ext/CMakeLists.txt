 
project(convertPressure16ext)

set(convertPressure16ext_SOURCES ../convertPressure.C)

include_directories(
  ${insight_INCLUDE_DIR}
  ${journalbearingcommon16ext_SOURCE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  )

setup_exe_target_OF16ext(convertPressure16ext "convertPressure"
  OpenFOAM
  Pstream
  finiteVolume
  ) 

#install(TARGETS convertPressure RUNTIME DESTINATION bin)
