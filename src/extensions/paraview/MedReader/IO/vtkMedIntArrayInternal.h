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

// .NAME vtkMedIntArrayInternal - dynamic, self-adjusting array of med_int
// .SECTION Description
// vtkMedIntArray is an array of values of type med_int.
// It provides methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef __vtkMedIntArrayInternal_h
#define __vtkMedIntArrayInternal_h

#include "vtkMed.h"

// Tell the template header how to give our superclass a DLL interface.
#if !defined(__vtkMedIntArrayInternal_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE med_int
#endif

#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#define vtkDataArray vtkAOSDataArrayTemplate<med_int>
class VTK_EXPORT vtkMedIntArrayInternal : public vtkDataArray
#undef vtkDataArray
{
public:
  static vtkMedIntArrayInternal* New();
  vtkTypeMacro(vtkMedIntArrayInternal,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the data type.
  // This returns the
  int GetDataType()
    { if(sizeof(med_int) == sizeof(vtkIdType)) return VTK_ID_TYPE;
      if(sizeof(med_int) == sizeof(int)) return VTK_INT;
      if(sizeof(med_int) == sizeof(long)) return VTK_LONG;
      return VTK_VOID;
     }

  // Description:
  // Copy the tuple value into a user-provided array.
  void GetTuple(vtkIdType i, med_int* tuple)
    { this->RealSuperclass::GetTypedTuple(i, tuple); }
  void GetTupleValue(vtkIdType i, med_int* tuple) { GetTuple(i, tuple); }

  // Description:
  // Set the tuple value at the ith location in the array.
  void SetTuple(vtkIdType i, const med_int* tuple)
    { this->RealSuperclass::SetTypedTuple(i, tuple); }
  void SetTupleValue(vtkIdType i, const med_int* tuple) { SetTuple(i, tuple); }

  // Description:
  // Insert (memory allocation performed) the tuple into the ith location
  // in the array.
  void InsertTuple(vtkIdType i, const med_int* tuple)
    { this->RealSuperclass::InsertTypedTuple(i, tuple); }
  void InsertTupleValue(vtkIdType i, const med_int* tuple) { InsertTuple(i, tuple); }

  // Description:
  // Insert (memory allocation performed) the tuple onto the end of the array.
  vtkIdType InsertNextTuple(const med_int* tuple)
    { return this->RealSuperclass::InsertNextTypedTuple(tuple); }
  vtkIdType InsertNextTupleValue(const med_int* tuple) { return InsertNextTuple(tuple); }

  // Description:
  // Get the data at a particular index.
  med_int GetValue(vtkIdType id)
    { return this->RealSuperclass::GetValue(id); }

  // Description:
  // Set the data at a particular index. Does not do range checking. Make sure
  // you use the method SetNumberOfValues() before inserting data.
  void SetValue(vtkIdType id, med_int value)
    { this->RealSuperclass::SetValue(id, value); }

  // Description:
  // Specify the number of values for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetValue() method for fast insertion.
  bool SetNumberOfValues(vtkIdType number)
    { return this->RealSuperclass::SetNumberOfValues(number); }

  // Description:
  // Insert data at a specified position in the array.
  void InsertValue(vtkIdType id, med_int f)
    { this->RealSuperclass::InsertValue(id, f); }

  // Description:
  // Insert data at the end of the array. Return its location in the array.
  vtkIdType InsertNextValue(med_int f)
    { return this->RealSuperclass::InsertNextValue(f); }

  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  med_int* WritePointer(vtkIdType id, vtkIdType number)
    { return this->RealSuperclass::WritePointer(id, number); }

  // Description:
  // Get the address of a particular data index. Performs no checks
  // to verify that the memory has been allocated etc.
  med_int* GetPointer(vtkIdType id)
    { return this->RealSuperclass::GetPointer(id); }

  // Description:
  // This method lets the user specify data to be held by the array.  The
  // array argument is a pointer to the data.  size is the size of
  // the array supplied by the user.  Set save to 1 to keep the class
  // from deleting the array when it cleans up or reallocates memory.
  // The class uses the actual array provided; it does not copy the data
  // from the suppled array.
  void SetArray(med_int* array, vtkIdType size, int save)
    { this->RealSuperclass::SetArray(array, size, save); }
  void SetArray(med_int* array, vtkIdType size, int save, int deleteMethod)
    { this->RealSuperclass::SetArray(array, size, save, deleteMethod); }

protected:
  vtkMedIntArrayInternal(vtkIdType numComp=1);
  ~vtkMedIntArrayInternal();

private:
  //BTX
  typedef vtkAOSDataArrayTemplate<med_int> RealSuperclass;
  //ETX
  vtkMedIntArrayInternal(const vtkMedIntArrayInternal&);  // Not implemented.
  void operator=(const vtkMedIntArrayInternal&);  // Not implemented.
};

#endif
