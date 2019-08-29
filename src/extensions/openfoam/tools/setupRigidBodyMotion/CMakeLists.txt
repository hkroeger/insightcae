
set(SRC setupRigidBodyMotion.C)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers
#  meshTools sampling surfMesh triSurface
)

set(OF_LIBS
# finiteVolume LESfilters sampling
)

# set(INCLUDE_DIRS 
#   ${ARMADILLO_INCLUDE_DIRS}
#   ${toolkit_SOURCE_DIR}
#   ${shipcommon_SOURCE_DIR}
# )
# 
set(IS_OF_TARGS 
 extendedRigidBodyMotion
)

set (OF_VERSIONS OFplus OFesi1806)
setup_exe_target_OF(setupRigidBodyMotion "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "${IS_OF_TARGS}")
