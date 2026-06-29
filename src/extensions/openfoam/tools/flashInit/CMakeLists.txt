
set(SRC flashInit.C)

set(OF_INCLUDE_DIRS
  sampling/lnInclude
  meshTools/lnInclude
)

set(OF_LIBS
  sampling
  meshTools
)

set(INCLUDE_DIRS
  ${insight_INCLUDE_DIR}
)

set(OF_VERSIONS OFesi2112 OFesi1806)
setup_exe_target_OF(flashInit "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")
