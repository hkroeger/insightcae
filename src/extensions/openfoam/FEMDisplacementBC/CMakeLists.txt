project(FEMDisplacementBC16ext)


set(FEMDisplacementBC_SOURCES 
 fsitransform.cpp 
 FEMDisplacementPointPatchVectorField.C
 FEMDisplacementTetPolyPatchVectorFieldCellDecomp.C
 FEMDisplacementTetPolyPatchVectorFieldFaceDecomp.C 
)

set(include_directories
  ${toolkit_SOURCE_DIR}
  ${ARMADILLO_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR}
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
  
setup_lib_target_OF16ext(FEMDisplacementBC16ext "${FEMDisplacementBC_SOURCES}" "FEMDisplacementBC" "${include_directories}"
  ${OF16ext_LIB_DIR}/libfiniteVolume.so
  ${OF16ext_LIB_DIR}/libincompressibleRASModels.so 
  ${OF16ext_LIB_DIR}/libincompressibleTransportModels.so
  ${OF16ext_LIB_DIR}/libtriSurface.so
  ${OF16ext_LIB_DIR}/libfiniteArea.so
  ${OF16ext_LIB_DIR}/libdynamicFvMesh.so
  ${OF16ext_LIB_DIR}/libfaceDecompositionMotionSolver.so
  ${OF16ext_LIB_DIR}/libfvMotionSolver.so
  ${OF16ext_LIB_DIR}/libsurfMesh.so
  ${OF16ext_LIB_DIR}/libsampling.so
)

#install(TARGETS FEMDisplacementBC LIBRARY DESTINATION lib)
