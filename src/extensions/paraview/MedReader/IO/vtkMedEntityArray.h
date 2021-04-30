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

#ifndef __vtkMedEntityArray_h_
#define __vtkMedEntityArray_h_

#include "vtkObject.h"
#include "vtkMedSetGet.h"
#include "vtkMedUtilities.h"
#include "vtkMed.h"
#include "vtkMedFilter.h"

class vtkMedIntArray;
class vtkFamilyIdSet;
class vtkMedFamily;
class vtkMedFamilyOnEntity;
class vtkMedMesh;
class vtkMedGrid;
class vtkIdList;
class vtkMedStructElement;
class vtkMedVariableAttribute;

class VTK_EXPORT vtkMedEntityArray: public vtkObject
{
public:
  static vtkMedEntityArray* New();
  vtkTypeMacro(vtkMedEntityArray, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the number of cells of this entity type.
  vtkSetMacro(NumberOfEntity, vtkIdType);
  vtkGetMacro(NumberOfEntity, vtkIdType);

  // Description:
  // the support of the cells : one of
  void  SetEntity(const vtkMedEntity& entity){this->Entity = entity;}
  const vtkMedEntity& GetEntity(){return this->Entity;}

  // Description:
  // This connectivity type of this entity : one of
  // MED_NODAL, MED_DESCENDING, MED_NO_CMODE
  vtkSetMacro(Connectivity, med_connectivity_mode);
  vtkGetMacro(Connectivity, med_connectivity_mode);

  // Description:
  // This array stores the family ids of each entity.
  virtual void SetConnectivityArray(vtkMedIntArray*);
  vtkGetObjectMacro(ConnectivityArray, vtkMedIntArray);

  // Description:
  // This array stores the connectivity array for this entity.
  virtual void SetFamilyIds(vtkMedIntArray*);
  virtual med_int GetFamilyId(med_int id);

  // Description:
  // This array stores the global Ids of the entities.
  virtual void SetGlobalIds(vtkMedIntArray*);
  vtkGetObjectMacro(GlobalIds, vtkMedIntArray);

  // Description:
  // For polygons, this array stores the index of each edge described in
  // the connectivity array
  // For polyhedrons, this arrays stores the index of each face described
  // in the NodeIndex array
  virtual void SetFaceIndex(vtkMedIntArray*);
  vtkGetObjectMacro(FaceIndex, vtkMedIntArray);

  // Description:
  // For polyhedrons, this arrays can store either
  // the index of each node of each face described in the Index1 array
  // (node connectivity) or the type each face described in the Index1
  // array (hierarchical connectivity)
  virtual void SetNodeIndex(vtkMedIntArray*);
  vtkGetObjectMacro(NodeIndex, vtkMedIntArray);

  // Description:
  // Arrays of entities are partitionned over families.
  vtkSetObjectVectorMacro(FamilyOnEntity, vtkMedFamilyOnEntity);
  vtkGetObjectVectorMacro(FamilyOnEntity, vtkMedFamilyOnEntity);

  // Description:
  // For polyhedrons, this arrays can store either
  // the index of each node of each face described in the Index1 array
  // (node connectivity) or the type each face described in the Index1
  // array (hierarchical connectivity)
  virtual void  SetParentGrid(vtkMedGrid*);
  vtkGetObjectMacro(ParentGrid, vtkMedGrid);

  // Description:
  // Compute the list of families that are on this array
  virtual void  ComputeFamilies();

  // Description:
  // returns true if there are cells of the given family on this entity.
  virtual int HasFamily(vtkMedFamily*);

  // Description:
  // returns 1 if the connectivity array is set and matches the number of
  // connectivity elements.
  virtual int IsConnectivityLoaded();

  // Description:
  // returns 1 if the family ids is set and matches the number of entities.
  virtual int IsFamilyIdsLoaded();

  // Description:
  // returns 1 if the global ids is set and matches the number of entities.
  virtual int IsGlobalIdsLoaded();

  // Description :
  // This gives the global id of the first element of this array.
  vtkSetMacro(InitialGlobalId, vtkIdType);
  vtkGetMacro(InitialGlobalId, vtkIdType);

  // Description :
  // resets all to default status, release memory
  virtual void Initialize();

  // Description:
  // Get the indices of the vertices used by a given cell.
  void  GetCellVertices(vtkIdType, vtkIdList*);

  virtual void  LoadConnectivity();

  // Descritpion:
  // This is a pointer to the StructElement object if any
  // This pointer is set during the LinkMedInfo pass
  virtual void  SetStructElement(vtkMedStructElement*);
  vtkGetObjectMacro(StructElement, vtkMedStructElement);

  void  SetVariableAttributeValues(vtkMedVariableAttribute*,
                                   vtkAbstractArray*);
  vtkAbstractArray* GetVariableAttributeValue(vtkMedVariableAttribute*);

  // Description:
  // Get/Set of the MED Filter for parallel reading.
  void  SetFilter(vtkMedFilter* filter){this->Filter = filter;}
  vtkMedFilter* GetFilter(){return this->Filter;}

protected:
  vtkMedEntityArray();
  virtual ~vtkMedEntityArray();

  vtkIdType NumberOfEntity;
  vtkMedEntity Entity;

  vtkMedFilter* Filter;

  med_connectivity_mode Connectivity;
  vtkIdType InitialGlobalId;

  vtkMedGrid* ParentGrid;

  vtkMedIntArray* FamilyIds;
  vtkMedIntArray* GlobalIds;
  vtkMedIntArray* ConnectivityArray;
  vtkMedIntArray* FaceIndex;
  vtkMedIntArray* NodeIndex;

  vtkMedStructElement* StructElement;

  int FamilyIdStatus;
  enum{
    FAMILY_ID_NOT_LOADED,
    FAMILY_ID_IMPLICIT,
    FAMILY_ID_EXPLICIT
  };

  //BTX
  vtkObjectVector<vtkMedFamilyOnEntity>* FamilyOnEntity;

  std::map<vtkMedVariableAttribute*, vtkSmartPointer<vtkAbstractArray> >
      VariableAttributeValue;
  //ETX

  int Valid;

private:
  vtkMedEntityArray(const vtkMedEntityArray&); // Not implemented.
  void operator=(const vtkMedEntityArray&); // Not implemented.
};

#endif //__vtkMedEntityArray_h_
