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

set(MedReader_HEADERS
    vtkMedSetGet.h
)

vtk_module_add_module(${vtkMedReaderTargetName}
  CLASSES ${MedReader_CLASSES}
  HEADERS ${MedReader_HEADERS}
)

CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
target_include_directories(${vtkMedReaderTargetName} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} )

target_link_libraries(${vtkMedReaderTargetName} PUBLIC
    ${MED_LIBRARIES}
    VTK::CommonExecutionModel
    ${VTK_PARALLEL}
    ${vtkPVServerManager}
  )

