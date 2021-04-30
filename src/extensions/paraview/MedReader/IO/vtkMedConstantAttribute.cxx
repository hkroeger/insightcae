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

#include "vtkMedConstantAttribute.h"

#include "vtkObjectFactory.h"
#include "vtkMedUtilities.h"

#include "vtkAbstractArray.h"
#include "vtkMedStructElement.h"
#include "vtkMedProfile.h"

#include <map>
#include <string>
#include <vector>
using namespace std;

vtkStandardNewMacro(vtkMedConstantAttribute);

vtkCxxSetObjectMacro(vtkMedConstantAttribute, Values, vtkAbstractArray);
vtkCxxSetObjectMacro(vtkMedConstantAttribute, ParentStructElement, vtkMedStructElement);
vtkCxxSetObjectMacro(vtkMedConstantAttribute, Profile, vtkMedProfile);

vtkMedConstantAttribute::vtkMedConstantAttribute()
{
  this->Name = NULL;
  this->AttributeType = MED_ATT_UNDEF;
  this->NumberOfComponent = 0;
  this->SupportEntityType = MED_UNDEF_ENTITY_TYPE;
  this->ProfileName = NULL;
  this->ProfileSize = 0;
  this->Values = NULL;
  this->ParentStructElement = NULL;
  this->Profile = NULL;
}

vtkMedConstantAttribute::~vtkMedConstantAttribute()
{
  this->SetName(NULL);
  this->SetProfileName(NULL);
  this->SetParentStructElement(NULL);
  this->SetValues(NULL);
  this->SetProfile(NULL);
}

void vtkMedConstantAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
