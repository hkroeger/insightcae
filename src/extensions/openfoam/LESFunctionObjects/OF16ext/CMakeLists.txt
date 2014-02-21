project(inflowGeneratorBC16ext)


set(inflowGeneratorBC_SOURCES 
 ../inflowGeneratorFvPatchVectorFieldFwd.C
)

include_directories(
  ${toolkit_SOURCE_DIR}
  ${OF16ext_LIBSRC_DIR}/OpenFOAM/lnInclude
  ${OF16ext_LIBSRC_DIR}/finiteVolume/lnInclude 
  ${OF16ext_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
  ${OF16ext_LIBSRC_DIR}/lduSolvers/lnInclude  
  ${OF16ext_LIBSRC_DIR}/turbulenceModels
  ${OF16ext_LIBSRC_DIR}/transportModels
  ${OF16ext_LIBSRC_DIR}/meshTools/lnInclude
  ${OF16ext_LIBSRC_DIR}/triSurface/lnInclude
  ${OF16ext_LIBSRC_DIR}/surfMesh/lnInclude
)

set(inflowGeneratorBC_SOURCE_DIR "${inflowGeneratorBC16ext_SOURCE_DIR}/..")
add_subdirectory(../turbulentStructures/turbulentStructure "${CMAKE_CURRENT_BINARY_DIR}/turbulentStructures/turbulentStructure")
add_subdirectory(../turbulentStructures/hatSpot "${CMAKE_CURRENT_BINARY_DIR}/turbulentStructures/hatSpot")
add_subdirectory(../turbulentStructures/homogeneousTurbulenceVorton "${CMAKE_CURRENT_BINARY_DIR}/turbulentStructures/homogeneousTurbulenceVorton")
include_directories(${inflowGeneratorBC_INC_DIRS})
message(STATUS "INC_DIRS=" ${inflowGeneratorBC_INC_DIRS})
set(inflowGeneratorBC16ext_SOURCES ${inflowGeneratorBC_SOURCES})

setup_lib_target_OF16ext(inflowGeneratorBC16ext "inflowGeneratorBC"
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
