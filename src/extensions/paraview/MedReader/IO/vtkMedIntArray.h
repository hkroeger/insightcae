// Copyright (C) 2010-2012  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

// .NAME vtkMedIntArray - dynamic, self-adjusting array of med_int
// .SECTION Description
// vtkMedIntArray is an array of values of type med_int.
// It provides methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.
// If the med_int type is the same as the vtkIdType,
// this class inherits from the
// vtkIdType array, allowing safe shallow copies.

#ifndef __vtkMedIntArray_h
#define __vtkMedIntArray_h

#include "vtkMedUtilities.h"
#include "vtkMed.h"

#include "vtkMedIntArrayInternal.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"

//BTX
template <class T1> struct med_int_vtkIdType_Traits : IsSameTraits<T1, med_int>
{
  typedef vtkMedIntArrayInternal Superclass;
};

template <> struct med_int_vtkIdType_Traits<med_int>
  : IsSameTraits<med_int, med_int>
{
  typedef vtkIdTypeArray Superclass;
};
typedef med_int_vtkIdType_Traits<vtkIdType>::Superclass
  vtkMedIntArraySuperclass;
//ETX

class VTK_EXPORT vtkMedIntArray
//BTX
: public vtkMedIntArraySuperclass
//ETX
{
public :
  static vtkMedIntArray* New();
  vtkTypeMacro(vtkMedIntArray,vtkMedIntArraySuperclass);
  void PrintSelf(ostream& os, vtkIndent indent);
protected:
  vtkMedIntArray(vtkIdType numComp=1);
  ~vtkMedIntArray();
};

#endif
