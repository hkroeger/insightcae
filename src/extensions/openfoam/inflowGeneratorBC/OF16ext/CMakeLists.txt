project(inflowGeneratorBC16ext)


set(inflowGeneratorBC16ext_SOURCES 
 genericInflowGeneratorFvPatchVectorFieldFwd.C
)

include_directories(
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF16ext_LIBSRC_DIR}/turbulenceModels
  ${OF16ext_LIBSRC_DIR}/transportModels
  ${OF16ext_LIBSRC_DIR}/tetDecompositionFiniteElement/lnInclude
  ${OF16ext_LIBSRC_DIR}/dynamicMesh/dynamicMesh/lnInclude
  ${OF16ext_LIBSRC_DIR}/dynamicMesh/dynamicFvMesh/lnInclude
  ${OF16ext_LIBSRC_DIR}/dynamicMesh/meshMotion/tetDecompositionMotionSolver/lnInclude
  ${OF16ext_LIBSRC_DIR}/dynamicMesh/meshMotion/fvMotionSolver/lnInclude
  ${OF16ext_LIBSRC_DIR}/meshTools/lnInclude
  ${OF16ext_LIBSRC_DIR}/sampling/lnInclude
  ${OF16ext_LIBSRC_DIR}/triSurface/lnInclude
  ${OF16ext_LIBSRC_DIR}/surfMesh/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteArea/lnInclude
)
  
setup_lib_target_OF16ext(inflowGeneratorBC16ext "inflowGeneratorBC"
  OpenFOAM
  finiteVolume
  incompressibleRASModels 
  incompressibleTransportModels
  triSurface
  finiteArea
  surfMesh
  sampling
)

#install(TARGETS FEMDisplacementBC LIBRARY DESTINATION lib)
