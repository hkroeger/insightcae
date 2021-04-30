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

#set(MEDREADER_HDRS)
#set(EXTRACTGROUP_HDRS)

#include(${MedReader_BINARY_DIR}/IO/vtkMedReader_IOKit.cmake)

#include_directories(${VTK_MEDREADER_IO_HEADER_DIR})

#add_definitions( -I${CMAKE_CURRENT_BINARY_DIR}/../IO )

#foreach(class ${VTK_MEDREADER_IO_CLASSES})
#    set(MEDREADER_HDRS ${MEDREADER_HDRS} ${VTK_MEDREADER_IO_HEADER_DIR}/${class}.h)
#endforeach(class)

#foreach(class ${VTK_MEDREADER_IO_CLASSES_ABSTRACT})
#    set_source_files_properties(${VTK_MEDREADER_IO_HEADER_DIR}/${class}.h PROPERTIES ABSTRACT 1)
#endforeach(class)

#foreach(class ${VTK_MEDREADER_IO_CLASSES_WRAP_EXCLUDE})
#    set_source_files_properties(${VTK_MEDREADER_IO_HEADER_DIR}/${class}.h PROPERTIES WRAP_EXCLUDE 1)
#endforeach(class)

SET(SM_SRCS
    #vtkSMMedGroupSelectionDomain.cxx
    )

IF(PARAVIEW_BUILD_QT_GUI)

    #find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

    set(CLIENT_INTERFACES)
    SET(CLIENT_UI_SRCS
        #MedReaderPanel.ui
        #ExtractGroupPanel.ui
    )

    # create a plugin that implements an object panel for the reader
    SET(CLIENT_SRCS
        #pqMedReaderPanel.cxx
        #pqExtractGroupPanel.cxx
        #pqMedReaderPanel.h
        #pqExtractGroupPanel.h
    )


#    paraview_plugin_add_property_widget(
#        KIND WIDGET
#        TYPE "medReaderPanel"
#        CLASS_NAME pqMedReaderPanel
#        INTERFACES dock1_interfaces
#        SOURCES dock1_sources
#    )
#    list(APPEND CLIENT_INTERFACES ${dock1_interfaces})
#    list(APPEND CLIENT_SRCS ${dock1_sources})

#    paraview_plugin_add_property_widget(
#        KIND WIDGET
#        TYPE "extractGroupPanel"
#        CLASS_NAME pqExtractGroupPanel
#        INTERFACES dock2_interfaces
#        SOURCES dock2_sources
#    )
#    list(APPEND CLIENT_INTERFACES ${dock2_interfaces})
#    list(APPEND CLIENT_SRCS ${dock2_sources})
endif()

paraview_add_plugin(MedReader
  VERSION "4.0"
  UI_FILES ${CLIENT_UI_SRCS}
  UI_INTERFACES ${CLIENT_INTERFACES}
  REQUIRED_ON_SERVER
  SERVER_MANAGER_XML Resources/MedReaderServer.xml
  MODULES ${vtkMedReaderTargetName}
  SOURCES ${CLIENT_SRCS} ${SM_SRCS}
)


TARGET_LINK_LIBRARIES(MedReader PUBLIC ${vtkMedReaderTargetName})





