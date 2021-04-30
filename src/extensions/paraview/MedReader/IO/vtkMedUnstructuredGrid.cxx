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

#include "vtkMedUnstructuredGrid.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkDataArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkCellType.h"
#include "vtkInformation.h"

#include "vtkMedUtilities.h"
#include "vtkMedEntityArray.h"
#include "vtkMedMesh.h"
#include "vtkMedFile.h"
#include "vtkMedDriver.h"
#include "vtkMedFamilyOnEntityOnProfile.h"
#include "vtkMedFamilyOnEntity.h"
#include "vtkMedProfile.h"
#include "vtkMedUtilities.h"
#include "vtkMedStructElement.h"
#include "vtkMedVariableAttribute.h"

#include "vtkMultiProcessController.h"

vtkCxxSetObjectMacro(vtkMedUnstructuredGrid,Coordinates,vtkDataArray);

vtkStandardNewMacro(vtkMedUnstructuredGrid)

vtkMedUnstructuredGrid::vtkMedUnstructuredGrid()
{
  this->Coordinates = NULL;
  this->NumberOfPoints = 0;
}

vtkMedUnstructuredGrid::~vtkMedUnstructuredGrid()
{
  this->SetCoordinates(NULL);
}

int vtkMedUnstructuredGrid::IsCoordinatesLoaded()
{
  return this->Coordinates != NULL && this->Coordinates->GetNumberOfTuples()
     == this->NumberOfPoints;
}

void  vtkMedUnstructuredGrid::InitializeCellGlobalIds()
{
  vtkIdType ncells = 0;
  
  for(int id = 0; id < this->EntityArray->size(); id++)
    {
    vtkMedEntityArray* array = this->EntityArray->at(id);
    
    if(array == NULL)
      continue;
    
    if(array->GetEntity().EntityType == MED_NODE)
      continue;
    
    array->SetInitialGlobalId(ncells + 1);
    ncells += array->GetNumberOfEntity();
    }
}

void  vtkMedUnstructuredGrid::ClearMedSupports()
{
  this->Superclass::ClearMedSupports();
  for(int id = 0; id < this->EntityArray->size(); id++)
    {
    vtkMedEntityArray* array = this->EntityArray->at(id);
    array->Initialize();
    }
}

void  vtkMedUnstructuredGrid::LoadCoordinates()
{
  this->GetParentMesh()->GetParentFile()->GetMedDriver()->LoadCoordinates(this);
}

double* vtkMedUnstructuredGrid::GetCoordTuple(med_int index)
{
  return this->Coordinates->GetTuple(index);
}

vtkDataSet* vtkMedUnstructuredGrid::CreateVTKDataSet(
    vtkMedFamilyOnEntityOnProfile* foep)
{
  vtkMultiProcessController* controller =
      vtkMultiProcessController::GetGlobalController();

  vtkMedFamilyOnEntity* foe = foep->GetFamilyOnEntity();
  vtkMedEntityArray* array = foe->GetEntityArray();
  if(foep->GetValid() == 0)
    {
    return NULL;
    }

  vtkUnstructuredGrid* vtkugrid = vtkUnstructuredGrid::New();

  // now we copy all the flagged nodes in the grid, shallow copy if possible
  bool shallowCopyPoints=true;

  if(this->GetParentMesh()->GetSpaceDimension()!=3)
    {
    shallowCopyPoints=false;
    }

  shallowCopyPoints = shallowCopyPoints && foep->CanShallowCopyPointField(NULL);

  foe->GetParentGrid()->LoadCoordinates();

  vtkIdType numberOfPoints;
  vtkPoints* points=vtkPoints::New(this->GetCoordinates()->GetDataType());
  vtkugrid->SetPoints(points);
  points->Delete();

  vtkIdTypeArray* pointGlobalIds=vtkIdTypeArray::New();
  pointGlobalIds->SetName("MED_POINT_ID");
  pointGlobalIds->SetNumberOfComponents(1);
  vtkugrid->GetPointData()->SetGlobalIds(pointGlobalIds);
  pointGlobalIds->Delete();

  vtkIdTypeArray* cellGlobalIds=vtkIdTypeArray::New();
  cellGlobalIds->SetName("MED_CELL_ID");
  cellGlobalIds->SetNumberOfComponents(1);
  vtkugrid->GetCellData()->SetGlobalIds(cellGlobalIds);
  cellGlobalIds->Delete();

  if (shallowCopyPoints)
    {
    vtkugrid->GetPoints()->SetDataType(this->GetCoordinates()->GetDataType());
    vtkugrid->GetPoints()->SetData(this->GetCoordinates());
    // add global ids
    numberOfPoints=this->GetNumberOfPoints();
    pointGlobalIds->SetNumberOfTuples(numberOfPoints);
    vtkIdType* ptr=pointGlobalIds->GetPointer(0);
    for(int pid=0; pid<numberOfPoints; pid++)
      ptr[pid]=pid+1;
    }
  else
    {
    vtkIdType currentIndex=0;
    for(vtkIdType index=0; index<this->GetNumberOfPoints(); index++)
      {
      if(!foep->KeepPoint(index))
        continue;

      double coord[3]={0.0, 0.0, 0.0};
      double * tuple=this->GetCoordinates()->GetTuple(index);
      for(int dim=0; dim<this->GetParentMesh()->GetSpaceDimension()&&dim<3; dim++)
        {
        coord[dim]=tuple[dim];
        }
      vtkugrid->GetPoints()->InsertPoint(currentIndex, coord);
      pointGlobalIds->InsertNextValue(index+1);
      currentIndex++;
      }
    vtkugrid->GetPoints()->Squeeze();
    pointGlobalIds->Squeeze();
    numberOfPoints=currentIndex;
    }

  vtkMedStructElement* structelem = NULL;
  int vtkType = VTK_EMPTY_CELL;
  int nsupportcell = 1;
  vtkSmartPointer<vtkIdTypeArray> supportIndex = vtkSmartPointer<vtkIdTypeArray>::New();
  supportIndex->SetName("STRUCT_ELEMENT_INDEX");
  supportIndex->SetNumberOfComponents(2);
  supportIndex->GetInformation()->Set(vtkMedUtilities::STRUCT_ELEMENT_INDEX(), 1);
  supportIndex->GetInformation()->Set(vtkAbstractArray::GUI_HIDE(), 1);
  supportIndex->SetComponentName(0, "GLOBAL_ID");
  supportIndex->SetComponentName(1, "LOCAL_ID");

  if(array->GetEntity().EntityType == MED_STRUCT_ELEMENT)
    {
    structelem = array->GetStructElement();
    if(structelem == NULL)
      {
      foep->SetValid(0);
      }
    else
      {
      vtkType = vtkMedUtilities::GetVTKCellType(
          structelem->GetSupportGeometryType());
      nsupportcell = structelem->GetSupportNumberOfCell();
      if(structelem->GetSupportEntityType() != MED_CELL
         || strcmp(structelem->GetName(), MED_PARTICLE_NAME) == 0)
        {
        // Special case : the support connectivity is implicit
        // (for particles)
        // map this to points
        vtkType = VTK_VERTEX;
        nsupportcell = structelem->GetSupportNumberOfNode();
        }

      vtkugrid->GetInformation()->Set(vtkMedUtilities::STRUCT_ELEMENT(), structelem);
      for(int varattid = 0; varattid<structelem->GetNumberOfVariableAttribute(); varattid++)
        {
        vtkMedVariableAttribute* varatt = structelem->GetVariableAttribute(varattid);
        varatt->Load(array);
        vtkAbstractArray* values = array->GetVariableAttributeValue(varatt);
        vtkugrid->GetFieldData()->AddArray(values);
        }
      vtkugrid->GetCellData()->AddArray(supportIndex);
      }
    }
  else
    {
    vtkType = vtkMedUtilities::GetVTKCellType(
        array->GetEntity().GeometryType);
    }

  vtkSmartPointer<vtkIdList> pts = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkIdList> vtkpts = vtkSmartPointer<vtkIdList>::New();
  vtkIdType intialGlobalId = array->GetInitialGlobalId();
  vtkMedIntArray* pids = (foep->GetProfile()!=NULL?
                           foep->GetProfile()->GetIds() : NULL);
  vtkIdType maxId = (pids!=NULL?
                     pids->GetNumberOfTuples():
                     array->GetNumberOfEntity());

  int valid = foep->GetValid();
  if (controller != NULL)
    if (controller->GetNumberOfProcesses() > 1)
    valid = 1;

  array->LoadConnectivity();
  for (vtkIdType pindex = 0; pindex<maxId && valid; pindex++)
    {
    vtkIdType realIndex = (pids!=NULL?
                           pids->GetValue(pindex)-1:
                           pindex);

    if (!foep->KeepCell(realIndex))
      continue;

    for(int sid = 0; sid < nsupportcell; sid++)
      {
      cellGlobalIds->InsertNextValue(intialGlobalId+pindex);
      }

    // The supportIndex array has 2 component, the first to give the index
    // of the med cell in the variable attributes array, the second to give
    // the index of the cell in the n cells composing the structural element
    if(array->GetEntity().EntityType == MED_STRUCT_ELEMENT)
      {
      for(int sid = 0; sid < nsupportcell; sid++)
        {
        supportIndex->InsertNextTuple2(pindex, sid);
        }
      }

    if (array->GetEntity().GeometryType==MED_POLYHEDRON)
      {
      if(vtkMedUtilities::FormatPolyhedronForVTK(foep, pindex, pts))
        {
        vtkugrid->InsertNextCell(VTK_POLYHEDRON, pts);
        }
      else
        {
        foep->SetValid(0);
        vtkugrid->Delete();
        return NULL;
        }
      }
    else
      {
      array->GetCellVertices(realIndex, pts);
      vtkpts->Initialize();
      vtkpts->SetNumberOfIds(pts->GetNumberOfIds());

      for(vtkIdType node=0; node<pts->GetNumberOfIds(); node++)
        {
        vtkIdType pid = pts->GetId(node);
        vtkIdType ptid=foep->GetVTKPointIndex(pid);
        if(ptid < 0 || ptid >= vtkugrid->GetNumberOfPoints())
          {
          vtkDebugMacro("Index error, this cell"  <<
                        " is not on this profile");
#ifndef MedReader_HAVE_PARALLEL_INFRASTRUCTURE
          vtkugrid->Delete();
          foep->SetValid(0);
          return NULL;
#else
          return vtkugrid;
#endif
          }
        vtkpts->SetId(vtkMedUtilities::MedToVTKIndex(vtkType, node), ptid);
        }

      // for strutural elements, insert nsupportcell instead of only one
      if(nsupportcell > 1)
        {
        vtkIdType npts = vtkpts->GetNumberOfIds() / nsupportcell;
        for(int sid=0; sid < nsupportcell; sid++)
          {
          vtkIdType* ptids = vtkpts->GetPointer(sid * npts);
          vtkugrid->InsertNextCell(vtkType, npts, ptids);
          }
        }
      else
        {
        vtkugrid->InsertNextCell(vtkType, vtkpts);
        }
      }
    }

  if(vtkugrid->GetNumberOfCells() == vtkugrid->GetNumberOfPoints())
    {
    for(int fieldId = 0; fieldId < vtkugrid->GetFieldData()->GetNumberOfArrays(); fieldId++)
      {
      vtkDataArray* fieldData = vtkugrid->GetFieldData()->GetArray(fieldId);
    
      if(fieldData->GetNumberOfTuples() == vtkugrid->GetNumberOfCells())
        {
        vtkugrid->GetPointData()->AddArray(fieldData);
        }
      else
        {
        vtkDataArray* real_fieldData = fieldData->NewInstance();
        real_fieldData->SetName(fieldData->GetName());
        real_fieldData->SetNumberOfComponents(fieldData->GetNumberOfComponents());

        for(vtkIdType cellId = 0; cellId < vtkugrid->GetNumberOfPoints(); cellId++)
          {
          vtkIdType supportId = static_cast<int>(supportIndex->GetTuple2(cellId)[0]);
          real_fieldData->InsertNextTuple(fieldData->GetTuple(supportId));
          }

        vtkugrid->GetPointData()->AddArray(real_fieldData);
        real_fieldData->Delete();
        }
      }
    }

  return vtkugrid;
}

void vtkMedUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  PRINT_IVAR(os, indent, NumberOfPoints);
}
