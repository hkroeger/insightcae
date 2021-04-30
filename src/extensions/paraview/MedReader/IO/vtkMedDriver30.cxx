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

#include "vtkMedDriver30.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkCharArray.h"

#include "vtkMedFile.h"
#include "vtkMedCartesianGrid.h"
#include "vtkMedPolarGrid.h"
#include "vtkMedCurvilinearGrid.h"
#include "vtkMedUnstructuredGrid.h"
#include "vtkMedField.h"
#include "vtkMedMesh.h"
#include "vtkMedFamily.h"
#include "vtkMedUtilities.h"
#include "vtkMedEntityArray.h"
#include "vtkMedLocalization.h"
#include "vtkMedProfile.h"
#include "vtkMedFieldOverEntity.h"
#include "vtkMedFieldStep.h"
#include "vtkMedGroup.h"
#include "vtkMedIntArray.h"
#include "vtkMedInterpolation.h"
#include "vtkMedFieldOnProfile.h"
#include "vtkMedFraction.h"
#include "vtkMedLink.h"
#include "vtkMedStructElement.h"
#include "vtkMedConstantAttribute.h"
#include "vtkMedVariableAttribute.h"

#include "vtkMultiProcessController.h"

#include <string>
#include <vector>
#include <algorithm>
using namespace std;

vtkStandardNewMacro(vtkMedDriver30)

vtkMedDriver30::vtkMedDriver30() : vtkMedDriver()
{
}

vtkMedDriver30::~vtkMedDriver30()
{
}

// Description:
// load all Information data associated with this cartesian grid.
void vtkMedDriver30::ReadRegularGridInformation(vtkMedRegularGrid* grid)
{
  FileOpen open(this);

  grid->SetDimension(grid->GetParentMesh()->GetNumberOfAxis());

  for(int axis=0; axis < grid->GetDimension(); axis++)
    {
    med_int size;
    med_bool coordinatechangement, geotransformation;

    if ((size = MEDmeshnEntity(
                  this->FileId,
                  grid->GetParentMesh()->GetName(),
                  grid->GetComputeStep().TimeIt,
                  grid->GetComputeStep().IterationIt,
                  MED_NODE,
                  MED_NONE,
                  (med_data_type)(MED_COORDINATE_AXIS1 + axis),
                  MED_NO_CMODE,
                  &coordinatechangement,
                  &geotransformation)) < 0)
      {
      vtkErrorMacro("ERROR : number of coordinates on X axis ...");
      }

    grid->SetAxisSize(axis, size);
    }

  med_int ncell = 1;
  if(grid->GetAxisSize(0) > 1)
    ncell *= grid->GetAxisSize(0)-1;
  if(grid->GetAxisSize(1) > 1)
    ncell *= grid->GetAxisSize(1)-1;
  if(grid->GetAxisSize(2) > 1)
    ncell *= grid->GetAxisSize(2)-1;

  vtkMedEntity entity;
  entity.EntityType = MED_CELL;

  switch(grid->GetDimension())
    {
    case 0 :
      entity.GeometryType = MED_POINT1;
      break;
    case 1 :
      entity.GeometryType = MED_SEG2;
      break;
    case 2 :
      entity.GeometryType = MED_QUAD4;
      break;
    case 3 :
      entity.GeometryType = MED_HEXA8;
      break;
    default :
        vtkErrorMacro("Unsupported dimension for curvilinear grid : "
                      << grid->GetDimension());
    return;
    }

  vtkMedEntityArray* array = vtkMedEntityArray::New();
  array->SetParentGrid(grid);
  array->SetNumberOfEntity(ncell);
  array->SetEntity(entity);
  array->SetConnectivity(MED_NODAL);
  grid->AppendEntityArray(array);
  array->Delete();
  // this triggers the creation of undefined families
  this->LoadFamilyIds(array);

}

void  vtkMedDriver30::LoadRegularGridCoordinates(vtkMedRegularGrid* grid)
{
  FileOpen open(this);

  for(int axis=0; axis < grid->GetParentMesh()->GetNumberOfAxis(); axis++)
    {

    vtkDataArray* coords = vtkMedUtilities::NewCoordArray();
    grid->SetAxisCoordinate(axis, coords);
    coords->Delete();

    coords->SetNumberOfComponents(1);
    coords->SetNumberOfTuples(grid->GetAxisSize(axis));

    if (MEDmeshGridIndexCoordinateRd(
          this->FileId,
          grid->GetParentMesh()->GetName(),
          grid->GetComputeStep().TimeIt,
          grid->GetComputeStep().IterationIt,
          axis+1,
          (med_float*)coords->GetVoidPointer(0)) < 0)
      {
      vtkErrorMacro("ERROR : read axis " << axis << " coordinates ...");
      grid->SetAxisCoordinate(axis, NULL);
      return;
      }
    }
}

// Description:
// load all Information data associated with this standard grid.
void vtkMedDriver30::ReadCurvilinearGridInformation(vtkMedCurvilinearGrid* grid)
{
  FileOpen open(this);

  grid->SetDimension(grid->GetParentMesh()->GetNumberOfAxis());

  med_int size;
  med_bool coordinatechangement, geotransformation;

  if ((size = MEDmeshnEntity(
                this->FileId,
                grid->GetParentMesh()->GetName(),
                grid->GetComputeStep().TimeIt,
                grid->GetComputeStep().IterationIt,
                MED_NODE,
                MED_NONE,
                MED_COORDINATE,
                MED_NO_CMODE,
                &coordinatechangement,
                &geotransformation)) < 0)
    {
    vtkErrorMacro("ReadCurvilinearGridInformation MEDmeshnEntity");
    }

  grid->SetNumberOfPoints(size);

  med_int axissize[3];
  if(MEDmeshGridStructRd(
      this->FileId,
      grid->GetParentMesh()->GetName(),
      grid->GetComputeStep().TimeIt,
      grid->GetComputeStep().IterationIt,
      axissize) < 0)
    {
    vtkErrorMacro("ReadCurvilinearGridInformation MEDmeshGridStructRd");
    }
  grid->SetAxisSize(0, (axissize[0] <= 0 ? 1: axissize[0]));
  grid->SetAxisSize(1, (axissize[1] <= 0 ? 1: axissize[1]));
  grid->SetAxisSize(2, (axissize[2] <= 0 ? 1: axissize[2]));

  // A test to verify the number of points : total number of
  // points should be equal to the product of each axis size
  if(size != grid->GetAxisSize(0)*grid->GetAxisSize(1)*grid->GetAxisSize(2))
    {
    vtkErrorMacro("The total number of points of a Curvilinear grid should "
                  << "be the product of each axis size!");
    }

  med_int ncell = 1;
  if(grid->GetAxisSize(0) > 1)
    ncell *= grid->GetAxisSize(0)-1;
  if(grid->GetAxisSize(1) > 1)
    ncell *= grid->GetAxisSize(1)-1;
  if(grid->GetAxisSize(2) > 1)
    ncell *= grid->GetAxisSize(2)-1;

  vtkMedEntity entity;
  entity.EntityType = MED_CELL;

  switch(grid->GetDimension())
    {
    case 0 :
      entity.GeometryType = MED_POINT1;
      break;
    case 1 :
      entity.GeometryType = MED_SEG2;
      break;
    case 2 :
      entity.GeometryType = MED_QUAD4;
      break;
    case 3 :
      entity.GeometryType = MED_HEXA8;
      break;
    default :
        vtkErrorMacro("Unsupported dimension for curvilinear grid : "
                      << grid->GetDimension());
    return;
    }

  vtkMedEntityArray* array = vtkMedEntityArray::New();
  array->SetParentGrid(grid);
  array->SetNumberOfEntity(ncell);
  array->SetEntity(entity);
  array->SetConnectivity(MED_NODAL);
  grid->AppendEntityArray(array);
  array->Delete();
  // this triggers the creation of undefined families
  this->LoadFamilyIds(array);

}

// Description : read the number of entity of all geometry type
// for a given entity type and a given connectivity mode
void vtkMedDriver30::ReadNumberOfEntity(
    vtkMedUnstructuredGrid* grid,
    med_entity_type entityType,
    med_connectivity_mode connectivity)
{
  FileOpen open(this);

  med_bool changement, transformation;

  const char* meshName = grid->GetParentMesh()->GetName();

  const vtkMedComputeStep& cs = grid->GetComputeStep();

  med_int nentity = MEDmeshnEntity(
                        this->FileId,
                        meshName,
                        cs.TimeIt,
                        cs.IterationIt,
                        entityType,
                        MED_GEO_ALL,
                        MED_UNDEF_DATATYPE ,
                        connectivity,
                        &changement,
                        &transformation );

  for(med_int geotypeit = 1; geotypeit <= nentity; geotypeit++)
    {
    // read cell informations
    vtkMedEntity entity;
    entity.EntityType = entityType;

    char geometryName[MED_NAME_SIZE+1] = "";

    // this gives us the med_geometry_type
    if( MEDmeshEntityInfo( FileId, meshName,
                           cs.TimeIt,
                           cs.IterationIt,
                           entityType,
                           geotypeit,
                           geometryName,
                           &entity.GeometryType) < 0)
      {
      vtkErrorMacro("MEDmeshEntityInfo");
      continue;
      }

    entity.GeometryName = geometryName;
    med_int ncell = 0;

    if(entity.GeometryType == MED_POLYGON)
      {
      // read the number of cells of this type
      ncell = MEDmeshnEntity(this->FileId,
                             meshName,
                             cs.TimeIt,
                             cs.IterationIt,
                             entity.EntityType,
                             entity.GeometryType,
                             MED_INDEX_NODE,
                             connectivity,
                             &changement,
                             &transformation ) - 1;
      }
    else if(entity.GeometryType == MED_POLYHEDRON)
      {
      // read the number of cells of this type
      ncell = MEDmeshnEntity(this->FileId,
                             meshName,
                             cs.TimeIt,
                             cs.IterationIt,
                             entity.EntityType,
                             entity.GeometryType,
                             MED_INDEX_FACE,
                             connectivity,
                             &changement,
                             &transformation  ) - 1;
      }
    else
      {
      ncell = MEDmeshnEntity(this->FileId,
                             meshName,
                             cs.TimeIt,
                             cs.IterationIt,
                             entity.EntityType,
                             entity.GeometryType,
                             MED_CONNECTIVITY,
                             connectivity,
                             &changement,
                             &transformation  );
      }

    if(ncell > 0)
      {
      vtkMedEntityArray* array = vtkMedEntityArray::New();
      array->SetParentGrid(grid);
      array->SetNumberOfEntity(ncell);
      array->SetEntity(entity);
      array->SetConnectivity(connectivity);
      grid->AppendEntityArray(array);
      array->Delete();
      // this triggers the creation of undefined families
      this->LoadFamilyIds(array);
      }
    }
}

// Description:
// load all Information data associated with this unstructured grid.
void vtkMedDriver30::ReadUnstructuredGridInformation(
    vtkMedUnstructuredGrid* grid)
{
  FileOpen open(this);

  vtkMedMesh *mesh = grid->GetParentMesh();

  const char *meshName = mesh->GetName();
  med_connectivity_mode connectivity;

  med_bool changement;
  med_bool transformation;
  med_int profilesize;

  char profilename[MED_NAME_SIZE+1];
  memset(profilename, '\0', MED_NAME_SIZE+1);

  vtkMedComputeStep cs = grid->GetComputeStep();

  // first check if we have points
  vtkIdType npoints = MEDmeshnEntityWithProfile(
                        this->FileId,
                        meshName,
                        cs.TimeIt,
                        cs.IterationIt,
                        MED_NODE,
                        MED_NONE,
                        MED_COORDINATE,
                        MED_NODAL,
                        MED_COMPACT_PFLMODE,
                        profilename,
                        &profilesize,
                        &changement,
                        &transformation);

  if(npoints > 0)
    {
    grid->SetNumberOfPoints(npoints);
    }
  else
    {
    if(grid->GetPreviousGrid() == NULL)
      {
      vtkErrorMacro("No point and no previous grid");
      }
    grid->SetUsePreviousCoordinates(true);
    grid->SetNumberOfPoints(grid->GetPreviousGrid()->GetNumberOfPoints());
    return;
    }

  this->ReadNumberOfEntity(grid, MED_CELL, MED_NODAL);
  this->ReadNumberOfEntity(grid, MED_CELL, MED_DESCENDING);
  this->ReadNumberOfEntity(grid, MED_DESCENDING_FACE, MED_NODAL);
  this->ReadNumberOfEntity(grid, MED_DESCENDING_FACE, MED_DESCENDING);
  this->ReadNumberOfEntity(grid, MED_DESCENDING_EDGE, MED_NODAL);
  this->ReadNumberOfEntity(grid, MED_DESCENDING_EDGE, MED_DESCENDING);
  this->ReadNumberOfEntity(grid, MED_STRUCT_ELEMENT, MED_NODAL);
  this->ReadNumberOfEntity(grid, MED_STRUCT_ELEMENT, MED_DESCENDING);

  // create the point vtkMedEntityArray support
  vtkMedEntity entity;
  entity.EntityType = MED_NODE;
  entity.GeometryType = MED_POINT1;
  vtkMedEntityArray* pea = vtkMedEntityArray::New();
  pea->SetEntity(entity);
  pea->SetParentGrid(grid);
  pea->SetNumberOfEntity(grid->GetNumberOfPoints());
  grid->AppendEntityArray(pea);
  pea->Delete();

  this->LoadFamilyIds(pea);
}

void vtkMedDriver30::ReadFamilyInformation(vtkMedMesh* mesh, vtkMedFamily* family)
{
  FileOpen open(this);

  med_int familyid;
  med_int ngroup;
  char* groupNames = NULL;
  const  char* meshName = mesh->GetName();

  ngroup = MEDnFamilyGroup(FileId, meshName, family->GetMedIterator());

  bool has_no_group = false;
  if(ngroup <= 0)
    {
    if(ngroup < 0)
      {
      vtkErrorMacro("Error while reading the number of groups");
      }
    ngroup = 1;
    has_no_group = true;
    }

  groupNames = new char[ngroup * MED_LNAME_SIZE + 1];
  memset(groupNames, '\0', ngroup * MED_LNAME_SIZE + 1);

  // special case for files written by med < 3,
  // I have to use the 23 interface
  if(mesh->GetParentFile()->GetVersionMajor() < 3)
    {
    med_int *attributenumber = NULL;
    med_int *attributevalue = NULL;
    char *attributedes = NULL;

    med_int nattr = MEDnFamily23Attribute(
                      this->FileId,
                      meshName,
                      family->GetMedIterator());

    if(nattr < 0)
      {
      vtkErrorMacro("MEDnFamily23Attribute");
      }

    if(nattr > 0)
      {
      attributenumber = new med_int[nattr];
      attributevalue = new med_int[nattr];
      attributedes = new char[nattr*MED_COMMENT_SIZE+1];
      memset(attributedes, '\0', nattr*MED_COMMENT_SIZE+1);
      }

    char familyName[MED_NAME_SIZE+1] = "";

    if(MEDfamily23Info (this->FileId,
                        meshName,
                        family->GetMedIterator(),
                        familyName,
                        attributenumber,
                        attributevalue,
                        attributedes,
                        &familyid,
                        groupNames ) < 0)
      {
      vtkDebugMacro("MEDfamily23Info");
      }

    family->SetName(familyName);

    if(attributenumber != NULL)
      delete[] attributenumber;
    if(attributevalue != NULL)
      delete[] attributevalue;
    if(attributedes != NULL)
      delete[] attributedes;
    }
  else
    {
    char familyName[MED_NAME_SIZE+1] = "";
    if(MEDfamilyInfo( this->FileId,
                      meshName,
                      family->GetMedIterator(),
                      familyName,
                      &familyid,
                      groupNames ) < 0)
      {
      vtkErrorMacro(
          "vtkMedDriver30::ReadInformation(vtkMedFamily* family)"
          << " cannot read family informations.");
      return;
      }
    family->SetName(familyName);
    }

  family->SetId(familyid);

  if( familyid == 0 ) {
    family->SetPointOrCell( vtkMedUtilities::OnCell );
    mesh->AppendCellFamily( family );
  } else {
    //rnv: improve algorithm to determine entity of the family:
    //     1) Read Nb nodes
    //     2) Read families of the nodes
    //     3) If list of the families of the nodes contains familyid => vtkMedUtilities::OnPoint
    //     otherwise => vtkMedUtilities::OnCell
    med_bool  v1, v2;
    med_int size = MEDmeshnEntity(
				  this->FileId,
				  meshName,
				  MED_NO_DT,
				  MED_NO_IT,
				  MED_NODE,
				  MED_NO_GEOTYPE,
				  MED_COORDINATE,
				  MED_NO_CMODE,
				  &v1,
				  &v2 );
    if( size < 0 ) {
      vtkErrorMacro( "vtkMedDriver30::ReadInformation(vtkMedFamily* family)"
		     <<" cannot read nb Nodes" );
      return;
    }
    
    vector<med_int> n_fams;
    n_fams.resize( size );
    
    med_int ret_val = MEDmeshEntityFamilyNumberRd( this->FileId,
                                                   meshName,
                                                   MED_NO_DT,
                                                   MED_NO_IT,
                                                   MED_NODE,
                                                   MED_NO_GEOTYPE ,
                                                   &n_fams[0] );
    // Remove ZERO FAMILY
    remove( n_fams.begin(),n_fams.end(), 0 );
    
    bool isOnPoints = ( ret_val >= 0) && (find( n_fams.begin(), n_fams.end(), familyid ) != n_fams.end() );
    if( isOnPoints ) {
      family->SetPointOrCell(vtkMedUtilities::OnPoint);
      mesh->AppendPointFamily(family);
    } else {
      family->SetPointOrCell(vtkMedUtilities::OnCell);	      
      mesh->AppendCellFamily(family);	
    }
  }
  
  family->AllocateNumberOfGroup(ngroup);
  // if there where no group, set the name to the default value
  if(has_no_group)
    {
    memcpy(groupNames, vtkMedUtilities::NoGroupName,
           strlen(vtkMedUtilities::NoGroupName));
    }

  for(int index = 0; index < ngroup; index++)
    {
    char realGroupName[MED_LNAME_SIZE + 1];
    memset(realGroupName, '\0', MED_LNAME_SIZE + 1);
    memcpy(realGroupName, groupNames + index * MED_LNAME_SIZE,
        MED_LNAME_SIZE * sizeof(char));
    vtkMedGroup* group = mesh->GetOrCreateGroup(family->GetPointOrCell(),
        realGroupName);

    family->SetGroup(index, group);
    }

  delete[] groupNames;

  if(familyid == 0)
    {
    vtkMedFamily* famzero = vtkMedFamily::New();
    mesh->AppendPointFamily(famzero);
    famzero->Delete();

    famzero->SetName(family->GetName());
    famzero->SetMedIterator(family->GetMedIterator());
    famzero->SetId(family->GetId());
    famzero->SetPointOrCell(vtkMedUtilities::OnPoint);
    famzero->AllocateNumberOfGroup(family->GetNumberOfGroup());
    for(int gid=0; gid<family->GetNumberOfGroup(); gid++)
      {
      vtkMedGroup* group = mesh->GetOrCreateGroup(
          vtkMedUtilities::OnPoint,
          family->GetGroup(gid)->GetName());
      famzero->SetGroup(gid, group);
      mesh->AppendPointGroup(group);
      }
    }
}

void  vtkMedDriver30::ReadLinkInformation(vtkMedLink* link)
{
  med_int size;
  char linkMeshName[MED_NAME_SIZE+1] = "";
  if(MEDlinkInfo(this->FileId,
                 link->GetMedIterator(),
                 linkMeshName,
                 &size) < 0)
    {
    vtkErrorMacro("MEDlinkInfo");
    return;
    }
  link->SetMeshName(linkMeshName);
  if(size <= 0)
    return;

  char* path = new char[size + 1];
  memset(path, '\0', size+1);
  if(MEDlinkRd(this->FileId, link->GetMeshName(), path) < 0)
    {
    vtkErrorMacro("MEDlinkRd");
    memset(path, '\0', size+1);
    }

  link->SetLink(path);

  delete[] path;
}

void vtkMedDriver30::ReadFileInformation(vtkMedFile* file)
{
  FileOpen open(this);

  char comment[MED_COMMENT_SIZE+1] = "";

  MEDfileCommentRd(this->FileId,
                  comment);

  file->SetComment(comment);

  med_int major, minor, release;
  MEDfileNumVersionRd(this->FileId, &major, &minor, &release);
  file->SetVersionMajor(major);
  file->SetVersionMinor(minor);
  file->SetVersionRelease(release);

  int nlink = MEDnLink(this->FileId);
  file->AllocateNumberOfLink(nlink);
  for(int linkid=0; linkid<nlink; linkid++)
    {
    vtkMedLink* link = file->GetLink(linkid);
    link->SetMedIterator(linkid+1);
    this->ReadLinkInformation(link);
    }

  int nprof = MEDnProfile(FileId);
  // Reading id s not possible in parallel if the file contains Profiles
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller != NULL)
    if ((nprof != 0) && (controller->GetNumberOfProcesses() > 1))
    {
      vtkWarningMacro("ATTENTION: The MED Reader cannot read profiles when used in parallel");
    return;
    }
  file->AllocateNumberOfProfile(nprof);
  for(int i = 0; i < nprof; i++)
    {
    vtkMedProfile* profile = file->GetProfile(i);
    profile->SetMedIterator(i + 1);
    profile->SetParentFile(file);
    this->ReadProfileInformation(profile);
    }

  int nloc = MEDnLocalization(this->FileId);
  file->AllocateNumberOfLocalization(nloc);
  for(int i = 0; i < nloc; i++)
    {
    vtkMedLocalization* loc = file->GetLocalization(i);
    loc->SetMedIterator(i + 1);
    loc->SetParentFile(file);
    this->ReadLocalizationInformation(loc);
    }

  int nsupportmesh = MEDnSupportMesh(this->FileId);
  file->AllocateNumberOfSupportMesh(nsupportmesh);
  for(int i = 0; i < nsupportmesh; i++)
    {
    vtkMedMesh* supportmesh = file->GetSupportMesh(i);
    supportmesh->SetMedIterator(i + 1);
    supportmesh->SetParentFile(file);
    this->ReadSupportMeshInformation(supportmesh);
    }

  int nmesh = MEDnMesh(this->FileId);
  file->AllocateNumberOfMesh(nmesh);
  for(int i = 0; i < nmesh; i++)
    {
    vtkMedMesh* mesh = file->GetMesh(i);
    mesh->SetMedIterator(i + 1);
    mesh->SetParentFile(file);
    this->ReadMeshInformation(mesh);
    }

  int nfields = MEDnField(this->FileId);
  file->AllocateNumberOfField(nfields);
  for(int i = 0; i < nfields; i++)
    {
    vtkMedField* field = file->GetField(i);
    field->SetMedIterator(i + 1);
    field->SetParentFile(file);
    this->ReadFieldInformation(field);
    field->ComputeFieldType();
    while(field->HasManyFieldTypes())
      {
      vtkMedField* newfield = vtkMedField::New();
      int type = field->GetFirstType();
      newfield->ExtractFieldType(field, type);
      file->AppendField(newfield);
      newfield->Delete();
      }
    }

  int nstruct = MEDnStructElement(this->FileId);

  file->AllocateNumberOfStructElement(nstruct);
  for(int i = 0; i < nstruct; i++)
    {
    vtkMedStructElement* structelem = file->GetStructElement(i);
    structelem->SetMedIterator(i+1);
    structelem->SetParentFile(file);
    this->ReadStructElementInformation(structelem);
    }

}

void  vtkMedDriver30::ReadStructElementInformation(
    vtkMedStructElement* structelem)
{

  FileOpen open(this);

  char modelname[MED_NAME_SIZE+1] = "";
  med_geometry_type mgeotype;
  med_int modeldim;
  char supportmeshname[MED_NAME_SIZE+1] = "";
  med_entity_type sentitytype;
  med_int snbofnode;
  med_int snbofcell;
  med_geometry_type sgeotype;
  med_int nbofconstantattribute;
  med_bool anyprofile;
  med_int nbofvariableattribute;

  if(MEDstructElementInfo (this->FileId,
                           structelem->GetMedIterator(),
                           modelname,
                           &mgeotype,
                           &modeldim,
                           supportmeshname,
                           &sentitytype,
                           &snbofnode,
                           &snbofcell,
                           &sgeotype,
                           &nbofconstantattribute,
                           &anyprofile,
                           &nbofvariableattribute) < 0)
    {
    vtkErrorMacro("Error in MEDstructElementInfo");
    return;
    }
  structelem->SetName(modelname);
  structelem->SetGeometryType(mgeotype);
  structelem->SetModelDimension(modeldim);
  structelem->SetSupportMeshName(supportmeshname);
  structelem->SetSupportEntityType(sentitytype);
  structelem->SetSupportNumberOfNode(snbofnode);
  structelem->SetSupportNumberOfCell(snbofcell);
  structelem->SetSupportGeometryType(sgeotype);
  structelem->AllocateNumberOfConstantAttribute(nbofconstantattribute);
  structelem->AllocateNumberOfVariableAttribute(nbofvariableattribute);
  structelem->SetAnyProfile(anyprofile);

  for(int attit = 0; attit < nbofconstantattribute; attit ++)
    {
    vtkMedConstantAttribute* constatt = structelem->GetConstantAttribute(attit);
    constatt->SetMedIterator(attit+1);
    constatt->SetParentStructElement(structelem);
    this->ReadConstantAttributeInformation(constatt);
    }

  for(int attit = 0; attit < nbofvariableattribute; attit ++)
    {
    vtkMedVariableAttribute* varatt = structelem->GetVariableAttribute(attit);
    varatt->SetMedIterator(attit+1);
    varatt->SetParentStructElement(structelem);
    this->ReadVariableAttributeInformation(varatt);
    }
}

void vtkMedDriver30::ReadConstantAttributeInformation(
    vtkMedConstantAttribute* constAttr)
{

  FileOpen open(this);

  char constattname[MED_NAME_SIZE+1] = "";
  med_attribute_type constatttype;
  med_int nbofcomponent;
  med_entity_type sentitytype;
  char profilename[MED_NAME_SIZE+1] = "";
  med_int profilesize;

  if(MEDstructElementConstAttInfo(
      this->FileId,
      constAttr->GetParentStructElement()->GetName(),
      constAttr->GetMedIterator(),
      constattname,
      &constatttype,
      &nbofcomponent,
      &sentitytype,
      profilename,
      &profilesize)   < 0)
    {
    vtkErrorMacro("MEDstructElementConstAttInfo error");
    return;
    }

  constAttr->SetName(constattname);
  constAttr->SetAttributeType(constatttype);
  constAttr->SetNumberOfComponent(nbofcomponent);
  constAttr->SetSupportEntityType(sentitytype);
  constAttr->SetProfileName(profilename);
  constAttr->SetProfileSize(profilesize);

  vtkAbstractArray* values = vtkMedUtilities::NewArray(constatttype);
  if(values == NULL)
    return;
  constAttr->SetValues(values);
  values->Delete();

  values->SetNumberOfComponents(nbofcomponent);
  vtkIdType ntuple = 0;
  if((strcmp(profilename, MED_NO_PROFILE) != 0) &&
     (strcmp(profilename, "\0") != 0))
    {
    ntuple = profilesize;
    }
  else if(constAttr->GetSupportEntityType() == MED_CELL)
    {
    ntuple = constAttr->GetParentStructElement()->GetSupportNumberOfCell();
    }
  else
    {
    ntuple = constAttr->GetParentStructElement()->GetSupportNumberOfNode();
    }
  values->SetNumberOfTuples(ntuple);

  void* ptr = NULL;
  vtkSmartPointer<vtkCharArray> buffer = vtkSmartPointer<vtkCharArray>::New();
  if(constatttype != MED_ATT_NAME)
    {
    ptr = values->GetVoidPointer(0);
    }
  else
    {
    buffer->SetNumberOfValues(MED_NAME_SIZE*nbofcomponent*ntuple);
    ptr = buffer->GetVoidPointer(0);
    }

  if(MEDstructElementConstAttRd (this->FileId,
        constAttr->GetParentStructElement()->GetName(),
        constAttr->GetName(), ptr) < 0)
    {
    vtkErrorMacro("MEDstructElementConstAttRd");
    return;
    }

  if(constatttype == MED_ATT_NAME)
    {
    char name[MED_NAME_SIZE+1] = "";
    char* nameptr = (char*) ptr;
    vtkStringArray* names = vtkStringArray::SafeDownCast(values);
    for(vtkIdType id = 0; id < nbofcomponent*ntuple; id++)
      {
      memset(name, '\0', MED_NAME_SIZE+1);
      strncpy(name, nameptr + id * MED_NAME_SIZE, MED_NAME_SIZE);
      names->SetValue(id, name);
      }
    }

  return;
}

void vtkMedDriver30::ReadVariableAttributeInformation(
    vtkMedVariableAttribute* varAttr)
{

  FileOpen open(this);

  char varattname[MED_NAME_SIZE+1] = "";
  med_attribute_type varatttype;
  med_int nbofcomponent;

  if(MEDstructElementVarAttInfo (
      this->FileId,
      varAttr->GetParentStructElement()->GetName(),
      varAttr->GetMedIterator(),
      varattname,
      &varatttype,
      &nbofcomponent) < 0)
    {
    vtkErrorMacro("MEDstructElementVarAttInfo");
    return;
    }

  varAttr->SetName(varattname);
  varAttr->SetAttributeType(varatttype);
  varAttr->SetNumberOfComponent(nbofcomponent);

  return;
}

void vtkMedDriver30::ReadProfileInformation(vtkMedProfile* profile)
{
  FileOpen open(this);

  med_int nelem;
  char profileName[MED_NAME_SIZE+1] = "";

  if(MEDprofileInfo(this->FileId,
                    profile->GetMedIterator(),
                    profileName,
                    &nelem) < 0)
    {
    vtkErrorMacro("cannot read information on profile"
        << profile->GetMedIterator());
    }
  profile->SetName(profileName);
  profile->SetNumberOfElement(nelem);
}

void vtkMedDriver30::ReadFieldInformation(vtkMedField* field)
{
  FileOpen open(this);

  if (field->GetMedIterator() == 0)
    return;

  int ncomp = MEDfieldnComponent(FileId, field->GetMedIterator());

  if(ncomp < 0)
    {
    field->SetNumberOfComponent(-1);
    vtkErrorMacro("cannot read the number of component of field "
        << field->GetMedIterator());
    return;
    }

  field->SetNumberOfComponent(ncomp);

  char* units = new char[MED_SNAME_SIZE * ncomp + 1];
  char* componentNames = new char[MED_SNAME_SIZE * ncomp + 1];
  memset(units, '\0', MED_SNAME_SIZE * ncomp + 1);
  memset(componentNames, '\0', MED_SNAME_SIZE * ncomp + 1);

  //med_type_champ dataType;
  med_field_type dataType;
  med_int nstep;
  med_bool localmesh;

  char name[MED_NAME_SIZE+1] = "";
  char meshName[MED_NAME_SIZE+1] = "";
  char unit[MED_SNAME_SIZE+1] = "";

  if( MEDfieldInfo( FileId,
                    field->GetMedIterator(),
                    name,
                    meshName,
                    &localmesh,
                    &dataType,
                    componentNames,
                    units,
                    unit,
                    &nstep) < 0)
    {
    vtkErrorMacro("cannot read the informations on field "
        << field->GetMedIterator());
    return;
    }

  field->SetName(name);
  field->SetMeshName(meshName);
  field->SetTimeUnit(unit);
  field->SetDataType(dataType);
  field->SetLocal(localmesh);

  for(int comp = 0; comp < ncomp; comp++)
    {
    char unit[MED_NAME_SIZE + 1] = "";
    memcpy(unit, units + MED_SNAME_SIZE * comp, MED_SNAME_SIZE * sizeof(char));
    field->GetUnit()->SetValue(comp, unit);

    char compName[MED_SNAME_SIZE + 1] = "";
    memcpy(compName, componentNames + MED_SNAME_SIZE * comp, MED_SNAME_SIZE
        * sizeof(char));
    field->GetComponentName()->SetValue(comp, compName);
    }

  delete[] units;
  delete[] componentNames;

  med_int ninterp = MEDfieldnInterp(FileId, field->GetName());
  if(ninterp < 0)
    {
    vtkErrorMacro("Error in MEDfieldnInterp");
    return;
    }

  field->AllocateNumberOfInterpolation(ninterp);

  for(med_int interpit=0; interpit<ninterp; interpit++)
    {
    vtkMedInterpolation* interp = field->GetInterpolation(interpit);
    interp->SetMedIterator(interpit + 1);
    this->ReadInterpolationInformation(interp);
    }

  vtkMedFieldStep* previousStep = NULL;

  for(med_int csit = 0; csit < nstep; csit++)
    {
    vtkMedFieldStep* step = vtkMedFieldStep::New();
    step->SetMedIterator(csit + 1);
    step->SetParentField(field);
    this->ReadFieldStepInformation(step, true);
    field->AddFieldStep(step);
    step->SetPreviousStep(previousStep);
    previousStep = step;
    step->Delete();
    }
}

void vtkMedDriver30::ReadFieldStepInformation(vtkMedFieldStep* step, bool readAllEntityInfo)
{
  vtkMedComputeStep cs;
  vtkMedComputeStep meshcs;
  vtkMedField* field = step->GetParentField();

  FileOpen open(this);

  if( MEDfieldComputingStepMeshInfo(
        FileId,
        field->GetName(),
        step->GetMedIterator(),
        &cs.TimeIt,
        &cs.IterationIt,
        &cs.TimeOrFrequency,
        &meshcs.TimeIt,
        &meshcs.IterationIt) < 0)
    {
    vtkErrorMacro("Error in MEDfieldComputingStepMeshInfo");
    return;
    }

  step->SetComputeStep(cs);
  step->SetMeshComputeStep(meshcs);

  if(!readAllEntityInfo || step->GetEntityInfoLoaded())
    return;

  step->SetEntityInfoLoaded(1);
  
  vtkMedFile* file = field->GetParentFile();
  vtkMedMesh* mesh = file->GetMesh(field->GetMeshName());
  
  if(mesh == NULL)
    return;
  
  std::set<vtkMedEntity> tmp_entities;
  std::set<vtkMedEntity> entities;
  mesh->GatherMedEntities(tmp_entities);
  
  std::set<vtkMedEntity>::iterator tmp_entity_it = tmp_entities.begin();
  while(tmp_entity_it != tmp_entities.end())
    {
    vtkMedEntity entity = *tmp_entity_it;
    tmp_entity_it++;
    entities.insert(entity);
    if(entity.EntityType == MED_CELL)
      {
      vtkMedEntity newEntity;
      newEntity.EntityType = MED_NODE_ELEMENT;
      newEntity.GeometryType = entity.GeometryType;
      newEntity.GeometryName = entity.GeometryName;
      entities.insert(newEntity);
      }
    }
  
  vtkMedEntity pointEntity;
  pointEntity.EntityType = MED_NODE;
  pointEntity.GeometryType = MED_NONE;
  pointEntity.GeometryName = MED_NAME_BLANK;
  entities.insert(pointEntity);

  std::set<vtkMedEntity>::iterator entity_it = entities.begin();
  while(entity_it != entities.end())
    {
    vtkMedEntity entity = *entity_it;
    entity_it++;

    med_int nvalues = 0;
    med_int nprofile;
    char profilename[MED_NAME_SIZE+1] = "";
    char localizationname[MED_NAME_SIZE+1] = "";

    nprofile = MEDfieldnProfile(
        this->FileId, 
        field->GetName(),
        step->GetComputeStep().TimeIt,
        step->GetComputeStep().IterationIt,
        entity.EntityType,
        entity.GeometryType,
        profilename,
        localizationname);
    if(nprofile < 0)
      {
      vtkErrorMacro("MEDfieldnProfile");
      continue;
      }

    bool hasprofile = (nprofile > 0);
    if(!hasprofile)
      {
      nprofile = 1;
      }

    med_int profilesize;
    med_int nintegrationpoint;
    
    for(int pid=0; pid<nprofile; pid++)
      {
      med_int medid = (hasprofile ? pid+1 : -1);
      nvalues = MEDfieldnValueWithProfile(
                  this->FileId, 
                  field->GetName(),
                  step->GetComputeStep().TimeIt,
                  step->GetComputeStep().IterationIt,
                  entity.EntityType,
                  entity.GeometryType,
                  medid,
                  MED_COMPACT_PFLMODE,
                  profilename,
                  &profilesize,
                  localizationname,
                  &nintegrationpoint);
            
      if(nvalues < 0)
        {
        vtkErrorMacro("MEDfieldnValueWithProfile");
        continue;
        }
      else if(nvalues > 0)
        {
        // I have found a profile with values, stop the loop here
        break;
        }
      }

    if(nvalues > 0)
      {
      vtkMedFieldOverEntity* fieldOverEntity = vtkMedFieldOverEntity::New();
      step->AppendFieldOverEntity(fieldOverEntity);
      fieldOverEntity->Delete();

      fieldOverEntity->SetParentStep(step);
      fieldOverEntity->SetEntity(entity);

      this->ReadFieldOverEntityInformation(fieldOverEntity);
      }
    }
}

void vtkMedDriver30::ReadFieldOverEntityInformation(vtkMedFieldOverEntity* fieldOverEntity)
{
  FileOpen open(this);

  vtkMedFieldStep* step = fieldOverEntity->GetParentStep();
  vtkMedField* field = step->GetParentField();
  vtkMedEntity entity = fieldOverEntity->GetEntity();

  const char* fieldName = field->GetName();
  const vtkMedComputeStep& cs = step->GetComputeStep();

  char profilename[MED_NAME_SIZE+1] = "";
  char localizationname[MED_NAME_SIZE+1] = "";

  med_int nProfiles = MEDfieldnProfile(
                        this->FileId,
                        fieldName,
                        cs.TimeIt,
                        cs.IterationIt,
                        entity.EntityType,
                        entity.GeometryType,
                        profilename,
                        localizationname);

  if(nProfiles < 0)
    {
    vtkErrorMacro("MEDfieldnProfile");
    }
  else if(nProfiles == 0)
    {
    fieldOverEntity->SetHasProfile(0);
    nProfiles = 1;
    }
  else
    {
    fieldOverEntity->SetHasProfile(1);
    }
  fieldOverEntity->AllocateNumberOfFieldOnProfile(nProfiles);
  for(int profit = 0; profit < nProfiles; profit++)
    {
    vtkMedFieldOnProfile* fop = fieldOverEntity->GetFieldOnProfile(profit);
    med_int medid = (fieldOverEntity->GetHasProfile()? profit+1: -1);
    fop->SetMedIterator(medid);
    fop->SetParentFieldOverEntity(fieldOverEntity);
    this->ReadFieldOnProfileInformation(fop);
    }
}

void vtkMedDriver30::ReadFieldOnProfileInformation(vtkMedFieldOnProfile* fop)
{
  vtkMedFieldOverEntity* fieldOverEntity = fop->GetParentFieldOverEntity();
  vtkMedFieldStep* step = fieldOverEntity->GetParentStep();
  vtkMedField* field = step->GetParentField();

  const vtkMedComputeStep& cs = step->GetComputeStep();
  med_int profilesize;
  med_int nbofintegrationpoint;

  char profileName[MED_NAME_SIZE+1] = "";
  char localizationName[MED_NAME_SIZE+1] = "";

  med_int nvalue = MEDfieldnValueWithProfile(FileId,
                    field->GetName(),
                    cs.TimeIt,
                    cs.IterationIt,
                    fieldOverEntity->GetEntity().EntityType,
                    fieldOverEntity->GetEntity().GeometryType,
                    fop->GetMedIterator(),
                    MED_COMPACT_STMODE,
                    profileName,
                    &profilesize,
                    localizationName,
                    &nbofintegrationpoint);

  if(nvalue < 0)
    {
    vtkErrorMacro("Error while reading MEDfieldnValueWithProfile");
    }

  fop->SetProfileName(profileName);
  fop->SetLocalizationName(localizationName);
  fop->SetNumberOfValues(nvalue);
  fop->SetNumberOfIntegrationPoint(nbofintegrationpoint);
  fop->SetProfileSize(profilesize);
}

void vtkMedDriver30::ReadMeshInformation(vtkMedMesh* mesh)
{
  FileOpen open(this);

  med_int mdim = 0;
  med_int sdim = 0;
  med_mesh_type meshtype;

  med_sorting_type sortingtype;
  med_int nstep = 0;
  med_axis_type axistype;
  med_int naxis;

  if ( (naxis=MEDmeshnAxis(this->FileId, mesh->GetMedIterator())) <0 )
    {
    vtkDebugMacro("Error reading mesh axis number");
    }

  if(naxis == 0)
    naxis=MEDmeshnAxis(this->FileId, mesh->GetMedIterator());

  char axisname[3*MED_SNAME_SIZE+1] = "";
  char axisunit[3*MED_SNAME_SIZE+1] = "";
  char name[MED_NAME_SIZE+1] = "";
  char description[MED_COMMENT_SIZE+1] = "";
  char timeUnit[MED_SNAME_SIZE+1] = "";

  if( MEDmeshInfo( this->FileId,
        mesh->GetMedIterator(),
        name,
        &sdim,
        &mdim,
        &meshtype,
        description,
        timeUnit,
        &sortingtype,
        &nstep,
        &axistype,
        axisname,
        axisunit ) )
    {
    vtkErrorMacro("Error reading mesh");
    }

  mesh->SetName(name);
  mesh->SetDescription(description);
  mesh->SetTimeUnit(timeUnit);
  mesh->SetSpaceDimension(sdim);
  mesh->SetMeshDimension(mdim);
  mesh->SetMeshType(meshtype);
  mesh->SetSortingType(sortingtype);
  mesh->SetAxisType(axistype);
  mesh->SetNumberOfAxis(naxis);

  for(int axis = 0; axis < naxis; axis++)
    {
    char theaxisname[MED_SNAME_SIZE+1] = "";
    char theaxisunit[MED_SNAME_SIZE+1] = "";
    strncpy(theaxisname, axisname + axis*MED_SNAME_SIZE, MED_SNAME_SIZE);
    strncpy(theaxisunit, axisunit + axis*MED_SNAME_SIZE, MED_SNAME_SIZE);
    mesh->GetAxisName()->SetValue(axis, theaxisname);
    mesh->GetAxisUnit()->SetValue(axis, theaxisunit);
    }

  char universalName[MED_LNAME_SIZE+1] = "";

  if(MEDmeshUniversalNameRd(this->FileId, name,
      universalName) < 0)
    {
    vtkDebugMacro("MEDmeshUniversalNameRd < 0");
    }
  mesh->SetUniversalName(universalName);

  // read the Information data of all families.
  // writing the family 0 is optional,
  // but I need it, so add it if it is not here.

  med_int nfam = MEDnFamily(this->FileId, name);

  for(int index = 0; index < nfam; index++)
    {
    vtkMedFamily* family = vtkMedFamily::New();
    family->SetMedIterator(index + 1);
    this->ReadFamilyInformation(mesh, family);
    family->Delete();
    }

  // this creates a family 0 if none has been read
  vtkMedFamily* familyZeroOnCell = mesh->GetOrCreateCellFamilyById(0);
  vtkMedFamily* familyZeroOnPoint = mesh->GetOrCreatePointFamilyById(0);

  // Load Information regarding the grid type
  if(meshtype == MED_STRUCTURED_MESH)
    {
    // Do it for structured data
    med_grid_type mtg;
    if(MEDmeshGridTypeRd(FileId, name, &mtg) < 0)
      {
      vtkErrorMacro("Error during structured grid Information loading.");
      return;
      }
    mesh->SetStructuredGridType(mtg);
    }

  vtkMedGrid* previousGrid = NULL;
  for(int gid=1; gid <= nstep; gid++)
    {
    vtkMedComputeStep cs;
    if(MEDmeshComputationStepInfo(FileId,
                                  name,
                                  gid,
                                  &cs.TimeIt,
                                  &cs.IterationIt,
                                  &cs.TimeOrFrequency) < 0)
      {
      vtkErrorMacro("MEDmeshComputationStepInfo error");
      }
    // Load Information regarding the grid type
    vtkMedGrid* grid = NULL;
    if(meshtype == MED_STRUCTURED_MESH)
      {
      switch(mesh->GetStructuredGridType())
        {
        case MED_CARTESIAN_GRID:
          grid = vtkMedCartesianGrid::New();
          break;
        case MED_POLAR_GRID:
          grid = vtkMedPolarGrid::New();
          break;
        case MED_CURVILINEAR_GRID:
          grid = vtkMedCurvilinearGrid::New();
          break;
        default:
          vtkErrorMacro("Unknown structured grid type " << mesh->GetStructuredGridType());
          return;
        }
      }
    else //(mesh->GetType() == MED_STRUCTURED_MESH)
      {
      grid = vtkMedUnstructuredGrid::New();
      }
    grid->SetParentMesh(mesh);
    grid->SetComputeStep(cs);
    this->ReadGridInformation(grid);
    mesh->AddGridStep(grid);
    grid->Delete();
    grid->SetPreviousGrid(previousGrid);
    previousGrid = grid;
    }
}

void vtkMedDriver30::ReadLocalizationInformation(vtkMedLocalization* loc)
{
  FileOpen open(this);

  med_int ngp;
  med_int spaceDimension;
  med_geometry_type type_geo;
  med_geometry_type sectiongeotype;
  med_int nsectionmeshcell;

  char name[MED_NAME_SIZE+1] = "";
  char interpolationName[MED_NAME_SIZE+1] = "";
  char sectionName[MED_NAME_SIZE+1] = "";

  if(MEDlocalizationInfo(
      this->FileId,
      loc->GetMedIterator(),
      name,
      &type_geo,
      &spaceDimension,
      &ngp,
      interpolationName,
      sectionName,
      &nsectionmeshcell,
      &sectiongeotype ) < 0)
    {
    vtkErrorMacro("Reading information on quadrature points definition : "
        << loc->GetMedIterator());
    }

  loc->SetName(name);
  loc->SetInterpolationName(interpolationName);
  loc->SetSectionName(sectionName);
  loc->SetNumberOfQuadraturePoint(ngp);
  loc->SetGeometryType(type_geo);
  loc->SetSpaceDimension(spaceDimension);
  loc->SetNumberOfCellInSection(nsectionmeshcell);
  loc->SetSectionGeometryType(sectiongeotype);

  med_float *localCoordinates = new med_float[loc->GetSizeOfPointLocalCoordinates()];
  med_float *pqLocalCoordinates = new med_float[loc->GetSizeOfQuadraturePointLocalCoordinates()];
  med_float *weights = new med_float[loc->GetSizeOfWeights()];

  if(MEDlocalizationRd(FileId,
      loc->GetName(),
      MED_FULL_INTERLACE,
      localCoordinates,
      pqLocalCoordinates,
      weights) < 0)
    {
    vtkErrorMacro("MEDlocalizationRd : " << loc->GetName());
    }

  vtkDoubleArray* lc = loc->GetPointLocalCoordinates();
  vtkDoubleArray *pqlc = loc->GetQuadraturePointLocalCoordinates();
  vtkDoubleArray *w = loc->GetWeights();

  lc->SetNumberOfValues(loc->GetSizeOfPointLocalCoordinates());
  for(int i=0; i<loc->GetSizeOfPointLocalCoordinates(); i++)
    {
    lc->SetValue(i, localCoordinates[i]);
    }

  pqlc->SetNumberOfValues(loc->GetSizeOfQuadraturePointLocalCoordinates());
  for(int i=0; i<loc->GetSizeOfQuadraturePointLocalCoordinates(); i++)
    {
    pqlc->SetValue(i, pqLocalCoordinates[i]);
    }

  w->SetNumberOfValues(loc->GetSizeOfWeights());
  for(int i=0; i<loc->GetSizeOfWeights(); i++)
    {
    w->SetValue(i, weights[i]);
    }
}

void vtkMedDriver30::ReadInterpolationInformation(vtkMedInterpolation* interp)
{

  med_geometry_type geotype;
  med_bool cellnode;
  med_int nbofbasisfunc;
  med_int nbofvariable;
  med_int maxdegree;
  med_int nmaxcoef;

  char name[MED_NAME_SIZE+1] = "";

  if(MEDinterpInfo (this->FileId,
                    interp->GetMedIterator(),
                    name,
                    &geotype, &cellnode, &nbofbasisfunc,
                    &nbofvariable, &maxdegree, &nmaxcoef) < 0)
    {
    vtkErrorMacro("MEDinterpInfo");
    return;
    }

  interp->SetName(name);
  interp->SetGeometryType(geotype);
  interp->SetIsCellNode(cellnode);
  interp->SetNumberOfVariable(nbofvariable);
  interp->SetMaximumDegree(maxdegree);
  interp->SetMaximumNumberOfCoefficient(nmaxcoef);
  interp->AllocateNumberOfBasisFunction(nbofbasisfunc);

  for(int basisid=0; basisid < nbofbasisfunc; basisid++)
    {
    vtkMedFraction* func = interp->GetBasisFunction(basisid);
    func->SetNumberOfVariable(nbofvariable);

    med_int ncoef = MEDinterpBaseFunctionCoefSize (
        this->FileId,
        interp->GetName(),
        basisid+1);
    func->SetNumberOfCoefficients(ncoef);

    if(ncoef <= 0 || nbofvariable <= 0)
      continue;

    med_int *power = new med_int[nbofvariable * ncoef];
    med_float *coefficient = new med_float[ncoef];

    if(MEDinterpBaseFunctionRd  (
        this->FileId,
        interp->GetName(),
        basisid+1,
        &ncoef,
        power,
        coefficient) < 0)
      {
      vtkErrorMacro("MEDinterpBaseFunctionRd");
      continue;
      }
    vtkDoubleArray* coeffs = func->GetCoefficients();
    for(int cid=0; cid < ncoef; cid++)
      {
      coeffs->SetValue(cid, coefficient[cid]);
      }
    vtkIntArray* powers = func->GetPowers();
    for(int pid=0; pid < ncoef*nbofvariable; pid++)
      {
      powers->SetValue(pid, power[pid]);
      }

    delete[] power;
    delete[] coefficient;
    }
}

void vtkMedDriver30::ReadSupportMeshInformation(
    vtkMedMesh* supportMesh)
{
  FileOpen open(this);

  char supportmeshname[MED_NAME_SIZE+1] = "";
  char description[MED_COMMENT_SIZE+1] = "";
  med_int spacedim;
  med_int meshdim;
  med_axis_type axistype;
  char axisname[3*MED_SNAME_SIZE+1] = "";
  char axisunit[3*MED_SNAME_SIZE+1] = "";

  if(MEDsupportMeshInfo (this->FileId,
                         supportMesh->GetMedIterator(),
                         supportmeshname,
                         &spacedim,
                         &meshdim,
                         description,
                         &axistype,
                         axisname,
                         axisunit) < 0)
    {
    vtkErrorMacro("MEDsupportMeshInfo");
    }

  supportMesh->SetName(supportmeshname);
  supportMesh->SetDescription(description);
  supportMesh->SetSpaceDimension(spacedim);
  supportMesh->SetMeshDimension(meshdim);
  supportMesh->SetAxisType(axistype);
  for(int dim = 0; dim < 3; dim++)
    {
    char axisname_dim[MED_SNAME_SIZE+1] = "";
    char axisunit_dim[MED_SNAME_SIZE+1] = "";

    strncpy(axisname_dim, axisname+dim*MED_SNAME_SIZE, MED_SNAME_SIZE);
    strncpy(axisunit_dim, axisunit+dim*MED_SNAME_SIZE, MED_SNAME_SIZE);

    supportMesh->GetAxisName()->SetValue(dim, axisname_dim);
    supportMesh->GetAxisUnit()->SetValue(dim, axisunit_dim);
    }

  return;
}

void vtkMedDriver30::LoadFamilyIds(vtkMedEntityArray* array)
{
  if(array->IsFamilyIdsLoaded())
    return;

  FileOpen open(this);

  vtkMedGrid* grid = array->GetParentGrid();

  vtkMedComputeStep cs = grid->GetComputeStep();

  // first, find if the family ids are implicit or explicit
  med_bool changement, transformation;

  med_int nfamid = MEDmeshnEntity(this->FileId,
                      grid->GetParentMesh()->GetName(),
                      cs.TimeIt,
                      cs.IterationIt,
                      array->GetEntity().EntityType,
                      array->GetEntity().GeometryType,
                      MED_FAMILY_NUMBER,
                      MED_NO_CMODE,
                      &changement,
                      &transformation);

  if(nfamid == array->GetNumberOfEntity())
    {

    vtkMedIntArray* famIds = vtkMedIntArray::New();
    array->SetFamilyIds(famIds);
    famIds->Delete();

    famIds->SetNumberOfTuples(nfamid);

    if ( MEDmeshEntityFamilyNumberRd(
            this->FileId,
            grid->GetParentMesh()->GetName(),
            cs.TimeIt,
            cs.IterationIt,
            array->GetEntity().EntityType,
            array->GetEntity().GeometryType,
            famIds->GetPointer(0) ) < 0)
      {
      vtkWarningMacro("Error loading the family ids of entity "
        << array->GetEntity().EntityType
        << " " << array->GetEntity().GeometryType
        << " on mesh " << grid->GetParentMesh()->GetName());
      array->SetFamilyIds(NULL);
      }
    }
  else
    {
    vtkDebugMacro("NumberOfEntity != Number of family ids");
    array->SetFamilyIds(NULL);
    }

  array->ComputeFamilies();
}

void vtkMedDriver30::LoadPointGlobalIds(vtkMedGrid* grid)
{
  if(grid->IsPointGlobalIdsLoaded())
    return;

  FileOpen open(this);

  vtkMedIntArray* globalIds = vtkMedIntArray::New();
  grid->SetPointGlobalIds(globalIds);
  globalIds->Delete();

  globalIds->SetNumberOfTuples(grid->GetNumberOfPoints());

  if( MEDmeshEntityNumberRd (
        this->FileId,
        grid->GetParentMesh()->GetName(),
        grid->GetComputeStep().TimeIt,
        grid->GetComputeStep().IterationIt,
        MED_NODE,
        MED_NONE,
        globalIds->GetPointer(0) ) < 0)
    {
    grid->SetPointGlobalIds(NULL);
    }
}

void vtkMedDriver30::LoadCoordinates(vtkMedGrid* grid)
{
  if(grid->IsCoordinatesLoaded())
    return;

  vtkMedRegularGrid* rgrid = vtkMedRegularGrid::SafeDownCast(grid);
  if(rgrid != NULL)
    {
    this->LoadRegularGridCoordinates(rgrid);
    return;
    }

  vtkMedUnstructuredGrid* ugrid = vtkMedUnstructuredGrid::SafeDownCast(grid);
  vtkMedCurvilinearGrid* cgrid = vtkMedCurvilinearGrid::SafeDownCast(grid);
  if(ugrid == NULL && cgrid == NULL)
    {
    //TODO : deal with structured grids
    vtkWarningMacro("this kind of grid is not yet supported");
    return;
    }

  if(grid->GetUsePreviousCoordinates())
    {
    vtkMedGrid* previousgrid = grid->GetPreviousGrid();
    if(previousgrid == NULL)
      {
      vtkErrorMacro("coordiantes have not changed, "
                    << "but there is no previous grid!");
      return;
      }

    this->LoadCoordinates(previousgrid);
    if(ugrid != NULL)
      ugrid->SetCoordinates(vtkMedUnstructuredGrid::SafeDownCast(previousgrid)
                            ->GetCoordinates());
    if(cgrid != NULL)
      cgrid->SetCoordinates(vtkMedCurvilinearGrid::SafeDownCast(previousgrid)
                            ->GetCoordinates());
    }
  else
    {

    FileOpen open(this);

    vtkDataArray* coords = vtkMedUtilities::NewCoordArray();
    if(ugrid != NULL)
      ugrid->SetCoordinates(coords);
    if(cgrid != NULL)
      cgrid->SetCoordinates(coords);
    coords->Delete();

    vtkMedComputeStep cs = grid->GetComputeStep();

    coords->SetNumberOfComponents(grid->GetParentMesh()->GetSpaceDimension());
    coords->SetNumberOfTuples(grid->GetNumberOfPoints());

    if ( MEDmeshNodeCoordinateRd( this->FileId,
                                  grid->GetParentMesh()->GetName(),
                                  cs.TimeIt,
                                  cs.IterationIt,
                                  MED_FULL_INTERLACE,
                                  (med_float*) coords->GetVoidPointer(0) ) < 0)
      {
      vtkErrorMacro("Load Coordinates for mesh "
          << grid->GetParentMesh()->GetName());
      }
    }
}

void vtkMedDriver30::LoadProfile(vtkMedProfile* profile)
{
  if(!profile || profile->IsLoaded())
    return;

  FileOpen open(this);

  vtkMedIntArray* indices = vtkMedIntArray::New();
  profile->SetIds(indices);
  indices->Delete();

  indices->SetNumberOfTuples(profile->GetNumberOfElement());

  char name[MED_NAME_SIZE+1] = "";

  if( MEDprofileRd(this->FileId,
                   profile->GetName(),
                   indices->GetPointer(0) ) < 0)
    {
    vtkErrorMacro("Reading profile indices ");
    }
}

void vtkMedDriver30::LoadConnectivity(vtkMedEntityArray* array)
{
  if(array->IsConnectivityLoaded())
    return;

  FileOpen open(this);

  vtkMedGrid* grid = array->GetParentGrid();

  grid = array->GetParentGrid();

  const char* meshName = grid->GetParentMesh()->GetName();

  vtkMedIntArray* conn = vtkMedIntArray::New();
  array->SetConnectivityArray(conn);
  conn->Delete();

  vtkMedComputeStep cs = grid->GetComputeStep();

  med_bool change;
  med_bool transformation;

  if(array->GetEntity().GeometryType == MED_POLYGON)
    {
    // first check if we have points
    med_int connSize = MEDmeshnEntity(
                            this->FileId,
                            meshName,
                            cs.TimeIt,
                            cs.IterationIt,
                            array->GetEntity().EntityType,
                            MED_POLYGON,
                            MED_CONNECTIVITY,
                            array->GetConnectivity(),
                            &change,
                            &transformation);

    if (connSize < 0)
      {
      vtkErrorMacro(<< "Error while reading polygons connectivity size."
                    << endl );
            return;
      }

    conn->SetNumberOfTuples(connSize);

    // How many polygon in the mesh in nodal connectivity mode
    // For the polygons, we get the size of array index
    med_int indexsize;
    if ((indexsize = MEDmeshnEntity(this->FileId,
                                    meshName,
                                    cs.TimeIt,
                                    cs.IterationIt,
                                    array->GetEntity().EntityType,
                                    MED_POLYGON,
                                    MED_INDEX_NODE,
                                    array->GetConnectivity(),
                                    &change,
                                    &transformation )) < 0)
      {
      vtkErrorMacro(<< "Error while reading polygons array index." << endl );
            return;
      }

    vtkMedIntArray* index = vtkMedIntArray::New();
    array->SetFaceIndex(index);
    index->Delete();

    index->SetNumberOfTuples( indexsize );

    if ( MEDmeshPolygonRd(this->FileId,
                          meshName,
                          cs.TimeIt,
                          cs.IterationIt,
                          array->GetEntity().EntityType,
                          array->GetConnectivity(),
                          index->GetPointer(0),
                          conn->GetPointer(0) ) < 0)
      {
      vtkErrorMacro(<< "MEDmeshPolygonRd");
      return;
      }
    }
  else if(array->GetEntity().GeometryType == MED_POLYHEDRON)
    {

    vtkIdType connSize = MEDmeshnEntity(this->FileId,
                                        meshName,
                                        grid->GetComputeStep().TimeIt,
                                        grid->GetComputeStep().IterationIt,
                                        array->GetEntity().EntityType,
                                        MED_POLYHEDRON,
                                        MED_CONNECTIVITY,
                                        array->GetConnectivity(),
                                        &change,
                                        &transformation);
    if (connSize < 0)
      {
      vtkErrorMacro(<< "Error while reading polyhedrons connectivity size."
                    << endl );
            return;
      }

    conn->SetNumberOfTuples(connSize);

    vtkMedIntArray* faceIndex = vtkMedIntArray::New();
    array->SetFaceIndex(faceIndex);
    faceIndex->Delete();

    vtkMedIntArray* nodeIndex = vtkMedIntArray::New();
    array->SetNodeIndex(nodeIndex);
    nodeIndex->Delete();

    vtkIdType np = array->GetNumberOfEntity() + 1;
    faceIndex->SetNumberOfTuples(np);

    med_int nodeIndexSize;

    if ((nodeIndexSize = MEDmeshnEntity(this->FileId,
                                        meshName,
                                        grid->GetComputeStep().TimeIt,
                                        grid->GetComputeStep().IterationIt,
                                        array->GetEntity().EntityType,
                                        MED_POLYHEDRON,
                                        MED_INDEX_NODE,
                                        array->GetConnectivity(),
                                        &change,
                                        &transformation )) < 0)
      {
      vtkErrorMacro(<< "Error while reading polygons array index." << endl );
            return;
      }

    nodeIndex->SetNumberOfTuples(nodeIndexSize);

    if (MEDmeshPolyhedronRd(this->FileId,
                            meshName,
                            cs.TimeIt,
                            cs.IterationIt,
                            array->GetEntity().EntityType,
                            array->GetConnectivity(),
                            faceIndex->GetPointer(0),
                            nodeIndex->GetPointer(0),
                            conn->GetPointer(0) ) < 0)
      {
      vtkErrorMacro("Error while reading connectivity of polyhedrons");
      return;
      }

    }
  else
    {
    bool doReadConnectivity = true;
    if(array->GetConnectivity() == MED_NODAL)
      {
      if(array->GetEntity().EntityType == MED_STRUCT_ELEMENT)
        {
        if(array->GetStructElement() == NULL)
          {
          vtkErrorMacro("Entity type = MED_STRUCT_ELEMENT, but StructElement is not set!");
          return;
          }
        vtkIdType ntuple = array->GetNumberOfEntity()
                           * array->GetStructElement()->GetConnectivitySize();

        conn->SetNumberOfTuples(ntuple);
        // particles are special : connectivity is not stored in the med file
        if(strcmp(array->GetStructElement()->GetName(), MED_PARTICLE_NAME) == 0 )
          {
          for(vtkIdType cellId = 0; cellId < ntuple; cellId++)
            {
            conn->SetValue(cellId, cellId+1);
            }
          doReadConnectivity = false;
          }
        }
      else
        {
        conn->SetNumberOfTuples(array->GetNumberOfEntity()
            * vtkMedUtilities::GetNumberOfPoint(
                array->GetEntity().GeometryType));
        }
      }
    else
      {
      conn->SetNumberOfTuples(array->GetNumberOfEntity()
          * vtkMedUtilities::GetNumberOfSubEntity(
              array->GetEntity().GeometryType));
      }

    if  (this->ParallelFileId == -1) // also (array->GetFilter() == NULL)
      {
      if ( (MEDmeshElementConnectivityRd(
            this->FileId,
            meshName,
            cs.TimeIt,
            cs.IterationIt,
            array->GetEntity().EntityType,
            array->GetEntity().GeometryType,
            array->GetConnectivity(),
            MED_FULL_INTERLACE,
            conn->GetPointer(0)) ) < 0)
        {
        vtkErrorMacro("Error while load connectivity of cells "
            << array->GetEntity().GeometryType);
        }
      }
    else
      {
      med_filter filter = MED_FILTER_INIT;

      int    start;
      int    stride;
      int    count;
      int    blocksize;
      int    lastblocksize;
      array->GetFilter()->GetFilterSizes(start, stride, count,
                                   blocksize, lastblocksize );

      med_int nbofconstituentpervalue = vtkMedUtilities::GetNumberOfNodes(
                                        array->GetEntity().GeometryType);

      if ( MEDfilterBlockOfEntityCr( this->ParallelFileId,
              array->GetNumberOfEntity(),
            1, // one is for mesh elements, more than 1 is for fields
              nbofconstituentpervalue,
            MED_ALL_CONSTITUENT,
            MED_FULL_INTERLACE,
            MED_COMPACT_STMODE,
            MED_NO_PROFILE,
            (med_size)start,
            (med_size)stride,
            (med_size)count,
            (med_size)blocksize,
            (med_size)lastblocksize,
            &filter ) < 0 )
        {
        vtkErrorMacro("Filter creation ");
        }

        if ( (MEDmeshElementConnectivityAdvancedRd(
              this->ParallelFileId,
              meshName,
              cs.TimeIt,
              cs.IterationIt,
              array->GetEntity().EntityType,
              array->GetEntity().GeometryType,
              array->GetConnectivity(),
              &filter,
              conn->GetPointer(0)) ) < 0)
          {
          vtkErrorMacro("Error while load connectivity of cells "
              << array->GetEntity().GeometryType);
          }

      if ( MEDfilterClose( &filter ) < 0)
          {
        vtkErrorMacro("ERROR : filter closing ...");
          }

      }
    }
}

void vtkMedDriver30::LoadCellGlobalIds(vtkMedEntityArray* array)
{
  if(array->IsGlobalIdsLoaded())
    return;

  FileOpen open(this);

  vtkMedIntArray* globalIds = vtkMedIntArray::New();
  array->SetGlobalIds(globalIds);
  globalIds->Delete();

  globalIds->SetNumberOfTuples(array->GetNumberOfEntity());

  vtkMedGrid* grid = array->GetParentGrid();
  vtkMedComputeStep cs = grid->GetComputeStep();

  if( MEDmeshEntityNumberRd (
        this->FileId,
        grid->GetParentMesh()->GetName(),
        cs.TimeIt,
        cs.IterationIt,
        array->GetEntity().EntityType,
        array->GetEntity().GeometryType,
        globalIds->GetPointer(0) ) < 0)
    {
    array->SetGlobalIds(NULL);
    }
}

void vtkMedDriver30::LoadField(vtkMedFieldOnProfile* fop, med_storage_mode mode)
{
  FileOpen open(this);

  vtkMedFieldOverEntity* fieldOverEntity = fop->GetParentFieldOverEntity();
  vtkMedFieldStep *step = fieldOverEntity->GetParentStep();
  vtkMedField* field = step->GetParentField();
  const vtkMedComputeStep& cs = step->GetComputeStep();

  vtkDataArray* data = vtkMedUtilities::NewArray(field->GetDataType());
  fop->SetData(data);
  data->Delete();

  med_int size;
  if(mode == MED_COMPACT_STMODE)
    {
    size = fop->GetNumberOfValues();
    }
  else
    {
    med_int profilesize;
    med_int nbofintegrationpoint;
    char profileName[MED_NAME_SIZE+1] = "";
    char localizationName[MED_NAME_SIZE+1] = "";
    size = MEDfieldnValueWithProfile(this->FileId,
                field->GetName(),
                cs.TimeIt,
                cs.IterationIt,
                fieldOverEntity->GetEntity().EntityType,
                fieldOverEntity->GetEntity().GeometryType,
                fop->GetMedIterator(),
                MED_GLOBAL_STMODE,
                profileName,
                &profilesize,
                localizationName,
                &nbofintegrationpoint);
    }

  if(fop->GetNumberOfIntegrationPoint() > 1)
    {
    size *= fop->GetNumberOfIntegrationPoint();
    }

  data->SetNumberOfComponents(field->GetNumberOfComponent());
  data->SetNumberOfTuples(size);
  if  (this->ParallelFileId == -1)
    {
    if ( MEDfieldValueWithProfileRd(
          this->FileId,
          field->GetName(),
          cs.TimeIt,
          cs.IterationIt,
          fieldOverEntity->GetEntity().EntityType,
          fieldOverEntity->GetEntity().GeometryType,
          mode,
          fop->GetProfileName(),
          MED_FULL_INTERLACE,
          MED_ALL_CONSTITUENT,
          (unsigned char*) data->GetVoidPointer(0) ) < 0)
      {
      vtkErrorMacro("Error on MEDfieldValueWithProfileRd");
      }
    }
  else
    {
  if  (field->GetFieldType() == vtkMedField::CellField)
    {
    med_filter filter = MED_FILTER_INIT;

    int    start;
    int    stride;
    int    count;
    int    blocksize;
    int    lastblocksize;
    fop->GetFilter()->GetFilterSizes(start, stride, count,
                                 blocksize, lastblocksize );

    if ( MEDfilterBlockOfEntityCr( this->ParallelFileId,
        fop->GetNumberOfValues(),
          1, // one is for mesh elements, more than 1 is for fields
          field->GetNumberOfComponent(),
          MED_ALL_CONSTITUENT,
          MED_FULL_INTERLACE,
          MED_COMPACT_STMODE,
          MED_NO_PROFILE,
          (med_size)start,
          (med_size)stride,
          (med_size)count,
          (med_size)blocksize,
          (med_size)lastblocksize,
          &filter ) < 0 )
      {
      vtkErrorMacro("Filter creation ");
      }

    if ( MEDfieldValueAdvancedRd(
            this->ParallelFileId,
            field->GetName(),
            cs.TimeIt,
            cs.IterationIt,
            fieldOverEntity->GetEntity().EntityType,
            fieldOverEntity->GetEntity().GeometryType,
            &filter,
            (unsigned char*) data->GetVoidPointer(0) ) < 0)
        {
        vtkErrorMacro("Error on MEDfieldValueAdvancedRd");
        }

    if ( MEDfilterClose( &filter ) < 0)
        {
      vtkErrorMacro("ERROR : filter closing ...");
        }
      }
  else
    {//TODO : option utilisateur pour desactiver ou non les champs avec profile en //
    if ( MEDfieldValueWithProfileRd(
              this->FileId,
              field->GetName(),
              cs.TimeIt,
              cs.IterationIt,
              fieldOverEntity->GetEntity().EntityType,
              fieldOverEntity->GetEntity().GeometryType,
              mode,
              fop->GetProfileName(),
              MED_FULL_INTERLACE,
              MED_ALL_CONSTITUENT,
              (unsigned char*) data->GetVoidPointer(0) ) < 0)
          {
          vtkErrorMacro("Error on MEDfieldValueWithProfileRd");
          }
    }
    }
}

void vtkMedDriver30::LoadVariableAttribute(vtkMedVariableAttribute* varatt,
                                           vtkMedEntityArray* array)
{
  FileOpen open(this);

  void  *value = NULL;

  vtkAbstractArray* valuearray = array->GetVariableAttributeValue(varatt);
  // first test if this is already loaded
  if(valuearray != NULL && valuearray->GetNumberOfTuples() > 0)
    return;

  if(valuearray == NULL)
    {
    valuearray = vtkMedUtilities::NewArray(varatt->GetAttributeType());
    array->SetVariableAttributeValues(varatt, valuearray);
    valuearray->Delete();
    }

  valuearray->SetNumberOfComponents(varatt->GetNumberOfComponent());
  valuearray->SetNumberOfTuples(array->GetNumberOfEntity());
  valuearray->SetName(varatt->GetName());

  vtkSmartPointer<vtkCharArray> chararray = vtkSmartPointer<vtkCharArray>::New();

  if(varatt->GetAttributeType() != MED_ATT_NAME)
    {
    value = valuearray->GetVoidPointer(0);
    }
  else
    {
    chararray->SetNumberOfValues(varatt->GetNumberOfComponent() *
                                  array->GetNumberOfEntity() *
                                  MED_NAME_SIZE);

    value = chararray->GetVoidPointer(0);
    }

  vtkMedComputeStep cs = array->GetParentGrid()->GetComputeStep();

  if(MEDmeshStructElementVarAttRd(
      this->FileId,
      array->GetParentGrid()->GetParentMesh()->GetName(),
      cs.TimeIt,
      cs.IterationIt,
      varatt->GetParentStructElement()->GetGeometryType(),
      varatt->GetName(),
      value) < 0)
    {

    if(cs.IterationIt == MED_NO_IT && cs.TimeIt == MED_NO_DT && cs.TimeOrFrequency == MED_UNDEF_DT)
      {
      vtkErrorMacro("MEDmeshStructElementVarAttRd");
      return;
      }
    // try to see if I can reuse
    // the variable attributes of the NO_DT, NO_IT compute step
    vtkMedComputeStep nocs;
    nocs.IterationIt = MED_NO_IT;
    nocs.TimeIt = MED_NO_DT;
    nocs.TimeOrFrequency = MED_UNDEF_DT;
    vtkMedEntityArray* nocs_array =
        array->GetParentGrid()->GetParentMesh()->GetGridStep(nocs)->GetEntityArray(array->GetEntity());
    if(nocs_array == NULL)
      {
      nocs_array = array->GetParentGrid()->GetParentMesh()->GetGridStep(0)->GetEntityArray(array->GetEntity());
      }

    if(nocs_array == NULL || nocs_array == array)
      {
      // try to force load the default compute step.
      if(MEDmeshStructElementVarAttRd(
          this->FileId,
          array->GetParentGrid()->GetParentMesh()->GetName(),
          nocs.TimeIt,
          nocs.IterationIt,
          varatt->GetParentStructElement()->GetGeometryType(),
          varatt->GetName(),
          value) < 0)
        {
        vtkErrorMacro("MEDmeshStructElementVarAttRd");
        return;
        }
      }
    else
      {
      this->LoadVariableAttribute(varatt, nocs_array);
      array->SetVariableAttributeValues(varatt, nocs_array->GetVariableAttributeValue(varatt));
      return;
      }
    }

  // If I am here, it means that I read the values
  if(varatt->GetAttributeType() == MED_ATT_NAME)
    {
    char current_name[MED_NAME_SIZE+1] = "";
    vtkStringArray* sarray = vtkStringArray::SafeDownCast(valuearray);
    for(vtkIdType id = 0; id < varatt->GetNumberOfComponent() *
                       array->GetNumberOfEntity(); id++)
      {
      memset(current_name, '\0', MED_NAME_SIZE+1);
      strncpy(current_name, ((char*)value) + id*MED_NAME_SIZE, MED_NAME_SIZE);
      sarray->SetValue(id, current_name);
      }
    }

  return;
}

void vtkMedDriver30::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
