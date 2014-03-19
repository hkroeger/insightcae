


set(inflowGeneratorBC_SOURCES 
 boostRandomGen.C 
 inflowGeneratorBaseFvPatchVectorField.C
 inflowGeneratorFvPatchVectorFieldFwd.C
)

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
add_subdirectory(turbulentStructures/turbulentStructure)
add_subdirectory(turbulentStructures/hatSpot)
add_subdirectory(turbulentStructures/homogeneousTurbulenceVorton)

project(inflowGeneratorBC16ext)
set(include_directories
  ${PROJECT_SOURCE_DIR}
  ${inflowGeneratorBC_INC_DIRS}
  ${ARMADILLO_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
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
setup_lib_target_OF16ext(inflowGeneratorBC16ext "${inflowGeneratorBC_SOURCES}" "inflowGeneratorBC" "${include_directories}"
  toolkit
  ${OF16ext_LIB_DIR}/libfiniteVolume.so
  ${OF16ext_LIB_DIR}/libincompressibleRASModels.so
  ${OF16ext_LIB_DIR}/libincompressibleTransportModels.so
  ${OF16ext_LIB_DIR}/libtriSurface.so
  ${OF16ext_LIB_DIR}/libsurfMesh.so
  ${OF16ext_LIB_DIR}/libsampling.so
)

project(inflowGeneratorBC22x)
set(include_directories
  ${PROJECT_SOURCE_DIR}
  ${inflowGeneratorBC_INC_DIRS}
  ${ARMADILLO_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
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
setup_lib_target_OF22x(inflowGeneratorBC22x "${inflowGeneratorBC_SOURCES}" "inflowGeneratorBC" "${include_directories}"
  toolkit
  ${OF22x_LIB_DIR}/libfiniteVolume.so
  ${OF22x_LIB_DIR}/libincompressibleRASModels.so
  ${OF22x_LIB_DIR}/libincompressibleTransportModels.so
  ${OF22x_LIB_DIR}/libtriSurface.so
  ${OF22x_LIB_DIR}/libsurfMesh.so
  ${OF22x_LIB_DIR}/libsampling.so
)
