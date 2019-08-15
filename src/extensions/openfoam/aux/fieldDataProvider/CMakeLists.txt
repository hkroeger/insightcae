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
  ${toolkit_SOURCE_DIR}
  ${insight_INCLUDE_DIR}
  ${ARMADILLO_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${GSL_INCLUDE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS 
    toolkit
    uniof
)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
