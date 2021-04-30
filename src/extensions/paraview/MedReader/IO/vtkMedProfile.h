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

#ifndef __vtkMedProfile_h_
#define __vtkMedProfile_h_

#include "vtkObject.h"
#include "vtkMed.h"

class vtkMedIntArray;
class vtkMedString;
class vtkMedFile;

class VTK_EXPORT vtkMedProfile : public vtkObject
{
public :
  static vtkMedProfile* New();
  vtkTypeMacro(vtkMedProfile, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the number of elements of this profile
  vtkSetMacro(NumberOfElement, vtkIdType);
  vtkGetMacro(NumberOfElement, vtkIdType);

  // Description:
  // The name of the profile in the file
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // Allocate and clear memory of this profil.
  virtual void  SetIds(vtkMedIntArray*);
  vtkGetObjectMacro(Ids, vtkMedIntArray);

  // Description:
  // return true if the index array is not null and the size match
  virtual int IsLoaded();

  // Description:
  // The index of this field in the med file
  vtkSetMacro(MedIterator, med_int);
  vtkGetMacro(MedIterator, med_int);

  // Description:
  // Load the profile ids
  void  Load();

  // Description:
  // this is the file where this Profile is stored
  virtual void  SetParentFile(vtkMedFile*);
  vtkGetObjectMacro(ParentFile, vtkMedFile);

protected:
  vtkMedProfile();
  virtual ~vtkMedProfile();

  med_int MedIterator;
  vtkIdType NumberOfElement;
  vtkMedIntArray* Ids;
  char* Name;
  med_geometry_type GeometryType;
  vtkMedFile* ParentFile;

private:
  vtkMedProfile(const vtkMedProfile&); // Not implemented.
  void operator=(const vtkMedProfile&); // Not implemented.

};

#endif //__vtkMedProfil_h_
