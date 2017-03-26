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

set(toolkit_SOURCES

    base/analysis.cpp
    base/parameter.cpp
    base/exception.cpp
    base/tools.cpp
    base/latextools.cpp
    base/linearalgebra.cpp
    base/resultset.cpp
    base/global.cpp
    base/softwareenvironment.cpp
#     base/parameterstudy.cpp
    base/stltools.cpp
    base/parameterset.cpp
    base/analysisstepcontrol.cpp
    base/plottools.cpp
    base/pythoninterface.cpp
    base/vtktools.cpp
    base/caseelement.cpp
    base/case.cpp
    
    openfoam/blockmesh_templates.cpp
    openfoam/analysiscaseelements.cpp
    openfoam/openfoamanalysis.cpp
    openfoam/openfoamcase.cpp
    openfoam/snappyhexmesh.cpp
    openfoam/numericscaseelements.cpp
    openfoam/cfmesh.cpp
    openfoam/basiccaseelements.cpp
    openfoam/openfoamdict.cpp
    openfoam/openfoamtools.cpp
    openfoam/thermophysicalcaseelements.cpp
    openfoam/boundaryconditioncaseelements.cpp
    openfoam/blockmesh.cpp
    openfoam/turbulencemodelcaseelements.cpp
    openfoam/electromagneticscaseelements.cpp
    
    code_aster/caexportfile.cpp
    code_aster/codeasterrun.cpp
)

IF(INSIGHT_BUILD_PYTHONBINDINGS)
  LIST(APPEND toolkit_SOURCES
   base/pythonanalysis.cpp
  )
ENDIF()

file(GLOB_RECURSE toolkit_HEADERS "*.h")


link_directories(${Boost_LIBRARY_DIR})
add_library(toolkit SHARED ${toolkit_SOURCES})
target_link_libraries(toolkit ${PYTHON_LIBRARIES} ${Boost_LIBRARIES} ${ARMADILLO_LIBRARY} ${GSL_LIBRARIES} dl)

target_include_directories(toolkit 
  PUBLIC ${PYTHON_INCLUDE_DIR}/python2.7
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${ARMADILLO_INCLUDE_DIRS}
  PUBLIC ${GSL_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )

add_PDL(toolkit "${toolkit_HEADERS}")

install(TARGETS toolkit LIBRARY DESTINATION lib COMPONENT ${INSIGHT_INSTALL_COMPONENT})
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
  DESTINATION include
  COMPONENT ${INSIGHT_INSTALL_COMPONENT}
  FILES_MATCHING PATTERN "*.h"
  )

IF(INSIGHT_BUILD_PYTHONBINDINGS)
    ADD_CUSTOM_COMMAND(
        OUTPUT toolkit_swigpyrun.timestamp
        COMMAND "${SWIG_EXECUTABLE}" -python -external-runtime
        COMMAND touch toolkit_swigpyrun.timestamp
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    ADD_CUSTOM_TARGET( toolkit_swigpyrun DEPENDS toolkit_swigpyrun.timestamp
            COMMENT "Checking for swigpyrun.h generation" )
    ADD_DEPENDENCIES( toolkit toolkit_swigpyrun )
ENDIF()

add_pybindings(toolkit "toolkit.i" toolkit)

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SWIG_RUNTIME} DESTINATION include/toolkit)
