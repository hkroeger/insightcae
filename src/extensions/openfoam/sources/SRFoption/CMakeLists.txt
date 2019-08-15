set(SRC srfoption.cpp)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
  ${insight_INCLUDE_DIR}
)

set(OF_VERSIONS OF23x OFesi1806)
setup_lib_target_OF(SRFoption "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")

