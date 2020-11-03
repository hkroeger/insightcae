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

#include "vtkMedFamilyOnEntity.h"

#include "vtkObjectFactory.h"
#include "vtkMedUtilities.h"
#include "vtkMedFamily.h"
#include "vtkMedEntityArray.h"
#include "vtkMedMesh.h"
#include "vtkMedGrid.h"
#include "vtkMedFamilyOnEntityOnProfile.h"

vtkCxxSetObjectMacro(vtkMedFamilyOnEntity, Family, vtkMedFamily);
vtkCxxSetObjectMacro(vtkMedFamilyOnEntity, EntityArray, vtkMedEntityArray);

vtkCxxSetObjectMacro(vtkMedFamilyOnEntity, ParentGrid,vtkMedGrid);

vtkStandardNewMacro(vtkMedFamilyOnEntity)

vtkMedFamilyOnEntity::vtkMedFamilyOnEntity()
{
  this->Family = NULL;
  this->EntityArray = NULL;
  this->ParentGrid = NULL;
}

vtkMedFamilyOnEntity::~vtkMedFamilyOnEntity()
{
  this->SetFamily(NULL);
  this->SetEntityArray(NULL);
  this->SetParentGrid(NULL);
}

vtkMedEntity vtkMedFamilyOnEntity::GetEntity()
{
  if(this->EntityArray != NULL)
    {
    return this->EntityArray->GetEntity();
    }

  return vtkMedEntity(MED_NODE, MED_POINT1);
}

int vtkMedFamilyOnEntity::GetPointOrCell()
{
  if(this->GetEntity().EntityType == MED_NODE)
    return vtkMedUtilities::OnPoint;
  return vtkMedUtilities::OnCell;
}

int vtkMedFamilyOnEntity::GetVertexOnly()
{
  if(this->GetPointOrCell() == vtkMedUtilities::OnPoint ||
     this->EntityArray == NULL)
    return true;

  vtkMedEntity entity = this->EntityArray->GetEntity();
  if(entity.EntityType == MED_POINT1 || entity.GeometryType == MED_NONE)
    return true;

  if(entity.GeometryName == MED_BALL_NAME && entity.EntityType == MED_STRUCT_ELEMENT)
    return true;

  if(entity.GeometryName == MED_PARTICLE_NAME && entity.EntityType == MED_STRUCT_ELEMENT)
    return true;

  return false;
}

void  vtkMedFamilyOnEntity::AddFamilyOnEntityOnProfile(
    vtkMedFamilyOnEntityOnProfile* foep)
{
  this->FamilyOnEntityOnProfile[foep->GetProfile()] = foep;
}

int vtkMedFamilyOnEntity::GetNumberOfFamilyOnEntityOnProfile()
{
  return this->FamilyOnEntityOnProfile.size();
}

vtkMedFamilyOnEntityOnProfile* vtkMedFamilyOnEntity::
    GetFamilyOnEntityOnProfile(vtkMedProfile* profile)
{
  if(this->FamilyOnEntityOnProfile.find(profile)
    != this->FamilyOnEntityOnProfile.end())
    return this->FamilyOnEntityOnProfile[profile];

  return NULL;
}

vtkMedFamilyOnEntityOnProfile* vtkMedFamilyOnEntity::
    GetFamilyOnEntityOnProfile(int index)
{
  if(index < 0 || index >= this->FamilyOnEntityOnProfile.size())
    return NULL;

  std::map<vtkMedProfile*,
  vtkSmartPointer<vtkMedFamilyOnEntityOnProfile> >::iterator it =
  this->FamilyOnEntityOnProfile.begin();

  for(int ii=0; ii<index; ii++)
    it++;

  return it->second;
}

void vtkMedFamilyOnEntity::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  PRINT_OBJECT(os, indent, Family);
  PRINT_OBJECT(os, indent, EntityArray);
}
