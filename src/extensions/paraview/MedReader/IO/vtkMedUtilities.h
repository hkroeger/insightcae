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

#ifndef _vtkMedUtilities_h_
#define _vtkMedUtilities_h_

#include "vtkObject.h"
#include "vtkSmartPointer.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStringArray.h"
#include "vtkMedSetGet.h"
#include "vtkMed.h"
#include "vtkMedReader.h"

#include <utility>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

class vtkDataArray;
class vtkMedMesh;
class vtkMedFamily;
class vtkMedGroup;
class vtkIdList;
class vtkMutableDirectedGraph;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;

//BTX
class vtkMedComputeStep
{
public :
  med_int IterationIt;
  med_int TimeIt;
  med_float TimeOrFrequency;

  vtkMedComputeStep()
    {
    this->IterationIt = MED_NO_IT;
    this->TimeIt = MED_NO_DT;
    this->TimeOrFrequency = MED_UNDEF_DT;
    }
};

bool operator==(const vtkMedComputeStep& cs0, const vtkMedComputeStep& cs1);
bool operator!=(const vtkMedComputeStep& cs0, const vtkMedComputeStep& cs1);
bool operator<(const vtkMedComputeStep& cs0, const vtkMedComputeStep& cs1);

class vtkMedEntity
{
public :

  vtkMedEntity() : EntityType(MED_NODE),
                     GeometryType(MED_NONE)
    {
    }

  vtkMedEntity(med_entity_type type, med_geometry_type geometry) :
      EntityType(type),
      GeometryType(geometry)
    {
    }

  ~vtkMedEntity()
    {
    }

  vtkMedEntity(const vtkMedEntity& entity) :
      EntityType(entity.EntityType),
      GeometryType(entity.GeometryType)
    {
    this->GeometryName = entity.GeometryName;
    }

  void operator=(const vtkMedEntity& entity)
    {
    this->EntityType = entity.EntityType;
    this->GeometryType = entity.GeometryType;
    this->GeometryName = entity.GeometryName;
    }

  med_entity_type EntityType;
  med_geometry_type GeometryType;
  std::string GeometryName;
};

bool operator==(const vtkMedEntity& cs0, const vtkMedEntity& cs1);
bool operator!=(const vtkMedEntity& cs0, const vtkMedEntity& cs1);
bool operator<(const vtkMedEntity& cs0, const vtkMedEntity& cs1);
//ETX

class VTK_EXPORT vtkMedUtilities
{
public:
  static vtkInformationIntegerKey* ELNO();
  static vtkInformationIntegerKey* ELGA();
  static vtkInformationStringVectorKey* BLOCK_NAME();
  static vtkInformationObjectBaseKey* STRUCT_ELEMENT();
  static vtkInformationIntegerKey* STRUCT_ELEMENT_INDEX();

  // Description:
  // return an array to store the coordinates of nodes.
  // the  type of the elements is the same as the one in the med file
  static vtkDataArray* NewCoordArray();

  // Description:
  // returns an array to store data of a given type.
  // the type corresponds to med types.
  static vtkDataArray* NewArray(med_field_type type);
  static vtkAbstractArray* NewArray(med_attribute_type type);

  //BTX
  enum
  {
    OnPoint, OnCell
  };
  //ETX

  //BTX
  // Description:
  // returns a name for the given med_geometry_type
  static const char* GeometryName(med_geometry_type geometry);

  // Description:
  // returns a name for the given med_geometry_type
  static const char* EntityName(med_entity_type type);

  // Description:
  // returns a name for the given med_connectivity_mode
  static const char* ConnectivityName(med_connectivity_mode conn);

  static const std::string SimplifyName(const char* medName);

  static const std::string FamilyKey(const char* meshName, int pointOrCell,
      const char* familyName);
  static const std::string GroupKey(const char* meshName, int pointOrCell,
      const char* groupName);

  static const std::string EntityKey(const vtkMedEntity&);

  static int GetNumberOfPoint(med_geometry_type geometry);
  static int GetDimension(med_geometry_type geometry);

  // returns the VTK cell type (as described in the vtkCellType.h file)
  // corresponding to the given med_geometry_type
  static int GetVTKCellType(med_geometry_type geometry);

  // returns the number of sub entity : the number of faces for cells,
  // the number of edges for faces, the number of nodes for edges
  static int GetNumberOfSubEntity(med_geometry_type geometry);

  // returns the number of Nodes
  static int GetNumberOfNodes(med_geometry_type geometry);
  //ETX

  static med_entity_type GetSubType(med_entity_type type);
  static med_geometry_type GetSubGeometry(med_geometry_type geometry,
      int index);

  static int GetParentNodeIndex(med_geometry_type parentGeometry,
      int subEntityIndex, int subEntityNodeIndex);

  // Description :
  // Project the ids gathered in the sub entity to the parent entity.
  // used for MED_DESC connectivity.
  // Rem : no check is performed, and do not work for
  // MED_POLYHEDRE and MED_POLYGON
  static void ProjectConnectivity(med_geometry_type parentGeometry,
      vtkIdList* parentIds, vtkIdList* subEntityIds, int subEntityIndex,
      bool invert);

  static char Separator;

  static const char* NoGroupName;
  static const char* OnCellName;
  static const char* OnPointName;

  //BTX
  static void SplitGroupKey(const char* name, vtkStdString& mesh,
      vtkStdString& entity, vtkStdString& group);

  static std::string GetModeKey(int index, double frequency, int maxindex);
  static int  GetModeFromKey(const char*, int& index, double& frequency);

  static int MedToVTKIndex(int vtktype, int node);

  static vtkMultiBlockDataSet* GetParent(vtkMultiBlockDataSet* root,
                                  vtkStringArray* path);

  // Description:
  // Format the id list so that it respects the VTK format for polyhedrons :
  // numfaces, npts_face0, pt0, ... npts_face1, pt1 ....
  static int  FormatPolyhedronForVTK(vtkMedFamilyOnEntityOnProfile*,
                               vtkIdType, vtkIdList*);

  static int SizeOf(med_attribute_type type);
  //ETX
};

//BTX

template<class T>
class vtkObjectVector: public std::vector<vtkSmartPointer<T> >
{
};

template<class T>
class vtkMedComputeStepMap: public
    std::map<med_int, std::map<med_int, vtkSmartPointer<T> > >
{
public :
  void  AddObject(const vtkMedComputeStep& cs, T* obj)
    {
    (*this)[cs.TimeIt][cs.IterationIt] = obj;
    this->TimeIt[cs.TimeOrFrequency] = cs.TimeIt;
    }

  T* GetObject(const vtkMedComputeStep& cs)
    {
    if(this->find(cs.TimeIt) == this->end())
      return NULL;

    std::map<med_int, vtkSmartPointer<T> >& itmap = (*this)[cs.TimeIt];

    if(itmap.find(cs.IterationIt) == itmap.end())
      return NULL;

    return itmap[cs.IterationIt];
    }

  med_int GetNumberOfObject()
    {
    med_int nb = 0;
    typename vtkMedComputeStepMap<T>::iterator it = this->begin();
    while(it != this->end())
      {
      nb += it->second.size();
      it++;
      }
    return nb;
    }

  T* GetObject(med_int id)
    {
    med_int nb = 0;
    if(id < 0)
      return NULL;

    typename vtkMedComputeStepMap<T>::iterator it = this->begin();
    while(it != this->end())
      {
      std::map<med_int, vtkSmartPointer<T> >& itmap = it->second;
      nb += itmap.size();
      if(id < nb)
        {
        typename std::map<med_int, vtkSmartPointer<T> >::iterator iterationit =
            itmap.begin();
        for(int ii=0; ii<nb-id-1; ii++)
          iterationit++;
        return iterationit->second;
        }
      it++;
      }
    return NULL;
    }

  T* FindObject(const vtkMedComputeStep& cs, int strategy)
    {
    // first test if the given compute step is present
    T* obj = this->GetObject(cs);
    if(obj != NULL)
      return obj;

    if(this->size() == 0)
      return NULL;

    // let us first find the iterator that corresponds to the given time
    med_int timeit = this->FindTimeIterator(cs.TimeOrFrequency, cs.TimeIt);

    std::map<med_int, vtkSmartPointer<T> >& itmap =
        (*this)[timeit];

    if(itmap.size() == 0)
      return NULL;

    if(strategy == vtkMedReader::PhysicalTime
       || strategy == vtkMedReader::Modes)
      {
      // in this strategies, we return the last iteration for each time.
      return itmap.rbegin()->second;
      }
    else if(strategy == vtkMedReader::Iteration)
      {
      // in this case, we look for the real iteration
      typename std::map<med_int, vtkSmartPointer<T> >::iterator iterationit
          = itmap.lower_bound(cs.IterationIt);

      // if this is not exactly the same step and if this is not the first
      // step, rool back one step to choose the one just before the asked time.
      if(iterationit->first != cs.IterationIt && iterationit != itmap.begin())
        iterationit--;

      // the time iterator asked for is higher than all times,
      // let us pick the last one.
      if(iterationit == itmap.end())
        iterationit--;

      return iterationit->second;
      }
     return NULL;
    }

  void  GatherTimes(std::set<med_float>& times)
    {
    typename std::map<med_float, med_int>::iterator it
        = this->TimeIt.begin();
    while(it != this->TimeIt.end())
      {
      times.insert(it->first);
      it++;
      }
    }

  void  GatherIterations(med_float time, std::set<med_int>& iterations)
    {
    med_int timeit = this->FindTimeIterator(time, -1);
    if(timeit == -1)
      return;

    std::map<med_int, vtkSmartPointer<T> >& itmap =
        (*this)[timeit];

    typename std::map<med_int, vtkSmartPointer<T> >::iterator it =
        itmap.begin();

    while(it != itmap.end())
      {
      iterations.insert(it->first);
      it++;
      }
    }

protected :

  med_int FindTimeIterator(med_float time, med_int defaultit)
    {
    if(this->TimeIt.size() == 0)
      return defaultit;

    typename std::map<med_float, med_int>::iterator it
        = this->TimeIt.lower_bound(time);

    // if this is not exactly the same step and if this is not the first step,
    // rool back one step to choose the one just before the asked time.
    if(it->first != time && it != this->TimeIt.begin())
      it--;

    // if the time iterator asked for is higher than all times,
    // let us pick the last one.
    if(it == this->TimeIt.end())
      it--;

    return it->second;
    }

  std::map<med_float, med_int> TimeIt;
};

template<class T>
class vtkList: public std::list<T>
{
};

template<class T1, class T2>
struct IsSameTraits
{
  static const bool IsSame()
  {
    return false;
  }
};

template<class T1>
struct IsSameTraits<T1, T1>
{
  static const bool IsSame()
  {
    return true;
  }
};

#define PRINT_IVAR(os, indent, name) \
  os << indent << #name << " : "  << name << endl;

#define PRINT_STRING(os, indent, name) \
  os << indent << #name << " : "  << ( name ? name : "(void)") << endl;

#define PRINT_OBJECT(os, indent, name) \
  os << indent << #name << " : " ;\
  if(name != NULL) \
  {\
    os << endl;\
    name->PrintSelf(os, indent.GetNextIndent());\
  }\
  else os << "(NULL)" << endl;

#define PRINT_VECTOR(os, indent, name, size) \
{\
  os << indent << #name << " : (";\
  for(vtkIdType _index = 0; _index<size; _index++)\
    {\
    os << name[_index];\
    if(_index < size-1)\
      os << ", ";\
    }\
  os << ")" << endl;\
}

#define PRINT_OBJECT_VECTOR(os, indent, name) \
{\
  os << indent << #name;\
  os << endl;\
  vtkIdType _size = name->size();\
  for(vtkIdType _index = 0; _index < _size; _index++)\
  {\
  os << indent << #name << _index << " : " << endl;\
  if(name->at(_index) != NULL)\
    name->at(_index)->PrintSelf(os, indent.GetNextIndent());\
  else\
    os << indent.GetNextIndent() << "(NULL)" << endl;\
  }\
}

#define PRINT_STRING_VECTOR(os, indent, name)\
{\
  os << indent << #name << ": ";\
  for(int _comp = 0; _comp<this->name->size(); _comp++)\
    {\
    os << this->name->at(_comp)->GetString();\
    if(_comp < this->name->size()-1)\
      os << ", ";\
    }\
  os << endl;\
}

#define PRINT_MED_STRING(os, indent, name)\
  os << indent << #name << ": " << this->name->GetString() << endl; \

//ETX

#endif
