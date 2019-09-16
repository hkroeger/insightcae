set(SRC sonicMRFFoam.C)

set(OF_INCLUDE_DIRS
    transportModels transportModels/compressible
    turbulenceModels turbulenceModels/compressible/RAS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
)


set(OF_VERSIONS Fx32 Fx41)
setup_exe_target_OF(sonicMRFFoam "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")
