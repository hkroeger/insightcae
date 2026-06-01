
set(SRC flashInit.C)

set(OF_INCLUDE_DIRS
  # finiteVolume and OpenFOAM core are pulled in by fvCFD.H
)

set(OF_LIBS
  # finiteVolume is linked by default via setup_exe_target_OF
)

set(INCLUDE_DIRS
  ${insight_INCLUDE_DIR}
)

setup_exe_target_OF(flashInit "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")
