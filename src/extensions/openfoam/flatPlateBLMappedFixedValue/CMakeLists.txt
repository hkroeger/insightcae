set(PRJ flatPlateBLMappedFixedValue)

set(SRC 
 flatPlateBLMappedFixedValueFvPatchField.C 
 flatPlateBLMappedFixedValueFvPatchFields.C 
)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS 
)

set(INCLUDE_DIRS 
)

set(LIBS 
)


set(OF_VERSIONS OF23x)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
