
set(SRC unstretchMesh.C)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers
#  meshTools sampling surfMesh triSurface
)

set(OF_LIBS
# finiteVolume LESfilters sampling
)

set(INCLUDE_DIRS
)

set(LIBS
 uniof
)

setup_exe_target_OF(unstretchMesh "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
linkToolkit_OF_Exe( unstretchMesh )
