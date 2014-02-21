project(LESFunctionObjects22x)


set(LESFunctionObjects_SOURCES 
#  ../inflowGeneratorFvPatchVectorFieldFwd.C
)

include_directories(
  ${toolkit_SOURCE_DIR}
  ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF22x_LIBSRC_DIR}/turbulenceModels
  ${OF22x_LIBSRC_DIR}/transportModels
  ${OF22x_LIBSRC_DIR}/meshTools/lnInclude
  ${OF22x_LIBSRC_DIR}/triSurface/lnInclude
  ${OF22x_LIBSRC_DIR}/sampling/lnInclude
  ${OF22x_LIBSRC_DIR}/fileFormats/lnInclude
  ${OF22x_LIBSRC_DIR}/lagrangian/basic/lnInclude
  ${OF22x_LIBSRC_DIR}/surfMesh/lnInclude
)

set(LESFunctionObjects_SOURCE_DIR "${LESFunctionObjects22x_SOURCE_DIR}/..")
add_subdirectory(../twoPointCorrelation "${CMAKE_CURRENT_BINARY_DIR}/twoPointCorrelation")
include_directories(${LESFunctionObjects_INC_DIRS})
message(STATUS "INC_DIRS=" ${LESFunctionObjects_INC_DIRS})
set(LESFunctionObjects22x_SOURCES ${LESFunctionObjects_SOURCES})

setup_lib_target_OF22x(LESFunctionObjects22x "LESFunctionObjects"
  toolkit
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
