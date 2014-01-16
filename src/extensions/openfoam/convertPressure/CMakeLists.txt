project(convertPressure)

find_package(OpenFOAM REQUIRED)

set(convertPressure_SOURCES convertPressure.C)

include_directories(
  ${insight_INCLUDE_DIR}
  ${journalbearingcommon_SOURCE_DIR}
  ${OF_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF_LIBSRC_DIR}/lduSolvers/lnInclude  
  )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OF_CXX_FLAGS}")

link_directories(${OF_LIB_DIR} ${OF_LIB_DIR}/openmpi-system)
set(LINK_FLAGS "${LINK_FLAGS} ${OF_LINK_FLAGS}")

add_executable(convertPressure ${convertPressure_SOURCES})
target_link_libraries(convertPressure 
  OpenFOAM
  Pstream
  finiteVolume
  ) 

install(TARGETS convertPressure RUNTIME DESTINATION bin)
