set(SRC rangeweightfield.cpp)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
  ${insight_INCLUDE_DIR}
)

set(OF_VERSIONS OFesi1806 OFesi1906 OFesi2112)
setup_lib_target_OF(rangeWeightField "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")


