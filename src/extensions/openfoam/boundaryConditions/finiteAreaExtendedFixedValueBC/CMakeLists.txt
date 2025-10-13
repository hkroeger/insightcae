set(PRJ finiteAreaExtendedFixedValueBC)

set(SRC 
 extendedFixedValueFaPatchFields.C
)

set(OF_INCLUDE_DIRS
  finiteArea/lnInclude
)

set(OF_LIBS 
)

set(INCLUDE_DIRS 
  ${ARMADILLO_INCLUDE_DIRS}
  ${GSL_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS
 # ${Boost_LIBRARIES}
)

set(IS_OF_LIBS 
 fieldDataProvider
)
set(OF_VERSIONS OFesi2112)
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "${IS_OF_LIBS}")
