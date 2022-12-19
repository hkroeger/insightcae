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
cmake_minimum_required(VERSION 3.16)
project(MedReader)

#if(COMMAND cmake_policy)
#	 cmake_policy(SET CMP0003 NEW)
#endif(COMMAND cmake_policy)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(MedReader_BUILD_ParaView_Plugin "Build a plugin for paraview (additional to the VTK module)" ON)

SET(MED_READER_MAJOR_VERSION 3)
SET(MED_READER_MINOR_VERSION 1)
SET(MED_READER_BUILD_VERSION 0)
SET(MED_READER_VERSION
    "${MED_READER_MAJOR_VERSION}.${MED_READER_MINOR_VERSION}.${MED_READER_BUILD_VERSION}")

find_package(VTK
    NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
    REQUIRED
)

if (MedReader_BUILD_ParaView_Plugin)
    find_package(ParaView
        REQUIRED
    )
endif()

set(vtkMedReaderTargetName vtkMedReader)


include(GNUInstallDirs)
set(CMAKE_INSTALL_LIBDIR "lib") # we don't want lib64...
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

#
# Load the ParaView/VTK packages
#

#if(MedReader_BUILD_ParaView_Plugin)
#    FIND_PACKAGE(ParaView REQUIRED)
#    if(NOT ParaView_FOUND)
#        MESSAGE(FATAL_ERROR "Please locate ParaView." )
#    ENDIF(NOT ParaView_FOUND)
#endif(MedReader_BUILD_ParaView_Plugin)

#FIND_PACKAGE(VTK REQUIRED)
#if(NOT VTK_FOUND)
#    MESSAGE(FATAL_ERROR "Please locate VTK." )
#ENDIF(NOT VTK_FOUND)


# Standard CMake option for building libraries shared or static by default.


#
# Find dependant Library
#
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake ${CMAKE_MODULE_PATH} )
FIND_PACKAGE(MED REQUIRED)
FIND_PACKAGE(HDF5 REQUIRED)
include_directories(${MED_INCLUDE_DIRS})
include_directories(${HDF5_INCLUDE_DIRS})

##
## Build the parallel vtkPMedReader
##
#set(MedReader_BUILD_PARALLEL ${VTK_USE_PARALLEL} CACHE BOOL "Build the Med Reader with parallel support.")


#
# Build the vtkMedReader
#
vtk_module_scan(
    MODULE_FILES      ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtk.module
    WANT_BY_DEFAULT   ON
    PROVIDES_MODULES  _mod
    )
vtk_module_build(
    RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    #HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/medreader-${LIBRARY_NAME_SUFFIX}"
    #CMAKE_DESTINATION "medreader-${LIBRARY_NAME_SUFFIX}/cmake"
    #HIERARCHY_DESTINATION "medreader-${LIBRARY_NAME_SUFFIX}/vtk/hierarchy"

    LIBRARY_NAME_SUFFIX ${LIBRARY_NAME_SUFFIX}
    MODULES ${_mod}
    )
##
## Build the ParaView Plugin
##
if(MedReader_BUILD_ParaView_Plugin)

    paraview_plugin_scan(
      PLUGIN_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewPlugin/paraview.plugin"
      ENABLE_BY_DEFAULT ON
      PROVIDES_PLUGINS  plugins
      REQUIRES_MODULES  required_modules)

    paraview_plugin_build(
      RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}"
      LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      LIBRARY_SUBDIRECTORY "${PARAVIEW_PLUGIN_SUBDIR}"
      PLUGINS ${plugins}
    )

endif()



