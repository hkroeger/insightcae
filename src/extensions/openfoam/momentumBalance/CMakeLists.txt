set(SRC momentumBalance.C)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(LIBS
 uniof
 toolkit
)

set(INCLUDE_DIRS
  ${insight_INCLUDE_DIR}
)

setup_exe_target_OF(momentumBalance "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
