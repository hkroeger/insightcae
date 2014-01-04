project(simpleDyMFoam)

find_package(OpenFOAM REQUIRED)

set(simpleDyMFoam_SOURCES simpleDyMFoam.C)

include_directories(
  ${insight_INCLUDE_DIR}
  ${journalbearingcommon_SOURCE_DIR}
  ${OF_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF_LIBSRC_DIR}/turbulenceModels
  ${OF_LIBSRC_DIR}/transportModels
  ${OF_LIBSRC_DIR}/turbulenceModels/incompressible/RAS/RASModel
  ${OF_LIBSRC_DIR}/transportModels/incompressible/singlePhaseTransportModel
  ${OF_LIBSRC_DIR}/dynamicMesh/dynamicFvMesh/lnInclude
  ${OF_LIBSRC_DIR}/dynamicMesh/dynamicMesh/lnInclude
  ${OF_LIBSRC_DIR}/meshTools/lnInclude
  )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OF_CXX_FLAGS}")

link_directories(${OF_LIB_DIR} ${OF_LIB_DIR}/openmpi-system)
set(LINK_FLAGS "${LINK_FLAGS} ${OF_LINK_FLAGS}")

add_executable(simpleDyMFoam ${simpleDyMFoam_SOURCES})
target_link_libraries(simpleDyMFoam 
  journalbearingcommon
  OpenFOAM
  Pstream
  finiteVolume
  incompressibleRASModels 
  incompressibleTransportModels
  dynamicFvMesh
  topoChangerFvMesh
  dynamicMesh
  meshTools
  ) 

install(TARGETS simpleDyMFoam RUNTIME DESTINATION bin)
