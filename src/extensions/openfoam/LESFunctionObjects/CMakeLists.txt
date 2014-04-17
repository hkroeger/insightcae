

set(PRJ LESFunctionObjects)

set(SRC 
)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  turbulenceModels/incompressible/LES turbulenceModels/LES/LESfilters transportModels
  meshTools triSurface sampling surfMesh lagrangian lagrangian/basic fileFormats
)

set(OF_LIBS 
finiteVolume incompressibleRASModels 
incompressibleTransportModels meshTools triSurface LESfilters 
surfMesh sampling)

set(INCLUDE_DIRS 
#   ${insight_INCLUDE_DIR}
#   ${ARMADILLO_INCLUDE_DIRS}
#   ${Boost_INCLUDE_DIRS}
#   ${toolkit_SOURCE_DIR}
#   ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS 
)

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
add_subdirectory(twoPointCorrelation)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}")

# 
# set(LESFunctionObjects_SOURCES  
#  #inflowGeneratorFvPatchVectorFieldFwd.C
# )
# 
# set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
# add_subdirectory(twoPointCorrelation)
# 
# 
# project(LESFunctionObjects22x)
# set(include_directories
#   ${toolkit_SOURCE_DIR}
#   ${OF22x_LIBSRC_DIR}/OpenFOAM/lnInclude
#   ${OF22x_LIBSRC_DIR}/finiteVolume/lnInclude 
#   ${OF22x_LIBSRC_DIR}/OSspecific/POSIX/lnInclude 
#   ${OF22x_LIBSRC_DIR}/lduSolvers/lnInclude  
#   ${OF22x_LIBSRC_DIR}/turbulenceModels
#   ${OF22x_LIBSRC_DIR}/transportModels
#   ${OF22x_LIBSRC_DIR}/meshTools/lnInclude
#   ${OF22x_LIBSRC_DIR}/triSurface/lnInclude
#   ${OF22x_LIBSRC_DIR}/sampling/lnInclude
#   ${OF22x_LIBSRC_DIR}/fileFormats/lnInclude
#   ${OF22x_LIBSRC_DIR}/lagrangian/basic/lnInclude
#   ${OF22x_LIBSRC_DIR}/surfMesh/lnInclude
# )
# setup_lib_target_OF22x(LESFunctionObjects22x "${LESFunctionObjects_SOURCES}" "LESFunctionObjects" "${include_directories}"
#   toolkit
#   ${OF22x_LIB_DIR}/libfiniteVolume.so
#   ${OF22x_LIB_DIR}/libincompressibleRASModels.so
#   ${OF22x_LIB_DIR}/libincompressibleTransportModels.so
#   ${OF22x_LIB_DIR}/libtriSurface.so
#   ${OF22x_LIB_DIR}/libsurfMesh.so
#   ${OF22x_LIB_DIR}/libsampling.so
# )
# 
# #install(TARGETS FEMDisplacementBC LIBRARY DESTINATION lib)

