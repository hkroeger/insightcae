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

#include "vtkMedStructElement.h"

#include "vtkObjectFactory.h"
#include "vtkMedUtilities.h"
#include "vtkMedVariableAttribute.h"
#include "vtkMedConstantAttribute.h"
#include "vtkMedFile.h"
#include "vtkMedMesh.h"

#include <map>
#include <string>
#include <vector>
using namespace std;

vtkCxxGetObjectVectorMacro(vtkMedStructElement, VariableAttribute, vtkMedVariableAttribute);
vtkCxxSetObjectVectorMacro(vtkMedStructElement, VariableAttribute, vtkMedVariableAttribute);
vtkCxxGetObjectVectorMacro(vtkMedStructElement, ConstantAttribute, vtkMedConstantAttribute);
vtkCxxSetObjectVectorMacro(vtkMedStructElement, ConstantAttribute, vtkMedConstantAttribute);

vtkCxxSetObjectMacro(vtkMedStructElement, ParentFile, vtkMedFile);
vtkCxxSetObjectMacro(vtkMedStructElement, SupportMesh, vtkMedMesh);

vtkStandardNewMacro(vtkMedStructElement)

vtkMedStructElement::vtkMedStructElement()
{
  this->MedIterator = -1;
  this->Name = NULL;
  this->GeometryType = MED_UNDEF_GEOTYPE;
  this->ModelDimension = 0;
  this->SupportMeshName = NULL;
  this->SupportMesh = NULL;
  this->SupportEntityType = MED_UNDEF_ENTITY_TYPE;
  this->SupportNumberOfNode = 0;
  this->SupportNumberOfCell = 0;
  this->SupportGeometryType = MED_UNDEF_GEOTYPE;
  this->AnyProfile = MED_FALSE;
  this->VariableAttribute = new vtkObjectVector<vtkMedVariableAttribute>();
  this->ConstantAttribute = new vtkObjectVector<vtkMedConstantAttribute>();
  this->ParentFile = NULL;
}

vtkMedStructElement::~vtkMedStructElement()
{
  this->SetName(NULL);
  this->SetSupportMeshName(NULL);
  this->SetParentFile(NULL);
  this->SetSupportMesh(NULL);
  delete this->VariableAttribute;
  delete this->ConstantAttribute;
}

void  vtkMedStructElement::LoadVariableAttributes(vtkMedEntityArray* array)
{
  for(int varattit = 0; varattit < this->GetNumberOfVariableAttribute(); varattit++)
    {
    vtkMedVariableAttribute* varatt = this->GetVariableAttribute(varattit);
    varatt->Load(array);
    }
}

int vtkMedStructElement::GetConnectivitySize()
{
  if(strcmp(this->Name, MED_PARTICLE_NAME) == 0
     || this->SupportEntityType != MED_CELL)
    return this->SupportNumberOfNode;

  return this->SupportNumberOfCell *
      vtkMedUtilities::GetNumberOfPoint(this->SupportGeometryType);
}

void vtkMedStructElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
