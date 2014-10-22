# 
# This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
# Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#

project(toolkit)
#find_package(Boost COMPONENTS filesystem regex date_time thread REQUIRED)

file(GLOB_RECURSE toolkit_SOURCES "*.cpp")
file(GLOB_RECURSE toolkit_HEADERS "*.h")
message(${toolkit_HEADERS})

#include_directories(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
include_directories(${toolkit_SOURCE_DIR})

link_directories(${Boost_LIBRARY_DIR})
add_library(toolkit SHARED ${toolkit_SOURCES})
target_link_libraries(toolkit ${Boost_LIBRARIES} ${PYTHON_LIBRARY} ${ARMADILLO_LIBRARY} gsl dl)

install(TARGETS toolkit LIBRARY DESTINATION lib)
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
  )

message(STATUS ${toolkit_HEADERS})

IF(INSIGHT_BUILD_PYTHONBINDINGS)
IF(SWIG_FOUND)
INCLUDE(${SWIG_USE_FILE})
FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
set(CMAKE_SWIG_OUTDIR ${CMAKE_BINARY_DIR}/share/insight/python/Insight)
SET(toolkit_INTERFACES "toolkit.i")
SET(CMAKE_SWIG_FLAGS "")
SET_SOURCE_FILES_PROPERTIES(${toolkit_INTERFACES} PROPERTIES CPLUSPLUS ON)
#SET_SOURCE_FILES_PROPERTIES(${toolkit_INTERFACES} PROPERTIES SWIG_FLAGS "-builtin")
SWIG_ADD_MODULE(toolkit python ${toolkit_INTERFACES} ${toolkit_SOURCES})
SWIG_LINK_LIBRARIES(toolkit ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${ARMADILLO_LIBRARY} gsl dl)
set_target_properties(${SWIG_MODULE_toolkit_REAL_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/share/insight/python/Insight)
ENDIF()

SET(SWIG_RUNTIME swigpyrun.h)
ADD_CUSTOM_COMMAND(
 OUTPUT ${SWIG_RUNTIME}
 COMMAND ${SWIG_EXECUTABLE} -python -external-runtime swigpyrun.h
 )
ADD_CUSTOM_TARGET(swig-runtime-h ALL DEPENDS ${SWIG_RUNTIME})
ENDIF()

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SWIG_RUNTIME} DESTINATION include/toolkit)
