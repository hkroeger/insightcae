project(FEMDisplacementBC16ext)


set(FEMDisplacementBC16ext_SOURCES 
 fsitransform.cpp 
 FEMDisplacementPointPatchVectorField.C
 FEMDisplacementTetPolyPatchVectorFieldCellDecomp.C
 FEMDisplacementTetPolyPatchVectorFieldFaceDecomp.C 
)

include_directories(
  ${toolkit_SOURCE_DIR}
  ${journalbearingcommon_SOURCE_DIR}
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
  
setup_lib_target_OF16ext(FEMDisplacementBC16ext "FEMDisplacementBC"
  OpenFOAM
  finiteVolume
  incompressibleRASModels 
  incompressibleTransportModels
  triSurface
  finiteArea
  dynamicFvMesh
  faceDecompositionMotionSolver
  fvMotionSolver
  surfMesh
  sampling
)

#install(TARGETS FEMDisplacementBC LIBRARY DESTINATION lib)
