set(PRJ fieldDataProvider)

set(SRC 
 fielddataproviders.cpp
 vectorspacebase.cpp
)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS 
)

set(INCLUDE_DIRS
  ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS
    uniof
)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
linkToolkit_OF( ${PRJ} )
