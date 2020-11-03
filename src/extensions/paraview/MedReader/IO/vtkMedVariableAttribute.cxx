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

#include "vtkMedVariableAttribute.h"

#include "vtkObjectFactory.h"
#include "vtkMedUtilities.h"
#include "vtkMedStructElement.h"
#include "vtkMedFile.h"
#include "vtkMedDriver.h"

#include <map>
#include <string>
#include <vector>
using namespace std;

vtkStandardNewMacro(vtkMedVariableAttribute);

vtkCxxSetObjectMacro(vtkMedVariableAttribute, ParentStructElement, vtkMedStructElement);

vtkMedVariableAttribute::vtkMedVariableAttribute()
{
  this->Name = NULL;
  this->AttributeType = MED_ATT_UNDEF;
  this->NumberOfComponent = 0;
  this->ParentStructElement = NULL;
}

vtkMedVariableAttribute::~vtkMedVariableAttribute()
{
  this->SetName(NULL);
  this->SetParentStructElement(NULL);
}

void  vtkMedVariableAttribute::Load(vtkMedEntityArray* array)
{
  this->GetParentStructElement()->GetParentFile()->GetMedDriver()->LoadVariableAttribute(this, array);
}

void vtkMedVariableAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
