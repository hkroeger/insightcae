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


set(MedReader_CLASSES
    vtkMedReader
    vtkMedDriver
    vtkMedDriver30
    vtkMedFactory
    vtkMedFile
    vtkMedMesh
    vtkMedGrid
    vtkMedRegularGrid
    vtkMedPolarGrid
    vtkMedCartesianGrid
    vtkMedCurvilinearGrid
    vtkMedUnstructuredGrid
    vtkMedFamily
    vtkMedGroup
    vtkMedField
    vtkMedEntityArray
    vtkMedProfile
    vtkMedLocalization
    vtkMedFieldOverEntity
    vtkMedFieldStep
    vtkMedUtilities
    #vtkMedString
    vtkMedIntArrayInternal
    vtkMedIntArray
    vtkMedFamilyOnEntity
    vtkMedInterpolation
    vtkMedFraction
    vtkMedFilter
    vtkMedFieldOnProfile
    vtkExtractGroup
    vtkMedSelection
    vtkMedLink
    vtkMedFamilyOnEntityOnProfile
    vtkMedConstantAttribute
    vtkMedVariableAttribute
    vtkMedStructElement
    vtkGenerateStructElement
)

#set_source_files_properties(
#  vtkMedGrid
#  vtkMedRegularGrid
#  vtkMedUtilities
#  PROPERTIES ABSTRACT 1
#)

#set_source_files_properties(
#  vtkMedIntArrayInternal
#  vtkMedUtilities
#  PROPERTIES WRAP_EXCLUDE 1
#)

#add_library(vtkMedReader ${MedReader_SRCS})

#set(VTK_LIBS vtkIO)
#if(${HDF5_IS_PARALLEL})
#  set(VTK_LIBS ${VTK_LIBS} vtkParallel)
#endif(${HDF5_IS_PARALLEL})

# Introducing selection for parallel compilation

#add_definitions( -I${CMAKE_CURRENT_BINARY_DIR} )

vtk_module_add_module(vtkMedReader
  FORCE_STATIC
  CLASSES ${MedReader_CLASSES}
)

CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
target_include_directories(vtkMedReader PUBLIC ${CMAKE_CURRENT_BINARY_DIR} )

target_link_libraries(vtkMedReader PUBLIC
    ${MED_LIBRARIES}
    VTK::CommonExecutionModel
    ${VTK_PARALLEL}
    ${vtkPVServerManager}
  )

# Install
INSTALL(
    TARGETS vtkMedReader
    RUNTIME DESTINATION lib/paraview
    LIBRARY DESTINATION lib/paraview
    ARCHIVE DESTINATION lib/paraview
)
