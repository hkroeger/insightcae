project(simpleDyMFoam16ext)

set(simpleDyMFoam16ext_SOURCES ../simpleDyMFoam.C)

include_directories(
  ${insight_INCLUDE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF16ext_LIBSRC_DIR}/turbulenceModels
  ${OF16ext_LIBSRC_DIR}/transportModels
  ${OF16ext_LIBSRC_DIR}/turbulenceModels/incompressible/RAS/RASModel
  ${OF16ext_LIBSRC_DIR}/transportModels/incompressible/singlePhaseTransportModel
  ${OF16ext_LIBSRC_DIR}/dynamicMesh/dynamicFvMesh/lnInclude
  ${OF16ext_LIBSRC_DIR}/dynamicMesh/dynamicMesh/lnInclude
  ${OF16ext_LIBSRC_DIR}/meshTools/lnInclude
  )

setup_exe_target_OF16ext(simpleDyMFoam16ext "simpleDyMFoam"
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

#install(TARGETS simpleDyMFoam RUNTIME DESTINATION bin)
