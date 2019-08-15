set(SRC srfinit.cpp)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
  ${insight_INCLUDE_DIR}
)

set(OF_VERSIONS OF23x OFplus OFesi1806)
setup_exe_target_OF(SRFinit "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")

