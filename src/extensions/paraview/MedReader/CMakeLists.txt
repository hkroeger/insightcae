# Copyright (C) 2010-2012  CEA/DEN, EDF R&D
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

project(MedReader)


#if(COMMAND cmake_policy)
#	 cmake_policy(SET CMP0003 NEW)
#endif(COMMAND cmake_policy)
		
SET(MED_READER_MAJOR_VERSION 3)
SET(MED_READER_MINOR_VERSION 1)
SET(MED_READER_BUILD_VERSION 0)
SET(MED_READER_VERSION
    "${MED_READER_MAJOR_VERSION}.${MED_READER_MINOR_VERSION}.${MED_READER_BUILD_VERSION}")

#
# Load the ParaView/VTK packages
#
set(MedReader_BUILD_ParaView_Plugin ON CACHE BOOL "Build the ParaView plugin")

if(MedReader_BUILD_ParaView_Plugin)
    FIND_PACKAGE(ParaView REQUIRED)
    if(NOT ParaView_FOUND)
        MESSAGE(FATAL_ERROR "Please locate ParaView." )
    ENDIF(NOT ParaView_FOUND)
endif(MedReader_BUILD_ParaView_Plugin)

FIND_PACKAGE(VTK REQUIRED)
if(NOT VTK_FOUND)
    MESSAGE(FATAL_ERROR "Please locate VTK." )
ENDIF(NOT VTK_FOUND)

	 
# Standard CMake option for building libraries shared or static by default.
OPTION(BUILD_SHARED_LIBS
       "Build with shared libraries."
       ${VTK_BUILD_SHARED_LIBS})
#
# Find dependant Library
#
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake ${CMAKE_MODULE_PATH} )
FIND_PACKAGE(MED REQUIRED)
include_directories(${MED_INCLUDE_DIRS})

#
# Build the vtkMedReader
#
#add_subdirectory(IO)

vtk_module_scan(
    MODULE_FILES      ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtk.module
    WANT_BY_DEFAULT   ON
    PROVIDES_MODULES  _mod
    )

set(CMAKE_INSTALL_LIBDIR "lib")
set(CMAKE_INSTALL_BINDIR "bin")
set(CMAKE_INSTALL_DATAROOTDIR "share")
set(CMAKE_INSTALL_INCLUDEDIR "include")
vtk_module_build(
    MODULES ${_mod}
    )
##
## Build the parallel vtkPMedReader
##
#set(MedReader_BUILD_PARALLEL ${VTK_USE_PARALLEL} CACHE BOOL "Build the Med Reader with parallel support.")

##
## Build the ParaView Plugin
##
if(MedReader_BUILD_ParaView_Plugin)

    paraview_plugin_scan(
      PLUGIN_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewPlugin/paraview.plugin"
      PROVIDES_PLUGINS  plugins
      REQUIRES_MODULES  required_modules)

    paraview_plugin_build(
      RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}"
      LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      LIBRARY_SUBDIRECTORY "${PARAVIEW_PLUGIN_SUBDIR}"
      PLUGINS ${plugins})

    #add_subdirectory(ParaViewPlugin)

endif(MedReader_BUILD_ParaView_Plugin)


	
