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

#include "vtkMedCurvilinearGrid.h"

#include "vtkMedMesh.h"
#include "vtkMedFile.h"
#include "vtkMedDriver.h"
#include "vtkMedFamilyOnEntityOnProfile.h"
#include "vtkMedFamilyOnEntity.h"
#include "vtkMedEntityArray.h"
#include "vtkMedProfile.h"
#include "vtkMedFamily.h"

#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkStructuredGrid.h"

vtkCxxSetObjectMacro(vtkMedCurvilinearGrid, Coordinates, vtkDataArray);

vtkStandardNewMacro(vtkMedCurvilinearGrid)

vtkMedCurvilinearGrid::vtkMedCurvilinearGrid()
{
  this->Coordinates = NULL;
  this->NumberOfPoints = 0;
}

vtkMedCurvilinearGrid::~vtkMedCurvilinearGrid()
{
  this->SetCoordinates(NULL);
}

void vtkMedCurvilinearGrid::SetDimension(int dim)
{
  this->AxisSize.resize(dim);
}

int vtkMedCurvilinearGrid::GetDimension()
{
  return this->AxisSize.size();
}

void  vtkMedCurvilinearGrid::SetAxisSize(int axis, med_int size)
{
  if(axis < 0)
    return;

  if(axis >= this->AxisSize.size())
    this->AxisSize.resize(axis+1);

  this->AxisSize[axis] = size;
}

med_int vtkMedCurvilinearGrid::GetAxisSize(int axis)
{
  if(axis < 0 || axis >= this->AxisSize.size())
    return 0;

  return this->AxisSize[axis];
}

void  vtkMedCurvilinearGrid::LoadCoordinates()
{
  vtkMedDriver* driver = this->GetParentMesh()->GetParentFile()->GetMedDriver();
  driver->LoadCoordinates(this);
}

int vtkMedCurvilinearGrid::IsCoordinatesLoaded()
{
  return this->Coordinates != NULL && this->Coordinates->GetNumberOfTuples()
     == this->NumberOfPoints;
}

double* vtkMedCurvilinearGrid::GetCoordTuple(med_int index)
{
  return this->Coordinates->GetTuple(index);
}

vtkDataSet* vtkMedCurvilinearGrid::CreateVTKDataSet(
    vtkMedFamilyOnEntityOnProfile* foep)
{
  vtkStructuredGrid* vtkgrid = vtkStructuredGrid::New();

  vtkPoints* points = vtkPoints::New();
  vtkgrid->SetPoints(points);
  points->Delete();

  vtkgrid->SetDimensions(this->GetAxisSize(0),
                         this->GetAxisSize(1),
                         this->GetAxisSize(2));

  this->LoadCoordinates();

  if(this->GetDimension() == 3)
    {
    vtkgrid->GetPoints()->SetData(this->GetCoordinates());
    }
  else
    {
    vtkDataArray* coords = vtkDataArray::SafeDownCast(
        vtkAbstractArray::CreateArray(this->GetCoordinates()->GetDataType()));
    coords->SetNumberOfComponents(3);
    coords->SetNumberOfTuples(this->GetNumberOfPoints());
    vtkgrid->GetPoints()->SetData(coords);
    coords->Delete();

    med_int npts = this->GetNumberOfPoints();
    double coord[3] = {0, 0, 0};
    for(med_int id=0; id<npts; id++)
      {
      double * tuple = this->Coordinates->GetTuple(id);
      for(int dim=0; dim<this->GetDimension(); dim++)
        {
        coord[dim] = tuple[dim];
        }
      coords->SetTuple(id, coord);
      }
    }

  if(foep->GetProfile() != NULL)
    {
    foep->GetProfile()->Load();
    vtkMedIntArray* pids = foep->GetProfile()->GetIds();
    med_int previd = -1;
    for(med_int pid=0; pid<pids->GetNumberOfTuples(); pid++)
      {
      med_int id = pids->GetValue(pid) - 1;
      for(med_int theid=previd+1; theid<id; theid++)
        {
        vtkgrid->BlankCell(theid);
        }

      previd = id;
      }
    }

  if(foep->GetFamilyOnEntity()->GetEntityArray()->GetNumberOfFamilyOnEntity() > 1)
    {
    med_int famid = foep->GetFamilyOnEntity()->GetFamily()->GetId();
    vtkMedEntityArray* ea = foep->GetFamilyOnEntity()->GetEntityArray();
    for(med_int id=0; id<vtkgrid->GetNumberOfCells(); id++)
      {
      if(ea->GetFamilyId(id) != famid)
        vtkgrid->BlankCell(id);
      }
    }

  return vtkgrid;
}

void vtkMedCurvilinearGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
