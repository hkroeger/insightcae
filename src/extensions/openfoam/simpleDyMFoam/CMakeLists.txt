if (OF16ext_FOUND)

set(simpleDyMFoam_SOURCES simpleDyMFoam.C)

project(simpleDyMFoam16ext)
set(include_directories
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
setup_exe_target_OF16ext(simpleDyMFoam16ext "${simpleDyMFoam_SOURCES}" "simpleDyMFoam" "${include_directories}"
  ${OF16ext_LIB_DIR}/libfiniteVolume.so
  ${OF16ext_LIB_DIR}/libincompressibleRASModels.so
  ${OF16ext_LIB_DIR}/libincompressibleTransportModels.so
  ${OF16ext_LIB_DIR}/libdynamicFvMesh.so
  ${OF16ext_LIB_DIR}/libtopoChangerFvMesh.so
  ${OF16ext_LIB_DIR}/libdynamicMesh.so
  ${OF16ext_LIB_DIR}/libmeshTools.so
) 

endif()