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

#include "vtkMedFieldOverEntity.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include "vtkMedUtilities.h"
#include "vtkMedMesh.h"
#include "vtkDataArray.h"
#include "vtkMedFieldOnProfile.h"
#include "vtkMedFieldStep.h"
#include "vtkMedField.h"

vtkCxxGetObjectVectorMacro(vtkMedFieldOverEntity, FieldOnProfile, vtkMedFieldOnProfile);
vtkCxxSetObjectVectorMacro(vtkMedFieldOverEntity, FieldOnProfile, vtkMedFieldOnProfile);

vtkCxxSetObjectMacro(vtkMedFieldOverEntity, ParentStep, vtkMedFieldStep);

vtkStandardNewMacro(vtkMedFieldOverEntity)

vtkMedFieldOverEntity::vtkMedFieldOverEntity()
{
  this->FieldOnProfile = new vtkObjectVector<vtkMedFieldOnProfile>();
  this->HasProfile = false;
  this->ParentStep = NULL;
}

vtkMedFieldOverEntity::~vtkMedFieldOverEntity()
{
  delete this->FieldOnProfile;
  this->SetParentStep(NULL);
}

void vtkMedFieldOverEntity::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
