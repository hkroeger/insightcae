project(FEMDisplacementBC)

find_package(OpenFOAM REQUIRED)

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g")

set(FEMDisplacementBC_SOURCES 
 fsitransform.cpp 
 FEMDisplacementPointPatchVectorField.C
 FEMDisplacementTetPolyPatchVectorFieldCellDecomp.C
 FEMDisplacementTetPolyPatchVectorFieldFaceDecomp.C 
)

include_directories(
  ${toolkit_SOURCE_DIR}
  ${journalbearingcommon_SOURCE_DIR}
  ${OF_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF_LIBSRC_DIR}/turbulenceModels
  ${OF_LIBSRC_DIR}/transportModels
  ${OF_LIBSRC_DIR}/tetDecompositionFiniteElement/lnInclude
  ${OF_LIBSRC_DIR}/dynamicMesh/dynamicMesh/lnInclude
  ${OF_LIBSRC_DIR}/dynamicMesh/dynamicFvMesh/lnInclude
  ${OF_LIBSRC_DIR}/dynamicMesh/meshMotion/tetDecompositionMotionSolver/lnInclude
  ${OF_LIBSRC_DIR}/dynamicMesh/meshMotion/fvMotionSolver/lnInclude
  ${OF_LIBSRC_DIR}/meshTools/lnInclude
  ${OF_LIBSRC_DIR}/sampling/lnInclude
  ${OF_LIBSRC_DIR}/triSurface/lnInclude
  ${OF_LIBSRC_DIR}/surfMesh/lnInclude
  ${OF_LIBSRC_DIR}/finiteArea/lnInclude
  )
  
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OF_CXX_FLAGS}")

link_directories(${OF_LIB_DIR} ${OF_LIB_DIR}/openmpi-system)
set(LINK_FLAGS "${LINK_FLAGS} ${OF_LINK_FLAGS}")

add_library(FEMDisplacementBC SHARED ${FEMDisplacementBC_SOURCES})
target_link_libraries(FEMDisplacementBC 
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

install(TARGETS FEMDisplacementBC LIBRARY DESTINATION lib)
