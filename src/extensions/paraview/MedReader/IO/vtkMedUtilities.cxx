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

#include "vtkMedUtilities.h"

#include "vtkMedMesh.h"
#include "vtkMedFamily.h"
#include "vtkMedGroup.h"
#include "vtkMedFamilyOnEntityOnProfile.h"
#include "vtkMedFamilyOnEntity.h"
#include "vtkMedEntityArray.h"
#include "vtkMedIntArray.h"
#include "vtkMedGrid.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkLongLongArray.h"
#include "vtkLongArray.h"
#include "vtkIdTypeArray.h"
#include "vtkCellType.h"
#include "vtkIdList.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkStringArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkOutEdgeIterator.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationObjectBaseKey.h"

#include <sstream>
using namespace std;
char vtkMedUtilities::Separator='/';

const char* vtkMedUtilities::NoGroupName="No Group";
const char* vtkMedUtilities::OnPointName="OnPoint";
const char* vtkMedUtilities::OnCellName="OnCell";

const int MED_TRIA_CHILD_TO_PARENT_INDEX[3][3]=
    {{0, 1, 3}, {1, 2, 4}, {2, 0, 5}};

const int MED_QUAD_CHILD_TO_PARENT_INDEX[4][3]=
    {{0, 1, 4}, {1, 2, 5}, {2, 3, 6}, {3, 0, 7}};

const int MED_TETRA_CHILD_TO_PARENT_INDEX[4][6]=
    {{0, 1, 2, 4, 5, 6},
     {0, 3, 1, 7, 8, 4},
     {1, 3, 2, 8, 9, 5},
     {2, 3, 0, 9, 7, 6}};

const int MED_HEXA_CHILD_TO_PARENT_INDEX[6][8]=
  {{0, 1, 2, 3, 8, 9, 10, 11},
   {4, 7, 6, 5, 15, 14, 13, 12},
   {0, 4, 5, 1, 16, 12, 17, 8},
   {1, 5, 6, 2, 17, 13, 18, 9},
   {2, 6, 7, 3, 18, 14, 19, 10},
   {3, 7, 4, 0, 19, 15, 16, 11}};

const int MED_PYRA_CHILD_TO_PARENT_INDEX[5][8]=
  {{0, 1, 2, 3, 5, 6, 7, 8},
   {0, 4, 1, -1, 9, 10, 5, -1},
   {1, 4, 2, -1, 10, 11, 6, -1},
   {2, 4, 3, -1, 11, 12, 7, -1},
   {3, 4, 0, -1, 12, 9, 8, -1}};

const int MED_PENTA_CHILD_TO_PARENT_INDEX[5][8]=
  {{0, 1, 2, -1, 6, 7, 8, -1},
   {3, 5, 4, -1, 11, 10, 9, -1},
   {0, 3, 4, 1, 12, 9, 13, 6},
   {1, 4, 5, 2, 13, 10, 14, 7},
   {2, 5, 3, 0, 14, 11, 12, 8}};

vtkInformationKeyMacro(vtkMedUtilities, ELNO, Integer);
vtkInformationKeyMacro(vtkMedUtilities, ELGA, Integer);
vtkInformationKeyMacro(vtkMedUtilities, BLOCK_NAME, StringVector);
vtkInformationKeyMacro(vtkMedUtilities, STRUCT_ELEMENT, ObjectBase);
vtkInformationKeyMacro(vtkMedUtilities, STRUCT_ELEMENT_INDEX, Integer);

vtkDataArray* vtkMedUtilities::NewCoordArray()
{
  return vtkMedUtilities::NewArray(MED_FLOAT64);
}

vtkDataArray* vtkMedUtilities::NewArray(med_field_type type)
{
  switch(type)
  {
    case MED_FLOAT64:
      if(sizeof(double)==8 && sizeof(med_float)==8)
        return vtkDoubleArray::New();
      vtkGenericWarningMacro("double type do not match med_float, aborting");
      return NULL;
    case MED_INT32:
      if(sizeof(vtkIdType)==4)
        return vtkIdTypeArray::New();
      if(sizeof(int)==4)
        return vtkIntArray::New();
      if(sizeof(long)==4)
        return vtkLongArray::New();
      vtkGenericWarningMacro("No vtk type matches MED_INT32, aborting");
      return NULL;
    case MED_INT64:
      if(sizeof(vtkIdType)==8)
        return vtkIdTypeArray::New();
      if(sizeof(long)==8)
        return vtkLongArray::New();
      if(sizeof(long long)==8)
        return vtkLongLongArray::New();
      vtkGenericWarningMacro("No vtk type matches MED_INT64, aborting");
      ;
      return NULL;
    case MED_INT:
      if(sizeof(med_int)==4)
        return vtkMedUtilities::NewArray(MED_INT32);
      if(sizeof(med_int)==8)
        return vtkMedUtilities::NewArray(MED_INT64);
      vtkGenericWarningMacro("No vtk type matches MED_INT, aborting");
      return NULL;
    default:
      vtkGenericWarningMacro("the array type is not known, aborting.");
      return NULL;
  }
}

vtkAbstractArray* vtkMedUtilities::NewArray(med_attribute_type type)
{
  switch(type)
    {
    case MED_ATT_FLOAT64 :
      if(sizeof(double) == sizeof(med_float))
        return vtkDoubleArray::New();
      vtkGenericWarningMacro("double type do not match med_float, aborting");
      return NULL;
    case MED_ATT_INT :
      if(sizeof(vtkIdType) == sizeof(med_int))
        return vtkIdTypeArray::New();
      if(sizeof(int) == sizeof(med_int))
        return vtkIntArray::New();
      if(sizeof(long) == sizeof(med_int))
        return vtkLongArray::New();
      if(sizeof(long long) == sizeof(med_int))
        return vtkLongLongArray::New();
      vtkGenericWarningMacro("med_int type does not match known VTK type, aborting");
      return NULL;
    case MED_ATT_NAME :
      return vtkStringArray::New();
    }
  return NULL;
}

const char* vtkMedUtilities::GeometryName(med_geometry_type geometry)
{
  switch(geometry)
  {
    case MED_POINT1:
      return "MED_POINT1";
    case MED_SEG2:
      return "MED_SEG2";
    case MED_SEG3:
      return "MED_SEG3";
    case MED_SEG4:
      return "MED_SEG4";
    case MED_TRIA3:
      return "MED_TRIA3";
    case MED_QUAD4:
      return "MED_QUAD4";
    case MED_TRIA6:
      return "MED_TRIA6";
    case MED_TRIA7:
      return "MED_TRIA7";
    case MED_QUAD8:
      return "MED_QUAD8";
    case MED_QUAD9:
      return "MED_QUAD9";
    case MED_TETRA4:
      return "MED_TETRA4";
    case MED_PYRA5:
      return "MED_PYRA5";
    case MED_PENTA6:
      return "MED_PENTA6";
    case MED_HEXA8:
      return "MED_HEXA8";
    case MED_TETRA10:
      return "MED_TETRA10";
    case MED_OCTA12:
      return "MED_OCTA12";
    case MED_PYRA13:
      return "MED_PYRA13";
    case MED_PENTA15:
      return "MED_PENTA15";
    case MED_HEXA20:
      return "MED_HEXA20";
    case MED_HEXA27:
      return "MED_HEXA27";
    case MED_POLYGON:
      return "MED_POLYGON";
    case MED_POLYHEDRON:
      return "MED_POLYHEDRON";
    case MED_STRUCT_GEO_INTERNAL:
      return "MED_STRUCT_GEO_INTERNAL";
    case MED_NO_GEOTYPE:
      return "MED_NO_GEOTYPE";
    default:
      return "UNKNOWN_GEOMETRY";
  }
}

const char* vtkMedUtilities::EntityName(med_entity_type type)
{
  switch(type)
    {
    case MED_CELL:
      return "MED_CELL";
    case MED_DESCENDING_FACE:
      return "MED_DESCENDING_FACE";
    case MED_DESCENDING_EDGE:
      return "MED_DESCENDING_EDGE";
    case MED_NODE:
      return "MED_NODE";
    case MED_NODE_ELEMENT:
      return "MED_NODE_ELEMENT";
    case MED_STRUCT_ELEMENT:
      return "MED_STRUCT_ELEMENT";
    case MED_UNDEF_ENTITY_TYPE:
      return "MED_UNDEF_ENTITY_TYPE";
    default:
      return "UNKNOWN_ENTITY_TYPE ";
  }
}

const char* vtkMedUtilities::ConnectivityName(med_connectivity_mode conn)
{
  switch(conn)
    {
    case MED_NODAL:
      return "MED_NODAL";
    case MED_DESCENDING:
      return "MED_DESCENDING";
    case MED_NO_CMODE:
      return "MED_NO_CMODE";
    default:
      return "UNKNOWN_CONNECTIVITY_MODE";
  }
}

const std::string vtkMedUtilities::SimplifyName(const char* medName)
{
  ostringstream sstr;
  bool underscore=false;
  bool space=false;
  int l=strlen(medName);
  for(int cc=0; cc<l; cc++)
    {
    if(medName[cc]==' ')
      {
      space=true;
      continue;
      }
    else if(medName[cc]=='_')
      {
      underscore=true;
      continue;
      }
    else
      {
      if(underscore||space)
        sstr<<'_';
      underscore=false;
      space=false;
      sstr<<medName[cc];
      }
    }
  return sstr.str();
}

const std::string vtkMedUtilities::FamilyKey(const char* meshName,
    int pointOrCell, const char* familyName)
{
  ostringstream sstr;
  sstr<<"FAMILY"<<Separator<<SimplifyName(meshName)<<Separator;
  if(pointOrCell==OnCell)
    sstr<<vtkMedUtilities::OnCellName;
  else
    sstr<<vtkMedUtilities::OnPointName;
  sstr<<Separator<<SimplifyName(familyName);
  return sstr.str();
}

const std::string vtkMedUtilities::GroupKey(const char* meshName,
    int pointOrCell, const char* groupName)
{
  ostringstream sstr;
  sstr << "GROUP" << vtkMedUtilities::Separator
      << vtkMedUtilities::SimplifyName(meshName)
      << vtkMedUtilities::Separator;
  if(pointOrCell==OnCell)
    sstr << vtkMedUtilities::OnCellName;
  else
    sstr << vtkMedUtilities::OnPointName;
  if(groupName==NULL)
    sstr << vtkMedUtilities::Separator
        << vtkMedUtilities::NoGroupName;
  else
    sstr << vtkMedUtilities::Separator
        << vtkMedUtilities::SimplifyName(groupName);

  return sstr.str();
}

const std::string vtkMedUtilities::EntityKey(const vtkMedEntity& entity)
{
  ostringstream sstr;
  sstr << "CELL_TYPE" << Separator << EntityName(entity.EntityType)
      << Separator<<entity.GeometryName;
  return sstr.str();
}

int vtkMedUtilities::GetNumberOfPoint(med_geometry_type geometry)
{
  return geometry%100;
}

int vtkMedUtilities::GetDimension(med_geometry_type geometry)
{
  return geometry/100;
}

int vtkMedUtilities::GetVTKCellType(med_geometry_type geometry)
{

  switch(geometry)
  {
    case MED_POINT1:
      return VTK_VERTEX;
    case MED_SEG2:
      return VTK_LINE;
    case MED_SEG3:
      return VTK_QUADRATIC_EDGE;
    case MED_SEG4:
      return VTK_CUBIC_LINE;
    case MED_TRIA3:
      return VTK_TRIANGLE;
    case MED_QUAD4:
      return VTK_QUAD;
    case MED_TRIA6:
      return VTK_QUADRATIC_TRIANGLE;
    case MED_TRIA7:
      return VTK_BIQUADRATIC_TRIANGLE;
    case MED_QUAD8:
      return VTK_QUADRATIC_QUAD;
    case MED_QUAD9:
      return VTK_BIQUADRATIC_QUAD;
    case MED_TETRA4:
      return VTK_TETRA;
    case MED_PYRA5:
      return VTK_PYRAMID;
    case MED_PENTA6:
      return VTK_WEDGE;
    case MED_HEXA8:
      return VTK_HEXAHEDRON;
    case MED_TETRA10:
      return VTK_QUADRATIC_TETRA;
    case MED_OCTA12:
      return VTK_HEXAGONAL_PRISM;
    case MED_PYRA13:
      return VTK_QUADRATIC_PYRAMID;
    case MED_PENTA15:
      return VTK_QUADRATIC_WEDGE;
    case MED_HEXA20:
      return VTK_QUADRATIC_HEXAHEDRON;
    case MED_HEXA27:
      return VTK_TRIQUADRATIC_HEXAHEDRON;
    case MED_POLYGON:
      return VTK_POLYGON;
    case MED_POLYHEDRON:
      return VTK_POLYHEDRON;
    case MED_NO_GEOTYPE:
      return VTK_EMPTY_CELL;
    default:
      vtkGenericWarningMacro("No vtk type matches " << geometry << ", aborting")
      ;
      return VTK_EMPTY_CELL;
  }
}

int vtkMedUtilities::MedToVTKIndex(int vtktype, int node)
{
  if(vtktype != VTK_TRIQUADRATIC_HEXAHEDRON)
    return node;

  static int VTK_TRIQUADRATIC_HEXAHEDRON_MED_TO_VTK_INDEX[27] =
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
     24, 22, 21, 23, 20, 25, 26};

  return VTK_TRIQUADRATIC_HEXAHEDRON_MED_TO_VTK_INDEX[node % 27] + static_cast<int>(27 * floor((double)node / 27));
}

int vtkMedUtilities::GetNumberOfNodes(med_geometry_type geometry)
{
  switch(geometry)
  {
    case MED_POINT1:
      return 1;
    case MED_SEG2:
      return 2;
    case MED_SEG3:
      return 3;
    case MED_SEG4:
      return 4;
    case MED_TRIA3:
      return 3;
    case MED_QUAD4:
      return 4;
    case MED_TRIA6:
      return 6;
    case MED_TRIA7:
      return 7;
    case MED_QUAD8:
      return 8;
    case MED_QUAD9:
      return 9;
    case MED_TETRA4:
      return 4;
    case MED_PYRA5:
      return 5;
    case MED_PENTA6:
      return 5;
    case MED_HEXA8:
      return 8;
    case MED_TETRA10:
      return 10;
    case MED_OCTA12:
      return 12;
    case MED_PYRA13:
      return 13;
    case MED_PENTA15:
      return 15;
    case MED_HEXA20:
      return 20;
    case MED_HEXA27:
      return 27;
    case MED_POLYGON:
      return -1;
    case MED_POLYHEDRON:
      return -1;
    case MED_NO_GEOTYPE:
      return 0;
    default:
      vtkGenericWarningMacro("No vtk type matches "
                             << vtkMedUtilities::GeometryName(geometry)
                             << ", aborting");
      return -1;
  }
}

int vtkMedUtilities::GetNumberOfSubEntity(med_geometry_type geometry)
{
  switch(geometry)
  {
    case MED_POINT1:
      return 0;
    case MED_SEG2:
      return 2;
    case MED_SEG3:
      return 3;
    case MED_SEG4:
      return 4;
    case MED_TRIA3:
      return 3;
    case MED_QUAD4:
      return 4;
    case MED_TRIA6:
      return 3;
    case MED_TRIA7:
      return 3;
    case MED_QUAD8:
      return 4;
    case MED_QUAD9:
      return 4;
    case MED_TETRA4:
      return 4;
    case MED_PYRA5:
      return 5;
    case MED_PENTA6:
      return 5;
    case MED_HEXA8:
      return 6;
    case MED_TETRA10:
      return 4;
    case MED_OCTA12:
      return 8;
    case MED_PYRA13:
      return 5;
    case MED_PENTA15:
      return 5;
    case MED_HEXA20:
      return 6;
    case MED_HEXA27:
      return 6;
    case MED_POLYGON:
      return -1;
    case MED_POLYHEDRON:
      return -1;
    case MED_NO_GEOTYPE:
      return 0;
    default:
      vtkGenericWarningMacro("No vtk type matches "
                             << geometry
                             << ", aborting");
      return -1;
  }
}

med_entity_type vtkMedUtilities::GetSubType(med_entity_type type)
{
  switch(type)
    {
    case MED_CELL:
      return MED_DESCENDING_FACE;
    case MED_DESCENDING_FACE:
      return MED_DESCENDING_EDGE;
    case MED_DESCENDING_EDGE:
      return MED_NODE;
    default:
      return MED_NODE;
    }
}

med_geometry_type vtkMedUtilities::GetSubGeometry(
    med_geometry_type geometry, int index)
{
  switch(geometry)
  {
    case MED_SEG2:
      return MED_POINT1;
    case MED_SEG3:
      return MED_POINT1;
    case MED_SEG4:
      return MED_POINT1;

    case MED_TRIA3:
      return MED_SEG2;
    case MED_TRIA6:
      return MED_SEG3;
    case MED_TRIA7:
      return MED_SEG3;

    case MED_QUAD4:
      return MED_SEG2;
    case MED_QUAD8:
      return MED_SEG3;
    case MED_QUAD9:
      return MED_SEG3;

    case MED_TETRA4:
      return MED_TRIA3;
    case MED_TETRA10:
      return MED_TRIA6;

    case MED_PYRA5:
      {
      if(index==0)
        return MED_QUAD4;
      return MED_TRIA3;
      }
    case MED_PYRA13:
      {
      if(index==0)
        return MED_QUAD8;
      else
        return MED_TRIA6;
      }

    case MED_PENTA6:
      {
      if(index==0||index==1)
        return MED_TRIA3;
      else
        return MED_QUAD4;
      }
    case MED_PENTA15:
      {
      if(index==0||index==1)
        return MED_TRIA6;
      else
        return MED_QUAD8;
      }

    case MED_HEXA8:
      return MED_QUAD4;
    case MED_HEXA20:
      return MED_QUAD8;
    case MED_HEXA27:
      return MED_QUAD9;
    default:
      return MED_NONE;
  }
}

int vtkMedUtilities::FormatPolyhedronForVTK(
    vtkMedFamilyOnEntityOnProfile* foep, vtkIdType index,
    vtkIdList* ids )
{
  vtkMedEntityArray* array = foep->GetFamilyOnEntity()->GetEntityArray();
  vtkMedIntArray* conn = array->GetConnectivityArray();
  vtkMedIntArray* faceIndex = array->GetFaceIndex();
  vtkMedIntArray* nodeIndex = array->GetNodeIndex();
  med_int start = faceIndex->GetValue(index)-1;
  med_int end = faceIndex->GetValue(index+1)-1;

  // The format for the Polyhedrons is:
  //(numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
  ids->Reset();

  if (array->GetConnectivity()==MED_NODAL)
    {
    ids->InsertNextId(end-start);
    for (int ff = start; ff<end; ff++)
      {
      med_int fstart = nodeIndex->GetValue(ff)-1;
      med_int fend = nodeIndex->GetValue(ff+1)-1;
      ids->InsertNextId(fend-fstart);
      for (med_int pt = fstart; pt<fend; pt++)
        {
        vtkIdType realIndex = foep->GetVTKPointIndex(conn->GetValue(pt)-1);
        if(realIndex < 0)
          {
          vtkGenericWarningMacro("this polyhedron is not on this profile");
          foep->SetValid(0);
          return 0;
          }
        ids->InsertNextId(realIndex);
        }
      }
    }

  if (array->GetConnectivity()==MED_DESCENDING)
    {
    ids->InsertNextId(end-start);
    vtkSmartPointer<vtkIdList> subIds = vtkSmartPointer<vtkIdList>::New();

    for (int i = 0 ; i<nodeIndex->GetSize(); i++)
      {
      int numPoints =
          vtkMedUtilities::GetNumberOfSubEntity(nodeIndex->GetValue(i));
      ids->InsertNextId(numPoints);

      vtkMedEntity entity;
      entity.EntityType = MED_DESCENDING_FACE;
      entity.GeometryType = nodeIndex->GetValue(i);

      vtkMedEntityArray* theFaces =
          array->GetParentGrid()->GetEntityArray(entity);

      theFaces->GetCellVertices(conn->GetValue(i)-1, subIds);

      for (int j = 0 ; j< numPoints; j++)
        {
        vtkIdType realIndex = foep->GetVTKPointIndex(subIds->GetId(j));
        if(realIndex < 0)
          {
          vtkGenericWarningMacro("this polyhedron is not on this profile");
          return 0;
          }
        ids->InsertNextId(realIndex);
        }
      }
    }
  return 1;
}

void vtkMedUtilities::SplitGroupKey(const char* name, vtkStdString& mesh,
    vtkStdString& entity, vtkStdString& group)
{
  vtkStdString remain;
  remain=name;
  mesh="*";
  entity="*";
  group="*";
  vtkStdString header="*";

  if(remain=="*")
    {
    return;
    }
  vtkStdString::size_type pos;
  // First get the header, which must be "GROUP"
  pos=remain.find_first_of(vtkMedUtilities::Separator);
  header=remain.substr(0, pos);
  remain=remain.substr(pos+1, remain.size()-pos-1);

  // then get the mesh name
  pos=remain.find_first_of(vtkMedUtilities::Separator);
  mesh=remain.substr(0, pos);
  if(mesh=="*"||pos==remain.size()-1)
    return;
  remain=remain.substr(pos+1, remain.size()-pos-1);

  // then the entity name (OnPoint or OnCell)
  pos=remain.find_first_of(vtkMedUtilities::Separator);
  entity=remain.substr(0, pos);
  if(entity=="*"||pos==remain.size()-1)
    return;

  // then the group
  group=remain.substr(pos+1, remain.size()-pos-1);
}

int vtkMedUtilities::GetParentNodeIndex(med_geometry_type parentGeometry,
    int subEntityIndex, int subEntityNodeIndex)
{
  switch(parentGeometry)
  {
    case MED_TRIA3:
    case MED_TRIA6:
    case MED_TRIA7:
      return MED_TRIA_CHILD_TO_PARENT_INDEX[subEntityIndex][subEntityNodeIndex];
    case MED_QUAD4:
    case MED_QUAD8:
    case MED_QUAD9:
      return MED_QUAD_CHILD_TO_PARENT_INDEX[subEntityIndex][subEntityNodeIndex];
    case MED_TETRA4:
    case MED_TETRA10:
      return MED_TETRA_CHILD_TO_PARENT_INDEX[subEntityIndex][subEntityNodeIndex];
    case MED_PYRA5:
    case MED_PYRA13:
      return MED_PYRA_CHILD_TO_PARENT_INDEX[subEntityIndex][subEntityNodeIndex];
    case MED_PENTA6:
    case MED_PENTA15:
      return MED_PENTA_CHILD_TO_PARENT_INDEX[subEntityIndex][subEntityNodeIndex];
    case MED_HEXA8:
    case MED_HEXA20:
    case MED_HEXA27:
      return MED_HEXA_CHILD_TO_PARENT_INDEX[subEntityIndex][subEntityNodeIndex];
  }
  return -1;
}

void vtkMedUtilities::ProjectConnectivity(med_geometry_type parentGeometry,
    vtkIdList* parentIds, vtkIdList* subEntityIds, int subEntityIndex, bool invert)
{
  for(int subEntityNodeIndex=0; subEntityNodeIndex
      <subEntityIds->GetNumberOfIds(); subEntityNodeIndex++)
    {
    int realIndex = subEntityNodeIndex;
    if(invert)
      realIndex = subEntityIds->GetNumberOfIds() - subEntityNodeIndex - 1;
    parentIds->SetId(GetParentNodeIndex(parentGeometry, subEntityIndex,
        subEntityNodeIndex), subEntityIds->GetId(realIndex));
    }
}

std::string vtkMedUtilities::GetModeKey(int index, double frequency, int maxindex)
{
  std::ostringstream key;
  key<<"[";
  if(maxindex > 0)
    {
      int maxdecim = (int)floor(log(1.0*maxindex)/log(10.0));
    int decim = 0;
    if(index > 0)
      {
        decim = (int)floor(log(1.0*index)/log(10.0));
      }
    for(int i=decim; i<maxdecim; i++)
      {
      key << "0";
      }
    }

  key<<index<<"] "<<frequency;
  return key.str();
}

int vtkMedUtilities::GetModeFromKey(const char* key, int& index,
    double& frequency)
{
  const std::string k(key);
  size_t index_start=k.find("[");
  size_t index_end=k.find("]");
  const string index_string=k.substr(index_start, index_end);
  stringstream indexsstr;
  indexsstr<<index_string;
  indexsstr>>index;
  const string freq_string=k.substr(index_end+1, string::npos);
  stringstream freqsstr;
  freqsstr<<freq_string;
  freqsstr>>frequency;
  return 1;
}

vtkMultiBlockDataSet* vtkMedUtilities::GetParent(vtkMultiBlockDataSet* root,
                                vtkStringArray* path)
{
    vtkMultiBlockDataSet* output=root;
    vtkMultiBlockDataSet* parent=output;
    for(int depth = 0; depth<path->GetNumberOfValues(); depth++)
      {
      vtkStdString parentName = path->GetValue(depth);
      bool found=false;
      for(int blockId=0; blockId<parent->GetNumberOfBlocks(); blockId++)
        {
        vtkInformation* metaData=parent->GetMetaData(blockId);
        if(metaData->Has(vtkCompositeDataSet::NAME()))
          {
          const char* blockName=metaData->Get(vtkCompositeDataSet::NAME());
          if(parentName==blockName &&
              vtkMultiBlockDataSet::SafeDownCast(
                  parent->GetBlock(blockId))!=NULL)
            {
            parent=vtkMultiBlockDataSet::SafeDownCast(parent->GetBlock(blockId));
            found=true;
            break;
            }
          }
        }
      if (!found)
        {
        // If I am here, it means that I did not find any block with the good name, create one
        int nb=parent->GetNumberOfBlocks();
        vtkMultiBlockDataSet* block=vtkMultiBlockDataSet::New();
        parent->SetBlock(nb, block);
        block->Delete();
        parent->GetMetaData(nb)->Set(vtkCompositeDataSet::NAME(),
            parentName.c_str());
        parent=block;
        }
      }
    return parent;
}

int vtkMedUtilities::SizeOf(med_attribute_type type)
{
  switch(type)
    {
    case MED_ATT_FLOAT64 : return sizeof(med_float);
    case MED_ATT_INT : return sizeof(med_int);
    case MED_ATT_NAME : return MED_NAME_SIZE * sizeof(char);
    }
  return 0;
}

bool operator==(const vtkMedComputeStep& cs0, const vtkMedComputeStep& cs1)
{
  return cs0.IterationIt == cs1.IterationIt && cs0.TimeIt == cs1.TimeIt;
}

bool operator!=(const vtkMedComputeStep& cs0, const vtkMedComputeStep& cs1)
{
  return cs0.IterationIt != cs1.IterationIt || cs0.TimeIt != cs1.TimeIt;
}

bool operator<(const vtkMedComputeStep& cs0, const vtkMedComputeStep& cs1)
{
  if(cs0.IterationIt != cs1.IterationIt)
    return cs0.IterationIt < cs1.IterationIt;
  return cs0.TimeIt < cs1.TimeIt;
}

bool operator==(const vtkMedEntity& e0, const vtkMedEntity& e1)
{
  return e0.EntityType == e1.EntityType && e0.GeometryType == e1.GeometryType;
}

bool operator!=(const vtkMedEntity& e0, const vtkMedEntity& e1)
{
  return e0.EntityType != e1.EntityType || e0.GeometryType != e1.GeometryType;
}

bool operator<(const vtkMedEntity& e0, const vtkMedEntity& e1)
{
  if(e0.EntityType != e1.EntityType)
    return e0.EntityType < e1.EntityType;
  return e0.GeometryType < e1.GeometryType;
}
