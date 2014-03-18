project(toolkit)
#find_package(Boost COMPONENTS filesystem regex date_time thread REQUIRED)

file(GLOB_RECURSE toolkit_SOURCES "*.cpp")
file(GLOB_RECURSE toolkit_HEADERS "*.h")
message(${toolkit_HEADERS})

#include_directories(${CMAKE_SOURCE_DIR})
#INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
include_directories(${toolkit_SOURCE_DIR})

link_directories(${Boost_LIBRARY_DIR})
add_library(toolkit SHARED ${toolkit_SOURCES})
target_link_libraries(toolkit ${Boost_LIBRARIES} ${ARMADILLO_LIBRARY} gsl dl)

install(TARGETS toolkit LIBRARY DESTINATION lib)
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
  )

message(STATUS ${toolkit_HEADERS})

FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})
FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

file(GLOB_RECURSE toolkit_INTERFACES "*.i")

SET(CMAKE_SWIG_FLAGS "")

SET_SOURCE_FILES_PROPERTIES(${toolkit_INTERFACES} PROPERTIES CPLUSPLUS ON)
SET_SOURCE_FILES_PROPERTIES(${toolkit_INTERFACES} PROPERTIES SWIG_FLAGS "-includeall")
SWIG_ADD_MODULE(toolkit python ${toolkit_INTERFACES} ${toolkit_SOURCES})
SWIG_LINK_LIBRARIES(toolkit ${PYTHON_LIBRARIES})

SET(SWIG_RUNTIME swigpyrun.h)
ADD_CUSTOM_COMMAND(
  OUTPUT ${SWIG_RUNTIME}
  COMMAND ${SWIG_EXECUTABLE} -python -external-runtime swigpyrun.h
  )
ADD_CUSTOM_TARGET(swig-runtime-h ALL DEPENDS ${SWIG_RUNTIME})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SWIG_RUNTIME} DESTINATION include/toolkit)
