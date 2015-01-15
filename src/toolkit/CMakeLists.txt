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

ADD_CUSTOM_COMMAND( OUTPUT TurbulentVelocityInletBC__Parameters.h
                    COMMAND ${CMAKE_SOURCE_DIR}/gen-sets.py ${CMAKE_CURRENT_SOURCE_DIR}/openfoam/boundaryconditioncaseelements.h
                    COMMAND ${CMAKE_BINARY_DIR}/bin/pdl TurbulentVelocityInletBC__Parameters.pdl
                    DEPENDS pdl ${CMAKE_SOURCE_DIR}/gen-sets.py ${CMAKE_CURRENT_SOURCE_DIR}/openfoam/boundaryconditioncaseelements.h
                    COMMENT "Generating source code from PDL in header" )
                    
ADD_CUSTOM_TARGET( RunGenerator DEPENDS TurbulentVelocityInletBC__Parameters.h 
                   COMMENT "Checking if re-generation is required" )
include_directories(${CMAKE_CURRENT_BINARY_DIR})
                    
#include_directories(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR} )
include_directories(${toolkit_SOURCE_DIR})

link_directories(${Boost_LIBRARY_DIR})
add_library(toolkit SHARED ${toolkit_SOURCES})
target_link_libraries(toolkit ${Boost_LIBRARIES} ${PYTHON_LIBRARY} ${ARMADILLO_LIBRARY} ${GSL_LIBRARIES} dl)
ADD_DEPENDENCIES( toolkit RunGenerator )

install(TARGETS toolkit LIBRARY DESTINATION lib)
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
  )

add_pybindings(toolkit "toolkit.i" toolkit)

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SWIG_RUNTIME} DESTINATION include/toolkit)
