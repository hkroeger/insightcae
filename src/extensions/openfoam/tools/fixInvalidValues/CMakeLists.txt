
set(SRC fixInvalidValues.C)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
)

set(LIBS
 uniof
)

setup_exe_target_OF(fixInvalidValues "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
linkToolkit_OF_Exe( fixInvalidValues )
