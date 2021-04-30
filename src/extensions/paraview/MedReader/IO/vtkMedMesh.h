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

#ifndef __vtkMedMesh_h_
#define __vtkMedMesh_h_

#include "vtkObject.h"
#include "vtkMedSetGet.h"
#include "vtkMed.h"
#include "vtkMedUtilities.h"

class vtkMedGrid;
class vtkMedFamily;
class vtkMedGroup;
class vtkMedString;
class vtkMedIntArray;
class vtkMedComputeStep;
class vtkMedFile;
class vtkStringArray;

#include <set>

class VTK_EXPORT vtkMedMesh: public vtkObject
{
public:
  static vtkMedMesh* New();
  vtkTypeMacro(vtkMedMesh, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The name of the mesh in the med file.
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // The universal name of the mesh.
  vtkGetStringMacro(UniversalName);
  vtkSetStringMacro(UniversalName);

  // Description:
  // The name of the mesh in the med file.
  vtkGetStringMacro(Description);
  vtkSetStringMacro(Description);

  // Description:
  // The unit of the time steps.
  vtkGetStringMacro(TimeUnit);
  vtkSetStringMacro(TimeUnit);

  // Description:
  // The dimension of the space this mesh lives in
  vtkSetMacro(SpaceDimension, med_int);
  vtkGetMacro(SpaceDimension, med_int);

  // Description:
  // The dimension of this mesh
  vtkSetMacro(MeshDimension, med_int);
  vtkGetMacro(MeshDimension, med_int);

  // Description:
  // The type of grid used by this mesh
  vtkSetMacro(MeshType, med_mesh_type);
  vtkGetMacro(MeshType, med_mesh_type);

  // Description:
  // This stores how the compute steps should be iterated over
  // either first by time or first by iteration
  vtkSetMacro(SortingType, med_sorting_type);
  vtkGetMacro(SortingType, med_sorting_type);

  // Description:
  // This is the type of the axis
  // MED_CARTESIAN, MED_CYLINDRICAL, MED_SPHERICAL or MED_UNDEF_AXIS_TYPE
  vtkSetMacro(AxisType, med_axis_type);
  vtkGetMacro(AxisType, med_axis_type);

  // Description:
  // This is the type of the axis
  // MED_CARTESIAN, MED_CYLINDRICAL, MED_SPHERICAL or MED_UNDEF_AXIS_TYPE
  vtkSetMacro(StructuredGridType, med_grid_type);
  vtkGetMacro(StructuredGridType, med_grid_type);

  // Description:
  // a mesh can be modified over time, so this stores the
  // different compute steps of a given mesh
  void  AddGridStep(vtkMedGrid*);
  void  ClearGridStep();
  vtkMedGrid* GetGridStep(const vtkMedComputeStep&);
  vtkMedGrid* FindGridStep(const vtkMedComputeStep&, int);
  med_int GetNumberOfGridStep();
  vtkMedGrid* GetGridStep(med_int);
  void  GatherGridTimes(std::set<med_float>&);
  void  GatherGridIterations(med_float,std::set<med_int>&);

  // Description:
  // Container of the cell families in this mesh
  vtkGetObjectVectorMacro(CellFamily, vtkMedFamily);
  vtkSetObjectVectorMacro(CellFamily, vtkMedFamily);
  virtual vtkMedFamily* GetOrCreateCellFamilyById(med_int);

  // Description:
  // Get the Point Families
  vtkGetObjectVectorMacro(PointFamily, vtkMedFamily);
  vtkSetObjectVectorMacro(PointFamily, vtkMedFamily);
  virtual vtkMedFamily* GetOrCreatePointFamilyById(med_int);

  int GetNumberOfFamily();
  vtkMedFamily* GetFamily(int);

  // Description:
  // Container of the groups in this mesh
  vtkGetObjectVectorMacro(PointGroup, vtkMedGroup);
  vtkSetObjectVectorMacro(PointGroup, vtkMedGroup);
  vtkGetObjectVectorMacro(CellGroup, vtkMedGroup);
  vtkSetObjectVectorMacro(CellGroup, vtkMedGroup);
  virtual vtkMedGroup*  GetOrCreateGroup(int pointOrCell, const char*);

  // Description:
  // this id is the id to use when reading the med file
  vtkSetMacro(MedIterator, med_int);
  vtkGetMacro(MedIterator, med_int);

  // Description:
  // This stores the name of each axis
  vtkGetObjectMacro(AxisName, vtkStringArray);

  // Description:
  // This stores the unit of each axis
  vtkGetObjectMacro(AxisUnit, vtkStringArray);

  // Description:
  // This sets the number of axis of this mesh, and also allocates
  // the arrays to store the name and unit of each axis.
  void  SetNumberOfAxis(int);
  int GetNumberOfAxis();

  // Description:
  // free the memory used by this mesh (coordinates, connectivity)
  virtual void  ClearMedSupports();

  // Description:
  // The name of the mesh in the med file.
  virtual void  SetParentFile(vtkMedFile*);
  vtkGetObjectMacro(ParentFile, vtkMedFile);

  // Description:
  // This helper method fills the std::set with all vtkMedEntity types used in the first grid step
  // MED specifies that all types must be present in the first step for a varying mesh to be valid
  virtual void  GatherMedEntities(std::set<vtkMedEntity>& entities);

protected:
  vtkMedMesh();
  virtual ~vtkMedMesh();

  char* Name;
  char* UniversalName;
  char* Description;
  char* TimeUnit;
  med_int MedIterator;
  med_int SpaceDimension;
  med_int MeshDimension;
  med_mesh_type MeshType;
  med_sorting_type SortingType;
  med_axis_type AxisType;
  med_grid_type StructuredGridType;

  vtkMedFile* ParentFile;

  vtkObjectVector<vtkMedFamily>* CellFamily;
  vtkObjectVector<vtkMedFamily>* PointFamily;

  vtkObjectVector<vtkMedGroup>* PointGroup;
  vtkObjectVector<vtkMedGroup>* CellGroup;

  vtkStringArray* AxisName;
  vtkStringArray* AxisUnit;

  vtkMedComputeStepMap<vtkMedGrid>* GridStep;

  bool IsSupportMesh;

private:
  vtkMedMesh(const vtkMedMesh&); // Not implemented.
  void operator=(const vtkMedMesh&); // Not implemented.

};

#endif //__vtkMedMesh_h_
