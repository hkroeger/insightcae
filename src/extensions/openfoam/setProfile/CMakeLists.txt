set(SRC setProfile.C)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
#  meshTools
)

set(OF_LIBS
# finiteVolume
)

set(INCLUDE_DIRS 
)

set(LIBS
    toolkit
    uniof
)

setup_exe_target_OF(setProfile "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
