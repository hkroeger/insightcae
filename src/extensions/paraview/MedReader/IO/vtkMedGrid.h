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

#ifndef __vtkMedGrid_h_
#define __vtkMedGrid_h_

#include "vtkObject.h"
#include "vtkMedSetGet.h"
#include "vtkMedUtilities.h"
#include "vtkMed.h"
#include "vtkMedIntArray.h"

class vtkMedString;
class vtkMedFamilyOnEntity;
class vtkMedIntArray;

class VTK_EXPORT vtkMedGrid : public vtkObject
{
public :
  vtkTypeMacro(vtkMedGrid, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This identifies the time and iteration of this grid
  void  SetComputeStep(vtkMedComputeStep cs)
    {
    this->ComputeStep = cs;
    }
  vtkMedComputeStep  GetComputeStep()
    {
    return this->ComputeStep;
    }

  // Description:
  // returns the number of points. Each sub class has to implement this method.
  virtual med_int GetNumberOfPoints() = 0;

  // Description:
  // Initialize the global Ids of the first element of each MedEntityArray
  virtual void  InitializeCellGlobalIds(){;}

  // Description:
  // this stores the array giving the family id for each point of this mesh.
  //vtkGetObjectVectorMacro(PointFamilyData, vtkMedFamilyOnEntity);
  //vtkSetObjectVectorMacro(PointFamilyData, vtkMedFamilyOnEntity);
  //virtual vtkMedFamilyOnEntity* GetPointFamilyDataById(med_int id);

  // Description:
  // Gather the families that are present on this mesh nodes
  //virtual void ComputePointFamilies();

  // Description:
  // this array contains the global ids of the points used by the grid.
  virtual void  SetPointGlobalIds(vtkMedIntArray*);
  vtkGetObjectMacro(PointGlobalIds, vtkMedIntArray);

  // Description:
  // this array contains the family ids of the points used by the grid.
  //virtual void  SetPointFamilyIds(vtkMedIntArray*);
  //vtkGetObjectMacro(PointFamilyIds, vtkMedIntArray);

  // Description:
  // The mesh that use this grid
  virtual void  SetParentMesh(vtkMedMesh*);
  vtkGetObjectMacro(ParentMesh, vtkMedMesh);

  // Description:
  // The mesh that use this grid
  virtual void  SetPreviousGrid(vtkMedGrid*);
  vtkGetObjectMacro(PreviousGrid, vtkMedGrid);

  // Description:
  // returns 1 if the global Ids array is set, and the
  //  number of tuples matches the number of points
  virtual int IsPointGlobalIdsLoaded();

  // Description:
  // clear the entity arrays storing the connectivity
  virtual void  ClearMedSupports();

  // Description:
  // This is the coordinate system the grid live in.
  vtkSetMacro(CoordinateSystem, med_axis_type);
  vtkGetMacro(CoordinateSystem, med_axis_type);

  // Description:
  // This flag is set during the information loading, and tells if the
  // coordinates of this grid at this step has changed from the previous step.
  // if not, you should request the coordinates array from the previous grid
  // instead of this one.
  vtkSetMacro(UsePreviousCoordinates, bool);
  vtkGetMacro(UsePreviousCoordinates, bool);

  // Description:
  // Add a cell array to this unstructured grid.
  // each cell array represents a different cell type.
  vtkGetObjectVectorMacro(EntityArray, vtkMedEntityArray);
  vtkSetObjectVectorMacro(EntityArray, vtkMedEntityArray);

  // Description:
  // load the family ids using the driver
  //void  LoadPointFamilyIds();

  virtual void  LoadCoordinates() = 0;
  virtual int  IsCoordinatesLoaded() = 0;

  // Description:
  // return the vtkMedEntityArray that match the Entity type, if any.
  virtual vtkMedEntityArray* GetEntityArray(const vtkMedEntity&);

  virtual double* GetCoordTuple(med_int index) = 0;

  // Description:
  // This will instanciate a new vtkDataSet object.
  // The caller is responsible for deleting it.
  virtual vtkDataSet* CreateVTKDataSet(vtkMedFamilyOnEntityOnProfile*) = 0;
  
  //  Description:
  // This utility method returns all vtkMedEntity types present in this grid
  virtual void  GatherMedEntities(std::set<vtkMedEntity>& entities);

protected:
  vtkMedGrid();
  virtual ~vtkMedGrid();

  vtkMedComputeStep ComputeStep;
  vtkMedIntArray* PointGlobalIds;
  //vtkMedIntArray* PointFamilyIds;

  vtkMedMesh* ParentMesh;

  vtkMedGrid* PreviousGrid;

  //BTX
  //vtkObjectVector<vtkMedFamilyOnEntity>* PointFamilyData;
  //ETX

  bool UsePreviousCoordinates;
  med_axis_type CoordinateSystem;
  //BTX
  vtkObjectVector<vtkMedEntityArray>* EntityArray;
  //ETX

private:
  vtkMedGrid(const vtkMedGrid&); // Not implemented.
  void operator=(const vtkMedGrid&); // Not implemented.

};

#endif //__vtkMedGrid_h_
