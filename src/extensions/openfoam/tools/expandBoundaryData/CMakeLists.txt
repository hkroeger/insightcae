set(SRC expandBoundaryData.C)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
)

set(OF_VERSIONS OFesi1806)
setup_exe_target_OF(expandBoundaryData "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")
