set(SRC eMeshTransformPoints.C)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
#  meshTools
)

set(OF_LIBS
# finiteVolume
)

set(INCLUDE_DIRS 
)

setup_exe_target_OF(eMeshTransformPoints "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")
