project(kOmegaSST216ext)

find_package(OpenFOAM-16ext REQUIRED)

set(kOmegaSST2_SOURCES 
 kOmegaSST2.C
)

include_directories(
  ${kOmegaSST2_SOURCE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF16ext_LIBSRC_DIR}/turbulenceModels
  ${OF16ext_LIBSRC_DIR}/turbulenceModels/incompressible/RAS/lnInclude
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

add_subdirectory(wallFunctions/hybridOmegaWallFunction2)
add_subdirectory(wallFunctions/nutHybridWallFunction2)

message(${kOmegaSST2_SOURCE_DIR})

set(kOmegaSST216ext_SOURCES ${kOmegaSST2_SOURCES})
setup_lib_target_OF16ext(kOmegaSST216ext "kOmegaSST2"
  OpenFOAM
  finiteVolume
  incompressibleRASModels 
  incompressibleTransportModels
  ) 

#install(TARGETS kOmegaSST2 LIBRARY DESTINATION lib)
