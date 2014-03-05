project(inflowGeneratorBC22x)


set(inflowGeneratorBC_SOURCES 
 ../inflowGeneratorBaseFvPatchVectorField.C
 ../inflowGeneratorFvPatchVectorFieldFwd.C
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
  ${OF22x_LIBSRC_DIR}/surfMesh/lnInclude
)

set(inflowGeneratorBC_SOURCE_DIR "${inflowGeneratorBC22x_SOURCE_DIR}/..")
add_subdirectory(../turbulentStructures/turbulentStructure "${CMAKE_CURRENT_BINARY_DIR}/turbulentStructures/turbulentStructure")
add_subdirectory(../turbulentStructures/hatSpot "${CMAKE_CURRENT_BINARY_DIR}/turbulentStructures/hatSpot")
add_subdirectory(../turbulentStructures/homogeneousTurbulenceVorton "${CMAKE_CURRENT_BINARY_DIR}/turbulentStructures/homogeneousTurbulenceVorton")
include_directories(${inflowGeneratorBC_INC_DIRS})
message(STATUS "INC_DIRS=" ${inflowGeneratorBC_INC_DIRS})
set(inflowGeneratorBC22x_SOURCES ${inflowGeneratorBC_SOURCES})

setup_lib_target_OF22x(inflowGeneratorBC22x "inflowGeneratorBC"
  toolkit
  OpenFOAM
  finiteVolume
  incompressibleRASModels 
  incompressibleTransportModels
  triSurface
  surfMesh
  sampling
)

#install(TARGETS FEMDisplacementBC LIBRARY DESTINATION lib)
