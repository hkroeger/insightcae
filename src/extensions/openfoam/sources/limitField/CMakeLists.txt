

set(PRJ limitField)

set(SRC
    limitfields.cpp
)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
)

set(LIBS
     uniof
)

set(OF_VERSIONS OFplus OFesi1806)
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
