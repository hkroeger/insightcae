
set(PRJ limitEnergyField)

set(SRC
    limitEnergyField.C
)

set(OF_INCLUDE_DIRS
    thermophysicalModels/basic
)

set(OF_LIBS
    fluidThermophysicalModels
)

set(INCLUDE_DIRS
)

set(LIBS
    uniof
)

set(OF_VERSIONS OFesi2112)
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
