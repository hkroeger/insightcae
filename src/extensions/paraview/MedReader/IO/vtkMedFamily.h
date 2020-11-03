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

#ifndef __vtkMedFamily_h_
#define __vtkMedFamily_h_

#include "vtkObject.h"
#include "vtkMedSetGet.h"
#include "vtkMed.h"

class vtkMedMesh;
class vtkMedGroup;
class vtkMedString;

class VTK_EXPORT vtkMedFamily: public vtkObject
{
public:
  static vtkMedFamily* New();
  vtkTypeMacro(vtkMedFamily, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the name of this family.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // The id of this family.
  vtkGetMacro(Id, med_int);
  vtkSetMacro(Id, med_int);

  // Description:
  // Each family can be part of one or several groups.
  // This give access to the group names.
  vtkGetObjectVectorMacro(Group, vtkMedGroup);
  vtkSetObjectVectorMacro(Group, vtkMedGroup);

  // Description:
  // returns id this is a node or a cell centered family.
  vtkSetMacro(PointOrCell, int);
  vtkGetMacro(PointOrCell, int);

  // Description:
  // the index of this field in the med file.
  vtkSetMacro(MedIterator, med_int);
  vtkGetMacro(MedIterator, med_int);

protected:
  vtkMedFamily();
  virtual ~vtkMedFamily();

  med_int Id;
  med_int MedIterator;
  char* Name;
  int PointOrCell;

  //BTX
  vtkObjectVector<vtkMedGroup>* Group;
  //ETX

private:
  vtkMedFamily(const vtkMedFamily&); // Not implemented.
  void operator=(const vtkMedFamily&); // Not implemented.

};

#endif //__vtkMedFamily_h_
