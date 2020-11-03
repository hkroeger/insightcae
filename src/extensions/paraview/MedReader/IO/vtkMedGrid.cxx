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

#include "vtkMedGrid.h"

#include "vtkObjectFactory.h"

#include "vtkMedUtilities.h"
#include "vtkMedMesh.h"
#include "vtkMedIntArray.h"
#include "vtkMedFamilyOnEntity.h"
#include "vtkMedFamily.h"
#include "vtkMedEntityArray.h"
#include "vtkMedFile.h"
#include "vtkMedDriver.h"

#include <set>
using std::set;

vtkCxxGetObjectVectorMacro(vtkMedGrid, EntityArray, vtkMedEntityArray);
vtkCxxSetObjectVectorMacro(vtkMedGrid, EntityArray, vtkMedEntityArray);

vtkCxxSetObjectMacro(vtkMedGrid, PointGlobalIds, vtkMedIntArray);
vtkCxxSetObjectMacro(vtkMedGrid, ParentMesh, vtkMedMesh);
vtkCxxSetObjectMacro(vtkMedGrid, PreviousGrid, vtkMedGrid);


vtkMedGrid::vtkMedGrid()
{
  this->ParentMesh = NULL;
  this->PointGlobalIds = NULL;
  this->PreviousGrid = NULL;

  this->CoordinateSystem = MED_CARTESIAN;
  this->EntityArray = new vtkObjectVector<vtkMedEntityArray>();
  this->UsePreviousCoordinates = false;
}

vtkMedGrid::~vtkMedGrid()
{
  this->SetPointGlobalIds(NULL);
  this->SetParentMesh(NULL);
  delete this->EntityArray;
}

int vtkMedGrid::IsPointGlobalIdsLoaded()
{
  return this->PointGlobalIds != NULL
      && this->PointGlobalIds->GetNumberOfTuples()
          == this->GetNumberOfPoints();
}

void  vtkMedGrid::ClearMedSupports()
{
  this->SetPointGlobalIds(NULL);
}

vtkMedEntityArray* vtkMedGrid::GetEntityArray(const vtkMedEntity& entity)
{
  for(int id = 0; id < this->EntityArray->size(); id++)
    {
    vtkMedEntityArray* array = this->EntityArray->at(id);
    if(array->GetEntity() == entity)
      return array;
    }
  return NULL;
}

void  vtkMedGrid::GatherMedEntities(std::set<vtkMedEntity>& entities)
{
  for(int id = 0; id < this->EntityArray->size(); id++)
    {
    vtkMedEntityArray* array = this->EntityArray->at(id);
    entities.insert(array->GetEntity());
    }
}

void vtkMedGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
