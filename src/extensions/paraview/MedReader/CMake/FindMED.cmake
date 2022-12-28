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

# - Find MED library
# Find the MED includes and library
# This module defines
#  MED3HOME, a directory where MED was installed. This directory is used to help find trhe other values.
#  MED_INCLUDE_DIR, where to find med.h
#  MED_INCLUDE_DIRS, where to find med.h file, concatenated with other include dirs from HDF5 and MPI (if parallel)
#  MED_LIBRARIES, libraries to link against to use MED. (including HDF5 and MPI if parallel)
#  MED_FOUND, If false, do not try to use MED.
# also defined, but not for general use are
#  MED_LIBRARY, the med library.
#  MEDC_LIBRARY, the medC library

SET(MED3HOME $ENV{MED3HOME} CACHE PATH "Path to the med install dir")

IF(NOT MED3HOME)
  FIND_PROGRAM(MDUMP mdump)
  IF(MDUMP)
    SET(MED3HOME ${MDUMP})
    GET_FILENAME_COMPONENT(MED3HOME ${MED3HOME} PATH)
    GET_FILENAME_COMPONENT(MED3HOME ${MED3HOME} PATH)
  ENDIF(MDUMP)
ENDIF(NOT MED3HOME)

FIND_PATH(MED_INCLUDE_DIR med.h
  HINTS
  ${MED3HOME}/include
  PATHS
  /usr/local/include
  /usr/include
)

FIND_LIBRARY(MED_LIBRARY med
  HINTS
  ${MED_INCLUDE_DIR}/../lib
  ${MED3HOME}/lib
  PATHS
  /usr/local/lib
  /usr/lib
)

FIND_LIBRARY(MEDC_LIBRARY medC
  HINTS
  ${MED_LIBRARY_DIR}
  ${MED3HOME}/lib
  PATHS
  /usr/local/lib
  /usr/lib
)

get_filename_component(MED_LIBRARY_DIR ${MEDC_LIBRARY} PATH)

IF(MED_INCLUDE_DIR)
  IF(MEDC_LIBRARY)
    SET(MED_LIBRARIES ${MEDC_LIBRARY} )
    IF(MED_LIBRARY)
      SET(MED_LIBRARIES ${MED_LIBRARIES} ${MED_LIBRARY} )
    ENDIF(MED_LIBRARY)
    SET( MED_FOUND "YES" )
  ENDIF(MEDC_LIBRARY)
ENDIF(MED_INCLUDE_DIR)

IF(${MED_FOUND})
  IF(WINDOWS)
    SET(MED_LIBRARIES ${MED_LIBRARIES} ${HDF5_LIBS})
    SET(MED_INCLUDE_DIRS ${MED_INCLUDE_DIR} ${HDF5_INCLUDE_DIR})
  ELSE(WINDOWS)
    FIND_PACKAGE(HDF5 REQUIRED)
    SET(MED_LIBRARIES ${MED_LIBRARIES} ${HDF5_LIBRARIES})
    SET(MED_INCLUDE_DIRS ${MED_INCLUDE_DIR} ${HDF5_INCLUDE_DIRS})
    IF(${HDF5_IS_PARALLEL})
      FIND_PACKAGE(MPI REQUIRED)
      SET(MED_LIBRARIES ${MED_LIBRARIES} ${MPI_LIBRARY} ${MPI_EXTRA_LIBRARY})
      SET(MED_INCLUDE_DIRS ${MED_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})
    ENDIF(${HDF5_IS_PARALLEL})
  ENDIF(WINDOWS)
ENDIF(${MED_FOUND})

SET(MED_INCLUDE_DIR ${MED_INCLUDE_DIRS})
