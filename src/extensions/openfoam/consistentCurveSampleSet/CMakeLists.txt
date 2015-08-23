set(PRJ consistentCurveSampleSet)

set(SRC 
 ${PRJ}.C
)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  sampling fileFormats lagrangian/basic
  meshTools triSurface sampling surfMesh
)

set(OF_LIBS 
finiteVolume 
triSurface lagrangian 
meshTools forces surfMesh sampling
)

set(INCLUDE_DIRS 
#   ${insight_INCLUDE_DIR}
#   ${ARMADILLO_INCLUDE_DIRS}
#   ${Boost_INCLUDE_DIRS}
#   ${toolkit_SOURCE_DIR}
#   ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS 
# toolkit
)

# set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
# add_subdirectory(faceQualityMarker)
#set(OF_VERSIONS OF22x OF23x)
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
 
