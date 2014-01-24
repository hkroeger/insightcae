project(kOmegaSST2)

find_package(OpenFOAM REQUIRED)

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g")

set(kOmegaSST2_SOURCES 
 kOmegaSST2.C
)

include_directories(
  ${kOmegaSST2_SOURCE_DIR}
  ${OF_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF_LIBSRC_DIR}/turbulenceModels
  ${OF_LIBSRC_DIR}/turbulenceModels/incompressible/RAS/lnInclude
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

add_subdirectory(wallFunctions/hybridOmegaWallFunction2)
add_subdirectory(wallFunctions/nutHybridWallFunction2)

message(${kOmegaSST2_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OF_CXX_FLAGS}")

link_directories(${OF_LIB_DIR} ${OF_LIB_DIR}/openmpi-system)
set(LINK_FLAGS "${LINK_FLAGS} ${OF_LINK_FLAGS}")

add_library(kOmegaSST2 SHARED ${kOmegaSST2_SOURCES})
target_link_libraries(kOmegaSST2 
  OpenFOAM
  finiteVolume
  incompressibleRASModels 
  incompressibleTransportModels
  ) 

install(TARGETS kOmegaSST2 LIBRARY DESTINATION lib)
