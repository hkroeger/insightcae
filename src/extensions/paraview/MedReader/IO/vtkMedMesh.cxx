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

#include "vtkMedMesh.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkDataArray.h"

#include "vtkMedFamily.h"
#include "vtkMedFamilyOnEntity.h"
#include "vtkMedGroup.h"
#include "vtkMedGrid.h"
#include "vtkMedUtilities.h"
#include "vtkMedIntArray.h"
#include "vtkMedFile.h"

#include <sstream>

vtkCxxGetObjectVectorMacro(vtkMedMesh, CellFamily, vtkMedFamily);
vtkCxxSetObjectVectorMacro(vtkMedMesh, CellFamily, vtkMedFamily);
vtkCxxGetObjectVectorMacro(vtkMedMesh, PointFamily, vtkMedFamily);
vtkCxxSetObjectVectorMacro(vtkMedMesh, PointFamily, vtkMedFamily);

vtkCxxGetObjectVectorMacro(vtkMedMesh, PointGroup, vtkMedGroup);
vtkCxxSetObjectVectorMacro(vtkMedMesh, PointGroup, vtkMedGroup);
vtkCxxGetObjectVectorMacro(vtkMedMesh, CellGroup, vtkMedGroup);
vtkCxxSetObjectVectorMacro(vtkMedMesh, CellGroup, vtkMedGroup);

vtkCxxSetObjectMacro(vtkMedMesh, ParentFile, vtkMedFile);

vtkStandardNewMacro(vtkMedMesh)

vtkMedMesh::vtkMedMesh()
{
  this->GridStep = new vtkMedComputeStepMap<vtkMedGrid> ();
  this->Name = NULL;
  this->UniversalName = NULL;
  this->Description = NULL;
  this->TimeUnit = NULL;
  this->CellFamily = new vtkObjectVector<vtkMedFamily> ();
  this->PointFamily = new vtkObjectVector<vtkMedFamily> ();
  this->PointGroup = new vtkObjectVector<vtkMedGroup> ();
  this->CellGroup = new vtkObjectVector<vtkMedGroup> ();
  this->AxisName = vtkStringArray::New();
  this->AxisUnit = vtkStringArray::New();
  this->AxisName->SetNumberOfValues(3);
  this->AxisUnit->SetNumberOfValues(3);
  this->MedIterator = -1;
  this->MeshType = MED_UNDEF_MESH_TYPE;
  this->StructuredGridType = MED_UNDEF_GRID_TYPE;
  this->ParentFile = NULL;
  this->AxisType = MED_CARTESIAN;
  this->SortingType = MED_SORT_DTIT;
  this->MeshDimension = 3;
  this->SpaceDimension = 3;
  this->IsSupportMesh = 0;
}

vtkMedMesh::~vtkMedMesh()
{
  this->SetName(NULL);
  this->SetUniversalName(NULL);
  this->SetDescription(NULL);
  delete this->CellFamily;
  delete this->PointFamily;
  delete this->PointGroup;
  delete this->CellGroup;
  this->AxisName->Delete();
  this->AxisUnit->Delete();
  delete this->GridStep;
}

vtkMedGroup* vtkMedMesh::GetOrCreateGroup(int pointOrCell, const char* name)
{
  if(pointOrCell == vtkMedUtilities::OnCell)
    {
    for(int g = 0; g < this->CellGroup->size(); g++)
      {
      vtkMedGroup* group = this->CellGroup->at(g);
      if(group != NULL && strcmp(name, group->GetName()) == 0)
        {
        return group;
        }
      }
    vtkMedGroup* group = vtkMedGroup::New();
    this->CellGroup->push_back(group);
    //group->SetPointOrCell(vtkMedUtilities::OnCell);
    group->SetName(name);
    group->Delete();
    return group;
    }
  else
    {
    for(int g = 0; g < this->PointGroup->size(); g++)
      {
      vtkMedGroup* group = this->PointGroup->at(g);
      if(group != NULL && strcmp(name, group->GetName()) == 0)
        {
        return group;
        }
      }
    vtkMedGroup* group = vtkMedGroup::New();
    this->CellGroup->push_back(group);
    //group->SetPointOrCell(vtkMedUtilities::OnPoint);
    group->SetName(name);
    group->Delete();
    return group;
    }
  return NULL;
}

int vtkMedMesh::GetNumberOfFamily()
{
  return this->GetNumberOfCellFamily() + this->GetNumberOfPointFamily();
}

vtkMedFamily* vtkMedMesh::GetFamily(int index)
{
  if(index < 0)
    return NULL;
  if(index < this->GetNumberOfCellFamily())
    return this->GetCellFamily(index);
  else if(index < GetNumberOfFamily())
    return this->GetPointFamily(index - this->GetNumberOfCellFamily());
  else return NULL;
}

vtkMedFamily* vtkMedMesh::GetOrCreateCellFamilyById(med_int id)
{
  for(int i = 0; i < this->GetNumberOfCellFamily(); i++)
    {
    vtkMedFamily* family = this->GetCellFamily(i);
    if(family->GetId() == id)
      {
      return family;
      }
    }
  vtkMedFamily* family = vtkMedFamily::New();
  family->SetId(id);
  std::ostringstream sstr;
  sstr << "UNDEFINED_CELL_FAMILY_" << id;
  family->SetName(sstr.str().c_str());
  family->SetPointOrCell(vtkMedUtilities::OnCell);
  family->SetMedIterator(-1);
  this->AppendCellFamily(family);
  family->Delete();
  return family;
}

void  vtkMedMesh::SetNumberOfAxis(int naxis)
{
  this->AxisName->SetNumberOfValues(naxis);
  this->AxisUnit->SetNumberOfValues(naxis);
}

int  vtkMedMesh::GetNumberOfAxis()
{
  return this->AxisName->GetNumberOfValues();
}

vtkMedFamily* vtkMedMesh::GetOrCreatePointFamilyById(med_int id)
{
  for(int i = 0; i < this->GetNumberOfPointFamily(); i++)
    {
    vtkMedFamily* family = this->GetPointFamily(i);

    if(family->GetId() == id)
      return family;
    }
  vtkMedFamily* family = vtkMedFamily::New();
  family->SetId(id);
  std::ostringstream sstr;
  sstr << "UNDEFINED_POINT_FAMILY_" << id;
  family->SetName(sstr.str().c_str());
  family->SetPointOrCell(vtkMedUtilities::OnPoint);
  this->AppendPointFamily(family);
  family->Delete();
  return family;
}

void  vtkMedMesh::AddGridStep(vtkMedGrid* grid)
{
  this->GridStep->AddObject(grid->GetComputeStep(), grid);
}

void  vtkMedMesh::ClearGridStep()
{
  this->GridStep->clear();
}

vtkMedGrid* vtkMedMesh::GetGridStep(const vtkMedComputeStep& cs)
{
  return this->GridStep->GetObject(cs);
}

vtkMedGrid* vtkMedMesh::FindGridStep(const vtkMedComputeStep& cs,
                                     int strategy)
{
  return this->GridStep->FindObject(cs, strategy);
}

void  vtkMedMesh::GatherGridTimes(std::set<med_float>& timeset)
{
  this->GridStep->GatherTimes(timeset);
}

void  vtkMedMesh::GatherGridIterations(med_float time,
                                       std::set<med_int>& iterationset)
{
  this->GridStep->GatherIterations(time, iterationset);
}

void  vtkMedMesh::ClearMedSupports()
{
  med_int stepnb = this->GridStep->GetNumberOfObject();
  for(med_int stepid = 0; stepid<stepnb; stepid++ )
    {
    vtkMedGrid* grid = this->GridStep->GetObject(stepid);
    grid->ClearMedSupports();
    }
}

med_int vtkMedMesh::GetNumberOfGridStep()
{
  return this->GridStep->GetNumberOfObject();
}

vtkMedGrid* vtkMedMesh::GetGridStep(med_int id)
{
  return this->GridStep->GetObject(id);
}

void  vtkMedMesh::GatherMedEntities(std::set<vtkMedEntity>& entities)
{
  vtkMedGrid* firstStep = this->GetGridStep(0);
  if(firstStep == NULL)
    return;
  
  firstStep->GatherMedEntities(entities);
}

void vtkMedMesh::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  PRINT_IVAR(os, indent, MedIterator);
  PRINT_OBJECT_VECTOR(os, indent, CellFamily);
  PRINT_OBJECT_VECTOR(os, indent, PointFamily);
  PRINT_OBJECT_VECTOR(os, indent, PointGroup);
  PRINT_OBJECT_VECTOR(os, indent, CellGroup);

}
