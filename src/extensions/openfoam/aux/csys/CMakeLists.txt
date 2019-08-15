set(PRJ csys)

set(SRC 
 csys.cpp
)

set(OF_INCLUDE_DIRS
#  OpenFOAM OSspecific/POSIX
)

set(OF_LIBS 
)

set(INCLUDE_DIRS 
 ${Boost_INCLUDE_DIR}
)

set(LIBS 
 toolkit
)

# set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
# add_subdirectory(twoPointCorrelation)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")

