
set(SRC setupRigidBodyMotion.C)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers
#  meshTools sampling surfMesh triSurface
)

set(OF_LIBS
# finiteVolume LESfilters sampling
)

set(LIBS
    uniof
)

set(IS_OF_TARGS 
 extendedRigidBodyMotion
)

set (OF_VERSIONS OFplus OFesi1806 OFesi1906)
setup_exe_target_OF(setupRigidBodyMotion "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "${IS_OF_TARGS}")
linkToolkit_OF_Exe(setupRigidBodyMotion)
