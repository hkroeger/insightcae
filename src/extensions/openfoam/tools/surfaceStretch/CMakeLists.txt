
set(SRC surfaceStretch.C)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers
#  meshTools sampling surfMesh triSurface
)

set(OF_LIBS
# finiteVolume LESfilters sampling triSurface
)

set(INCLUDE_DIRS 
  ${ARMADILLO_INCLUDE_DIRS}
  ${toolkit_SOURCE_DIR}
  ${shipcommon_SOURCE_DIR}
)

set(LIBS
 shipcommon uniof
)

setup_exe_target_OF(surfaceStretch "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
