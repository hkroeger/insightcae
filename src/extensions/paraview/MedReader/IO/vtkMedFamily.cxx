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

#include "vtkMedFamily.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include "vtkMedGroup.h"
#include "vtkMedUtilities.h"

vtkCxxGetObjectVectorMacro(vtkMedFamily, Group, vtkMedGroup);
vtkCxxSetObjectVectorMacro(vtkMedFamily, Group, vtkMedGroup);

vtkStandardNewMacro(vtkMedFamily)

vtkMedFamily::vtkMedFamily()
{
  this->Id = 0;
  this->Name = NULL;
  this->Group = new vtkObjectVector<vtkMedGroup>();
  this->MedIterator = -1;
  this->PointOrCell = vtkMedUtilities::OnPoint;

  // by default, the family is part of the "NoGroup" fake group
  this->AllocateNumberOfGroup(1);
  vtkMedGroup* nogroup = this->GetGroup(0);
  nogroup->SetName(vtkMedUtilities::NoGroupName);

  this->SetName("UNDEFINED_FAMILY");
}

vtkMedFamily::~vtkMedFamily()
{
  this->SetName(NULL);
  delete this->Group;
}

void vtkMedFamily::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  PRINT_IVAR(os, indent, Id);
  PRINT_IVAR(os, indent, MedIterator);
  PRINT_IVAR(os, indent, PointOrCell);
  PRINT_OBJECT_VECTOR(os, indent, Group);
}
