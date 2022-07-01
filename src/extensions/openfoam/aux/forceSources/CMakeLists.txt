set(PRJ forceSources)

set(SRC
 forcesources.cpp
 constantforcesource.cpp
 subtractedforcesource.cpp
 multipliedforcesource.cpp
 relaxedforcesource.cpp
 forcesourcecombination.cpp
)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
  ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS
    uniof globalRegistry
)

setup_lib_target_OF(
    ${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}"
    "${INCLUDE_DIRS}" "${LIBS}" "")
#linkToolkit_OF_ShLib( ${PRJ} )
