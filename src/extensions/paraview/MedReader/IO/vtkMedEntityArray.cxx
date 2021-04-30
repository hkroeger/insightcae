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

#include "vtkMedEntityArray.h"

#include "vtkObjectFactory.h"
#include "vtkMedIntArray.h"
#include "vtkMedUtilities.h"
#include "vtkMedFamily.h"
#include "vtkMedFamilyOnEntity.h"
#include "vtkMedMesh.h"
#include "vtkMedGrid.h"
#include "vtkMedUnstructuredGrid.h"
#include "vtkMedRegularGrid.h"
#include "vtkMedCurvilinearGrid.h"
#include "vtkMedFile.h"
#include "vtkMedDriver.h"
#include "vtkMedStructElement.h"

#include "vtkIdList.h"

#include <set>
using std::set;

vtkCxxSetObjectVectorMacro(vtkMedEntityArray, FamilyOnEntity, vtkMedFamilyOnEntity);
vtkCxxGetObjectVectorMacro(vtkMedEntityArray, FamilyOnEntity, vtkMedFamilyOnEntity);
vtkCxxSetObjectMacro(vtkMedEntityArray,FamilyIds,vtkMedIntArray);
vtkCxxSetObjectMacro(vtkMedEntityArray,GlobalIds,vtkMedIntArray);
vtkCxxSetObjectMacro(vtkMedEntityArray,ConnectivityArray,vtkMedIntArray);
vtkCxxSetObjectMacro(vtkMedEntityArray,FaceIndex,vtkMedIntArray);
vtkCxxSetObjectMacro(vtkMedEntityArray,NodeIndex,vtkMedIntArray);

vtkCxxSetObjectMacro(vtkMedEntityArray,ParentGrid,vtkMedGrid);
vtkCxxSetObjectMacro(vtkMedEntityArray,StructElement,vtkMedStructElement);

vtkStandardNewMacro(vtkMedEntityArray);

vtkMedEntityArray::vtkMedEntityArray()
{
  this->NumberOfEntity = 0;
  this->Connectivity = MED_NODAL;
  this->FamilyIds = NULL;
  this->GlobalIds = NULL;
  this->ConnectivityArray = NULL;
  this->FaceIndex = NULL;
  this->NodeIndex = NULL;
  this->InitialGlobalId = 0;
  this->FamilyOnEntity = new vtkObjectVector<vtkMedFamilyOnEntity> ();
  this->FamilyIdStatus = vtkMedEntityArray::FAMILY_ID_NOT_LOADED;
  this->ParentGrid = NULL;
  this->StructElement = NULL;
  //this->Filter = NULL;
}

vtkMedEntityArray::~vtkMedEntityArray()
{
  this->SetFamilyIds(NULL);
  this->SetGlobalIds(NULL);
  this->SetConnectivityArray(NULL);
  this->SetFaceIndex(NULL);
  this->SetNodeIndex(NULL);
  delete this->FamilyOnEntity;
  this->SetParentGrid(NULL);
  this->SetStructElement(NULL);
  //this->SetFilter(NULL);
}

void vtkMedEntityArray::Initialize()
{
  this->SetFamilyIds(NULL);
  this->SetGlobalIds(NULL);
  this->SetConnectivityArray(NULL);
  this->SetFaceIndex(NULL);
  this->SetNodeIndex(NULL);
  this->FamilyOnEntity->clear();
  this->FamilyIdStatus = FAMILY_ID_NOT_LOADED;
}

void vtkMedEntityArray::ComputeFamilies()
{
  this->FamilyOnEntity->clear();
  vtkMedMesh* mesh = this->ParentGrid->GetParentMesh();

  if(this->FamilyIds == NULL)
    {
    vtkMedFamilyOnEntity* foe = vtkMedFamilyOnEntity::New();
    foe->SetParentGrid(this->ParentGrid);
    this->AppendFamilyOnEntity(foe);
    foe->Delete();
    if(this->GetEntity().EntityType != MED_NODE)
      {
      foe->SetFamily(mesh->GetOrCreateCellFamilyById(0));
      }
    else
      {
      foe->SetFamily(mesh->GetOrCreatePointFamilyById(0));
      }
    foe->SetEntityArray(this);
    this->FamilyIdStatus = vtkMedEntityArray::FAMILY_ID_IMPLICIT;
    return;
    }

  this->FamilyIdStatus = vtkMedEntityArray::FAMILY_ID_EXPLICIT;

  set<med_int> idset;
  for (vtkIdType index = 0; index < this->FamilyIds->GetNumberOfTuples(); index++)
    {
    med_int id = this->FamilyIds->GetValue(index);
    idset.insert(id);
    }

  for (set<med_int>::iterator it = idset.begin(); it != idset.end(); it++)
    {
    vtkMedFamilyOnEntity* foe = vtkMedFamilyOnEntity::New();
    foe->SetParentGrid(this->ParentGrid);
    this->AppendFamilyOnEntity(foe);
    foe->Delete();
    if(this->GetEntity().EntityType != MED_NODE)
      {
      foe->SetFamily(mesh->GetOrCreateCellFamilyById(*it));
      }
    else
      {
      foe->SetFamily(mesh->GetOrCreatePointFamilyById(*it));
      }
    foe->SetEntityArray(this);
    }
}

med_int vtkMedEntityArray::GetFamilyId(med_int id)
{
  if(this->FamilyIdStatus == FAMILY_ID_IMPLICIT)
    return 0;
  if(this->FamilyIdStatus == FAMILY_ID_NOT_LOADED)
    {
    vtkErrorMacro("You have to load family ids before asking for it!");
    }
  return this->FamilyIds->GetValue(id);
}

int vtkMedEntityArray::HasFamily(vtkMedFamily* family)
{
  for (int i = 0; i < this->FamilyOnEntity->size(); i++)
    {
    vtkMedFamilyOnEntity* foe = this->FamilyOnEntity->at(i);
    if(foe->GetFamily() == family)
      return 1;
    }
  return 0;
}

int vtkMedEntityArray::IsConnectivityLoaded()
{
  // Entity Arrays representing something else than cells
  // have no connectivity

  if(vtkMedUnstructuredGrid::SafeDownCast(this->GetParentGrid()) == NULL)
    return 1;

  if( this->Entity.EntityType != MED_CELL &&
      this->Entity.EntityType != MED_DESCENDING_FACE &&
      this->Entity.EntityType != MED_DESCENDING_EDGE &&
      this->Entity.EntityType != MED_STRUCT_ELEMENT)
    return 1;

  if(this->ConnectivityArray == NULL)
    return 0;

  if(this->Connectivity == MED_NODAL && this->Entity.EntityType != MED_STRUCT_ELEMENT)
    {
    vtkIdType connSize = 0;
    if(this->Entity.GeometryType == MED_POLYHEDRON || this->Entity.GeometryType == MED_POLYGON)
      {
      if(!this->NodeIndex || !this->FaceIndex)
        return 0;
      for(med_int i  = 0; i < this->NumberOfEntity; i++ ) 
        {
        med_int start = this->FaceIndex->GetValue(i)-1;
        med_int end = this->FaceIndex->GetValue(i+1)-1;
        for(med_int fi = start ; fi < end; fi++ ) 
          {          
          med_int fstart = this->NodeIndex->GetValue(fi)-1;
          med_int fend = this->NodeIndex->GetValue(fi+1)-1;
          connSize += (fend-fstart);
          }
        }
      } else
        {
        connSize = this->NumberOfEntity * 
        vtkMedUtilities::GetNumberOfPoint(this->Entity.GeometryType);
        }
    return connSize == this->ConnectivityArray->GetNumberOfTuples();
    }
  else if (this->Connectivity == MED_NODAL && this->Entity.EntityType == MED_STRUCT_ELEMENT)
    {
    if(this->StructElement == NULL)
      return 1;

    vtkIdType connSize = this->NumberOfEntity
                         * this->StructElement->GetConnectivitySize();

    return connSize == this->ConnectivityArray->GetNumberOfTuples();
    }
  else
    {
    vtkIdType connSize = this->NumberOfEntity
        * vtkMedUtilities::GetNumberOfSubEntity(this->Entity.GeometryType);

    return connSize == this->ConnectivityArray->GetNumberOfTuples();
    }
}

int vtkMedEntityArray::IsFamilyIdsLoaded()
{
  return this->FamilyIdStatus != vtkMedEntityArray::FAMILY_ID_NOT_LOADED;;
}

int vtkMedEntityArray::IsGlobalIdsLoaded()
{
  return this->GlobalIds != NULL && this->GlobalIds->GetNumberOfTuples()
      == this->NumberOfEntity;
}

void vtkMedEntityArray::GetCellVertices(vtkIdType index, vtkIdList* ids)
{
  ids->Initialize();

  if(this->Entity.EntityType == MED_NODE)
    {
    ids->InsertNextId(index);
    return;
    }

  if( this->Entity.EntityType != MED_CELL &&
      this->Entity.EntityType != MED_DESCENDING_FACE &&
      this->Entity.EntityType != MED_DESCENDING_EDGE &&
      this->Entity.EntityType != MED_STRUCT_ELEMENT)
    {
    vtkErrorMacro("This reader is not compatible with those entities (yet)...");
    return;
    }

  if(vtkMedUnstructuredGrid::SafeDownCast(this->ParentGrid) == NULL)
    {
    // this is a structured grid, connectivity is implicit...

    if(this->Entity.GeometryType == MED_POINT1)
      {
      // degenerate case if there is only one point
      ids->InsertNextId(0);
      return;
      }
    if(this->Entity.GeometryType == MED_SEG2)
      {
      // line
      ids->InsertNextId(index);
      ids->InsertNextId(index+1);
      return;
      }
    vtkMedRegularGrid* vtkrgrid = vtkMedRegularGrid::SafeDownCast(
        this->GetParentGrid());
    vtkMedCurvilinearGrid* vtkcgrid = vtkMedCurvilinearGrid::SafeDownCast(
        this->GetParentGrid());
    vtkIdType xncell = 0;
    vtkIdType yncell = 0;
    vtkIdType zncell = 0;
    vtkIdType xnpts = 1;
    vtkIdType ynpts = 1;
    vtkIdType znpts = 1;
    if(vtkrgrid!=NULL)
      {
      xncell = vtkrgrid->GetAxisSize(0)-1;
      yncell = vtkrgrid->GetAxisSize(1)-1;
      zncell = vtkrgrid->GetAxisSize(2)-1;
      xnpts = vtkrgrid->GetAxisSize(0);
      ynpts = vtkrgrid->GetAxisSize(1);
      znpts = vtkrgrid->GetAxisSize(2);
      }
    if(vtkcgrid != NULL)
      {
      xncell = vtkcgrid->GetAxisSize(0)-1;
      yncell = vtkcgrid->GetAxisSize(1)-1;
      zncell = vtkcgrid->GetAxisSize(2)-1;
      xnpts = vtkcgrid->GetAxisSize(0);
      ynpts = vtkcgrid->GetAxisSize(1);
      znpts = vtkcgrid->GetAxisSize(2);
      }
    vtkIdType xindex = index % xncell;
    if(xncell <= 0)
      return;

    vtkIdType yindex = index / xncell;

    if(this->Entity.GeometryType == MED_QUAD4)
      {
      // plane

      ids->InsertNextId(xindex + yindex*xnpts);
      ids->InsertNextId(xindex + 1 + yindex*xnpts);
      ids->InsertNextId(xindex + yindex*xnpts);
      ids->InsertNextId(xindex + 1 + (yindex + 1)*xnpts);
      return;
      }

    if(yncell <= 0)
      return;

    vtkIdType zindex = index / (xncell*yncell);

    if(this->Entity.GeometryType == MED_HEXA8)
      {
      // volume
      ids->InsertNextId(xindex   + (yindex  )*xnpts + (zindex  )*xnpts*ynpts);
      ids->InsertNextId(xindex+1 + (yindex  )*xnpts + (zindex  )*xnpts*ynpts);
      ids->InsertNextId(xindex   + (yindex+1)*xnpts + (zindex  )*xnpts*ynpts);
      ids->InsertNextId(xindex+1 + (yindex+1)*xnpts + (zindex  )*xnpts*ynpts);
      ids->InsertNextId(xindex   + (yindex  )*xnpts + (zindex+1)*xnpts*ynpts);
      ids->InsertNextId(xindex+1 + (yindex  )*xnpts + (zindex+1)*xnpts*ynpts);
      ids->InsertNextId(xindex   + (yindex+1)*xnpts + (zindex+1)*xnpts*ynpts);
      ids->InsertNextId(xindex+1 + (yindex+1)*xnpts + (zindex+1)*xnpts*ynpts);
      return;
      }
    return;
    }

  this->LoadConnectivity();

  if (this->GetEntity().GeometryType==MED_POLYHEDRON)
    {
    vtkMedIntArray* conn = this->GetConnectivityArray();
    vtkMedIntArray* faceIndex = this->GetFaceIndex();
    vtkMedIntArray* nodeIndex = this->GetNodeIndex();
    med_int start = faceIndex->GetValue(index)-1;
    med_int end = faceIndex->GetValue(index+1)-1;
    // the use of a set loses the order, but VTK do not support this order anyway.
    if (this->GetConnectivity()==MED_NODAL)
      {
      for (int ff = start; ff<end; ff++)
        {
        med_int fstart = nodeIndex->GetValue(ff)-1;
        med_int fend = nodeIndex->GetValue(ff+1)-1;
        for (int pt = fstart; pt<fend; pt++)
          {
          med_int ptid = conn->GetValue(pt)-1;
          ids->InsertNextId(ptid);
          }
        }
      }
    else // MED_DESCENDING
      {
      vtkMedUnstructuredGrid* ugrid =
          vtkMedUnstructuredGrid::SafeDownCast(this->ParentGrid);
      if (!ugrid)
        {
        vtkErrorMacro(
        "MED_DESCENDING connectivity is only supported on unstructured grids");
        return;
        }
      set<med_int> pts;
      vtkIdList* subIds = vtkIdList::New();
      for (int ff = start; ff<end; ff++)
        {
        med_int fid = conn->GetValue(ff)-1;
        vtkMedEntity entity;
        entity.GeometryType = (med_geometry_type) NodeIndex->GetValue(ff);
        entity.EntityType = MED_DESCENDING_FACE;
        vtkMedEntityArray* subarray = ugrid->GetEntityArray(entity);
        subarray->GetCellVertices(fid, subIds);
        for (int id = 0; id<subIds->GetNumberOfIds(); id++)
          {
          med_int ptid = subIds->GetId(id);
          if(pts.find(ptid) == pts.end())
            {
            ids->InsertNextId(ptid);
            pts.insert(ptid);
            }
          }
        }
      subIds->Delete();
      }
    }//end polyhedron
  else if (this->GetEntity().GeometryType==MED_POLYGON)
    {
    vtkMedIntArray* conn = this->GetConnectivityArray();
    vtkMedIntArray* nids = this->GetFaceIndex();
    med_int start = nids->GetValue(index)-1;
    med_int end = nids->GetValue(index+1)-1;
    if (this->GetConnectivity()==MED_NODAL)
      {
      for (int pt = start; pt<end; pt++)
        {
        ids->InsertNextId(conn->GetValue(pt)-1);
        }
      }
    else // MED_DESCENDING
      {
      vtkIdList* subpts=vtkIdList::New();
      vtkMedUnstructuredGrid* ugrid =
          vtkMedUnstructuredGrid::SafeDownCast(this->ParentGrid);
      if (!ugrid)
        {
        vtkErrorMacro("MED_DESCENDING connectivity is only "
                      << "supported on unstructured grids");
        return;
        }
      set<med_int> pts;
      for (int sub = start; sub<end; sub++)
        {
        med_int subid = conn->GetValue(sub)-1;
        vtkMedEntity subentity;
        subentity.GeometryType = MED_SEG2;
        subentity.EntityType = MED_DESCENDING_EDGE;
        vtkMedEntityArray* subarray = ugrid->GetEntityArray(subentity);
        subarray->GetCellVertices(subid, subpts);
        for(int id=0; id<subpts->GetNumberOfIds(); id++)
          {
          med_int ptid = subpts->GetId(id);
          if(pts.find(ptid) != pts.end())
            {
            pts.insert(ptid);
            ids->InsertNextId(ptid);
            }
          }
        }
      subpts->Delete();
      }
    }//end poygon
  else if (this->GetConnectivity()==MED_NODAL ||
           vtkMedUtilities::GetDimension(this->GetEntity().GeometryType)<1)
    {
    int npts = 0;
    if(this->GetEntity().EntityType == MED_STRUCT_ELEMENT)
      {
      if(this->StructElement != NULL)
        {
        npts = this->StructElement->GetConnectivitySize();
        }
      }
    else
      {
      npts = vtkMedUtilities::GetNumberOfPoint(this->GetEntity().GeometryType);
      }
    vtkMedIntArray* conn = this->GetConnectivityArray();
    for (int i = 0; i<npts; i++)
      {
      vtkIdType ptid = conn->GetValue(npts*index+i)-1;
      ids->InsertNextId(ptid);
      }
    }//end nodal case
  else
    {
    vtkIdList* subpts=vtkIdList::New();
    int nsub=vtkMedUtilities::GetNumberOfSubEntity(
        this->GetEntity().GeometryType);
    vtkMedUnstructuredGrid* ugrid =
        vtkMedUnstructuredGrid::SafeDownCast(this->ParentGrid);
    if (!ugrid)
      {
      vtkErrorMacro(
        "MED_DESCENDING connectivity is only supported on unstructured grids");
      return;
      }
    vtkMedIntArray* conn=this->GetConnectivityArray();
    ids->SetNumberOfIds(vtkMedUtilities::GetNumberOfPoint(
        this->GetEntity().GeometryType));
    for (int sub = 0; sub<nsub; sub++)
      {
      med_int subid = conn->GetValue(nsub*index+sub)-1;
      bool invert = false;
      if(subid < 0)
        {
        subid = -subid;
        invert = true;
        }

      vtkMedEntity subentity;
      subentity.GeometryType = vtkMedUtilities::GetSubGeometry(
          this->GetEntity().GeometryType, sub);
      subentity.EntityType = vtkMedUtilities::GetSubType(
          this->GetEntity().EntityType);
      vtkMedEntityArray* subarray = ugrid->GetEntityArray(subentity);
      if(subarray == NULL)
        {
        subentity.EntityType = MED_CELL;
        subarray = ugrid->GetEntityArray(subentity);
        }
      if(subarray == NULL)
        {
        vtkDebugMacro( << "Missing sub entity array " << subentity.GeometryType);
        this->Valid = false;
        break;
        }
      subarray->GetCellVertices(subid, subpts);
      vtkMedUtilities::ProjectConnectivity(this->GetEntity().GeometryType, ids, subpts,
          sub, invert);
      }
    subpts->Delete();
    }
}

void  vtkMedEntityArray::LoadConnectivity()
{
  if(this->IsConnectivityLoaded())
    return;

  this->GetParentGrid()->GetParentMesh()->GetParentFile()->GetMedDriver()
      ->LoadConnectivity(this);
}

void  vtkMedEntityArray::SetVariableAttributeValues(
    vtkMedVariableAttribute* varatt, vtkAbstractArray* value)
{
  this->VariableAttributeValue[varatt] = value;
}

vtkAbstractArray* vtkMedEntityArray::GetVariableAttributeValue(
    vtkMedVariableAttribute* varatt)
{
  if(this->VariableAttributeValue.find(varatt)
    == this->VariableAttributeValue.end())
    return NULL;

  return this->VariableAttributeValue[varatt];
}

void vtkMedEntityArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  PRINT_IVAR(os, indent, NumberOfEntity)
  PRINT_IVAR(os, indent, Connectivity)
  PRINT_IVAR(os, indent, InitialGlobalId)
}
