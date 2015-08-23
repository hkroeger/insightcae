set(PRJ kOmegaSST2)

set(SRC 
 kOmegaSST2.C
)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  turbulenceModels
  turbulenceModels/incompressible/RAS
  transportModels
  dynamicMesh/dynamicMesh
  dynamicMesh/dynamicFvMesh
  dynamicMesh/meshMotion/tetDecompositionMotionSolver
  dynamicMesh/meshMotion/fvMotionSolver
  meshTools
  sampling
  triSurface
  surfMesh
)

set(OF_LIBS 
finiteVolume incompressibleRASModels incompressibleTransportModels
)

set(INCLUDE_DIRS 
  ${insight_INCLUDE_DIR}
  ${ARMADILLO_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${toolkit_SOURCE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS 
#toolkit
)

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
add_subdirectory(wallFunctions/hybridOmegaWallFunction2)
add_subdirectory(wallFunctions/nutHybridWallFunction2)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
# 
# set(kOmegaSST2_SOURCES 
#  kOmegaSST2.C
# )
# 
# set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
# add_subdirectory(wallFunctions/hybridOmegaWallFunction2)
# add_subdirectory(wallFunctions/nutHybridWallFunction2)
# 
# if (OF16ext_FOUND)
# 
# project(kOmegaSST216ext)
# set(include_directories
#   ${kOmegaSST216ext_SOURCE_DIR}
#   ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
#   ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
#   ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
#   ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
#   ${OF16ext_LIBSRC_DIR}/turbulenceModels
#   ${OF16ext_LIBSRC_DIR}/turbulenceModels/incompressible/RAS/lnInclude
#   ${OF16ext_LIBSRC_DIR}/transportModels
#   ${OF16ext_LIBSRC_DIR}/tetDecompositionFiniteElement/lnInclude
#   ${OF16ext_LIBSRC_DIR}/dynamicMesh/dynamicMesh/lnInclude
#   ${OF16ext_LIBSRC_DIR}/dynamicMesh/dynamicFvMesh/lnInclude
#   ${OF16ext_LIBSRC_DIR}/dynamicMesh/meshMotion/tetDecompositionMotionSolver/lnInclude
#   ${OF16ext_LIBSRC_DIR}/dynamicMesh/meshMotion/fvMotionSolver/lnInclude
#   ${OF16ext_LIBSRC_DIR}/meshTools/lnInclude
#   ${OF16ext_LIBSRC_DIR}/sampling/lnInclude
#   ${OF16ext_LIBSRC_DIR}/triSurface/lnInclude
#   ${OF16ext_LIBSRC_DIR}/surfMesh/lnInclude
#   ${OF16ext_LIBSRC_DIR}/finiteArea/lnInclude
#   )
# setup_lib_target_OF16ext(kOmegaSST216ext "${kOmegaSST2_SOURCES}" "kOmegaSST2" "${include_directories}"
#   ${OF16ext_LIB_DIR}/libfiniteVolume.so
#   ${OF16ext_LIB_DIR}/libincompressibleRASModels.so
#   ${OF16ext_LIB_DIR}/libincompressibleTransportModels.so
#   ) 
# 
# #install(TARGETS kOmegaSST2 LIBRARY DESTINATION lib)
# endif()