set(SRC movingprobes.cpp)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(LIBS
 uniof
)

set(INCLUDE_DIRS
  ${insight_INCLUDE_DIR}
)

setup_exe_target_OF(movingProbes "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
linkToolkit_OF_Exe( movingProbes )
