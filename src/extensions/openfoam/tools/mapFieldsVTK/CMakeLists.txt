set(SRC mapfieldsvtk.cpp)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS

)

set(INCLUDE_DIRS
)

setup_exe_target_OF(mapFieldsVTK "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof;vtkHeaders" "fieldDataProvider")
linkToolkit_OF_Exe(mapFieldsVTK)
