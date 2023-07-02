set(PRJ fieldDataProvider)

set(SRC 
 fielddataproviders.cpp
 vectorspacebase.cpp
 vtkconversion.cpp
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
    toolkitHeaders vtkHeaders vtkOffscreen # link directly because this lib uses VTK
    toolkitVtkOffscreenLoader # load toolkit through loader lib to place VTK also in global lookup scope
)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
