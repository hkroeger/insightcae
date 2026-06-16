
set(SRC stripFieldValues.C)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
)

set(OF_VERSIONS OFesi1806 OFesi2112)
setup_exe_target_OF(stripFieldValues "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")
