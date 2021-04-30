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

include_directories(../..)

set(TestMedReader_SRCS
    TestMedReader.cxx
)

set(TestMedPolyhedron_SRCS
    TestMedPolyhedron.cxx
)

set(TestMedReadPolyhedron_SRCS
    TestMedReadPolyhedron.cxx
)

set(TestMedDescendingPolyhedron_SRCS
    TestMedDescendingPolyhedron.cxx
)

set(TestMedReadDescendingPolyhedron_SRCS
    TestMedReadDescendingPolyhedron.cxx
)

set(TestMedWriteOcta12_SRCS
    TestMedWriteOcta12.cxx
)

set(TestMedParallelRead_SRCS
    TestMedParallelRead.cxx
)

set(TestMedParallelWrite_SRCS
    TestMedParallelWrite.cxx
)

add_executable(TestMedReader ${TestMedReader_SRCS})
add_executable(TestMedPolyhedron ${TestMedPolyhedron_SRCS})
add_executable(TestMedReadPolyhedron ${TestMedReadPolyhedron_SRCS})
add_executable(TestMedDescendingPolyhedron ${TestMedDescendingPolyhedron_SRCS})
add_executable(TestMedReadDescendingPolyhedron ${TestMedReadDescendingPolyhedron_SRCS})
add_executable(TestMedWriteOcta12 ${TestMedWriteOcta12_SRCS})
#add_executable(TestMedParallelRead ${TestMedParallelRead_SRCS})
#add_executable(TestMedParallelWrite ${TestMedParallelWrite_SRCS})

#set(VTK_LIBS vtkMedReader vtkRendering)
#if(${HDF5_IS_PARALLEL})
#  set(VTK_LIBS ${VTK_LIBS} vtkParallel)
#endif(${HDF5_IS_PARALLEL})
SET(VTK_LIBS ${VTK_LIBRARIES} vtkMedReader)

target_link_libraries(TestMedReader ${VTK_LIBS} ${MED_LIBRARIES})
target_link_libraries(TestMedPolyhedron ${MED_LIBRARIES})
target_link_libraries(TestMedReadPolyhedron ${MED_LIBRARIES})
target_link_libraries(TestMedDescendingPolyhedron ${MED_LIBRARIES})
target_link_libraries(TestMedReadDescendingPolyhedron ${MED_LIBRARIES})
target_link_libraries(TestMedWriteOcta12 ${MED_LIBRARIES})
#target_link_libraries(TestMedParallelRead ${MED_LIBRARIES} MEDReader vtkParallel)
#target_link_libraries(TestMedParallelWrite ${MED_LIBRARIES} MEDReader vtkParallel)

