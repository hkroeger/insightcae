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

#include "vtkMedRegularGrid.h"

#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkStructuredGrid.h"

#include "vtkMedUtilities.h"
#include "vtkMedFile.h"
#include "vtkMedDriver.h"
#include "vtkMedMesh.h"
#include "vtkMedFamilyOnEntityOnProfile.h"
#include "vtkMedFamilyOnEntity.h"
#include "vtkMedEntityArray.h"
#include "vtkMedProfile.h"
#include "vtkMedFamily.h"

vtkCxxGetObjectVectorMacro(vtkMedRegularGrid, AxisCoordinate, vtkDataArray);
vtkCxxSetAbstractObjectVectorMacro(vtkMedRegularGrid, AxisCoordinate, vtkDataArray);

vtkStandardNewMacro(vtkMedRegularGrid)

vtkMedRegularGrid::vtkMedRegularGrid()
{
  this->AxisCoordinate = new vtkObjectVector<vtkDataArray>();
}

vtkMedRegularGrid::~vtkMedRegularGrid()
{
  delete this->AxisCoordinate;
}

void vtkMedRegularGrid::SetDimension(med_int dim)
{
  if(dim < 0)
    dim = 0;
  this->AxisSize.resize(dim);
  this->SetNumberOfAxisCoordinate(dim);
}

int vtkMedRegularGrid::GetDimension()
{
  return this->AxisSize.size();
}

void  vtkMedRegularGrid::SetAxisSize(int axis, med_int size)
{
  if(axis < 0)
    return;

  if(axis >= this->GetDimension())
    {
    this->SetDimension(axis+1);
    }

  this->AxisSize[axis] = size;
}

med_int vtkMedRegularGrid::GetAxisSize(int dim)
{
  if(dim < 0 || dim >= this->AxisSize.size())
    return 0;
  return this->AxisSize[dim];
}

med_int vtkMedRegularGrid::GetNumberOfPoints()
{
  med_int npts = 1;
  for(int dim = 0; dim < this->AxisSize.size(); dim++)
    {
    npts *= this->AxisSize[dim];
    }
  return npts;
}

void  vtkMedRegularGrid::LoadCoordinates()
{
  this->GetParentMesh()->GetParentFile()->GetMedDriver()->LoadCoordinates(this);
}

int vtkMedRegularGrid::IsCoordinatesLoaded()
{
  bool res =  this->GetDimension() == this->AxisSize.size() &&
      this->GetDimension() == this->AxisCoordinate->size();

  if(!res)
    return 0;

  med_int nloadedcoords = 1;
  for(int axis=0; axis < this->GetDimension(); axis++)
    {
    vtkDataArray* axiscoords = this->GetAxisCoordinate(axis);
    if(axiscoords != NULL)
      nloadedcoords *= axiscoords->GetNumberOfTuples();
    else
      nloadedcoords = 0;
    }
  return nloadedcoords == this->GetNumberOfPoints();
}

double* vtkMedRegularGrid::GetCoordTuple(med_int index)
{
  this->CoordTuple[0] = 0;
  this->CoordTuple[1] = 0;
  this->CoordTuple[2] = 0;
  med_int prevmod = 1;
  for(int axis=0; axis < this->GetDimension(); axis++)
    {
    med_int modulo = prevmod * this->AxisSize[axis];
    med_int axisindex = (index % modulo) / prevmod;
    prevmod = modulo;
    vtkDataArray* coords = this->GetAxisCoordinate(axis);
    this->CoordTuple[axis] = coords->
                             GetTuple1(axisindex);
    }

  return this->CoordTuple;
}

vtkDataSet* vtkMedRegularGrid::CreateVTKDataSet(vtkMedFamilyOnEntityOnProfile* foep)
{
  vtkStructuredGrid* vtkgrid = vtkStructuredGrid::New();

  vtkPoints* points = vtkPoints::New();
  vtkgrid->SetPoints(points);
  points->Delete();

  vtkIdType dims[3] = {this->GetAxisSize(0),
                    this->GetAxisSize(1),
                    this->GetAxisSize(2)};

  for(int dim=0; dim<3; dim++)
    dims[dim] = (dims[dim] >= 1 ? dims[dim] : 1);

  vtkgrid->SetDimensions(dims[0], dims[1], dims[2]);

  this->LoadCoordinates();

  if(this->GetAxisCoordinate(0) == NULL)
    {
    vtkgrid->Delete();
    return NULL;
    }

  vtkDataArray* coords = vtkMedUtilities::NewCoordArray();
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(this->GetNumberOfPoints());
  vtkgrid->GetPoints()->SetData(coords);
  coords->Delete();

  med_int npts;
  double coord[3] = {0, 0, 0};

  npts = this->GetNumberOfPoints();
  for(med_int id=0; id<npts; id++)
    {
    double * tuple = this->GetCoordTuple(id);
    for(int dim=0; dim<this->GetDimension(); dim++)
      {
      coord[dim] = tuple[dim];
      }
    coords->SetTuple(id, coord);
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

void vtkMedRegularGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
