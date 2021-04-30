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

#ifndef __vtkMedGroup_h_
#define __vtkMedGroup_h_

#include "vtkObject.h"

class vtkMedString;

class VTK_EXPORT vtkMedGroup : public vtkObject
{
public :
  static vtkMedGroup* New();
  vtkTypeMacro(vtkMedGroup, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the description of this file as stored in the med file.
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // This ivar says if this group contains point or cell families
  // This value is either vtkMedUtilities::OnPoint or vtkMedUtilities::OnCell
//  vtkSetMacro(PointOrCell, int);
//  vtkGetMacro(PointOrCell, int);

protected:
  vtkMedGroup();
  virtual ~vtkMedGroup();

  char* Name;

private:
  vtkMedGroup(const vtkMedGroup&); // Not implemented.
  void operator=(const vtkMedGroup&); // Not implemented.

};

#endif //__vtkMedGroup_h_
