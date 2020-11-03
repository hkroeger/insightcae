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

#include "vtkMedFamilyOnEntityOnProfile.h"

#include "vtkObjectFactory.h"
#include "vtkMedUtilities.h"
#include "vtkMedFamilyOnEntity.h"
#include "vtkMedProfile.h"
#include "vtkMedEntityArray.h"
#include "vtkMedFamily.h"
#include "vtkMedField.h"
#include "vtkMedGrid.h"
#include "vtkMedMesh.h"
#include "vtkMedFieldOnProfile.h"
#include "vtkMedFieldOverEntity.h"
#include "vtkMedFieldStep.h"
#include "vtkMedFile.h"
#include "vtkMedGrid.h"
#include "vtkMedDriver.h"
#include "vtkMedUnstructuredGrid.h"
#include "vtkMedIntArray.h"

#include "vtkBitArray.h"
#include "vtkIdList.h"

#include "vtkMultiProcessController.h"

vtkCxxSetObjectMacro(vtkMedFamilyOnEntityOnProfile,FamilyOnEntity, vtkMedFamilyOnEntity);
vtkCxxSetObjectMacro(vtkMedFamilyOnEntityOnProfile, Profile, vtkMedProfile);

vtkStandardNewMacro(vtkMedFamilyOnEntityOnProfile)

vtkMedFamilyOnEntityOnProfile::vtkMedFamilyOnEntityOnProfile()
{
  this->FamilyOnEntity = NULL;
  this->Profile = NULL;
  this->IntersectionStatus = vtkMedFamilyOnEntityOnProfile::NotComputed;
  this->UseAllPoints = false;
  this->MatchComputed = false;
  this->Valid = true;
}

vtkMedFamilyOnEntityOnProfile::~vtkMedFamilyOnEntityOnProfile()
{
  this->SetFamilyOnEntity(NULL);
  this->SetProfile(NULL);
}

bool vtkMedFamilyOnEntityOnProfile::KeepPoint(med_int index)
{
  if(this->IntersectionStatus == NotComputed)
    this->ComputeIntersection(NULL);

  if(this->UseAllPoints)
    return true;

  if(this->MedToVTKPointIndexMap.find(index)
    == this->MedToVTKPointIndexMap.end())
    return false;

  return true;
}

bool vtkMedFamilyOnEntityOnProfile::KeepCell(med_int index)
{
  if(this->FamilyOnEntity->GetEntityArray()->GetFamilyId(index)
    != this->FamilyOnEntity->GetFamily()->GetId())
    return false;
  return true;
}

int vtkMedFamilyOnEntityOnProfile::CanMapField(vtkMedFieldOnProfile* fop)
{
  // only point fields can be mapped on point supports.
  if(this->GetFamilyOnEntity()->GetEntity().EntityType == MED_NODE &&
     fop->GetParentFieldOverEntity()->GetEntity().EntityType != MED_NODE)
    return false;

  // if it is a cell-centered field, the geometry need to be the same
  if(fop->GetParentFieldOverEntity()->GetEntity().EntityType != MED_NODE
     && fop->GetParentFieldOverEntity()->GetEntity().GeometryType !=
     this->GetFamilyOnEntity()->GetEntity().GeometryType)
    return false;

  int numProc = 1;
  vtkMultiProcessController* controller =
		  vtkMultiProcessController::GetGlobalController();
  if (controller != NULL)
    {
    numProc = controller->GetNumberOfProcesses();
    }

  if ((this->GetValid() == 0) && numProc == 1)
    return false;

  this->ComputeIntersection(fop);

  if(this->IntersectionStatus == vtkMedFamilyOnEntityOnProfile::NoIntersection)
    return false;

  if(fop != NULL &&
     this->GetFamilyOnEntity()->GetEntity().EntityType != MED_NODE &&
     fop->GetParentFieldOverEntity()->GetEntity().EntityType == MED_NODE &&
     this->PointProfileMatch[fop->GetProfile()] == BadOrNoIntersection)
    return false;

  return true;
}

int vtkMedFamilyOnEntityOnProfile::CanShallowCopy(vtkMedFieldOnProfile *fop)
{
  if(fop == NULL)
    {
    bool shallow_on_points = this->CanShallowCopyPointField(NULL);
    bool shallow_on_cells = this->CanShallowCopyCellField(NULL);
    if(shallow_on_points && shallow_on_cells)
      return true;
    if(!shallow_on_points && !shallow_on_cells)
      return false;
    vtkErrorMacro("CanShallowCopy cannot answer : is it a point or a cell field?");
    return false;
    }

  if(fop->GetParentFieldOverEntity()->GetParentStep()->GetParentField()
    ->GetFieldType() == vtkMedField::PointField)
    return this->CanShallowCopyPointField(fop);
  else
    return this->CanShallowCopyCellField(fop);
}

void vtkMedFamilyOnEntityOnProfile::ComputeIntersection(vtkMedFieldOnProfile* fop)
{
  int nodeOrCellSupport=this->GetFamilyOnEntity()->GetPointOrCell();
  int fieldType;
  if(fop)
    {
    fieldType = fop->GetParentFieldOverEntity()->GetParentStep()->
                  GetParentField()->GetFieldType();
    }
  else
    {
    fieldType = (nodeOrCellSupport==vtkMedUtilities::OnPoint?vtkMedField::PointField:vtkMedField::CellField);
    }
  // Cell fields cannot match point supports
  if(fieldType != vtkMedField::PointField
     && nodeOrCellSupport == vtkMedUtilities::OnPoint)
    {
    this->IntersectionStatus = vtkMedFamilyOnEntityOnProfile::NoIntersection;
    this->UseAllPoints = false;
    }
  else if(fieldType != vtkMedField::PointField
     && nodeOrCellSupport ==vtkMedUtilities::OnCell)
    {
    this->ComputeCellFamilyVsCellProfileMatch();
    }
  else if(fieldType == vtkMedField::PointField
     && nodeOrCellSupport ==vtkMedUtilities::OnPoint)
    {
    vtkMedProfile* profile = NULL;
    if(fop != NULL)
      {
      profile = fop->GetProfile();
      }
    // point fields must share the same profile as the point support.
    this->ComputePointFamilyVsPointProfileMatch();

    }
  else if(fieldType == vtkMedField::PointField
     && nodeOrCellSupport == vtkMedUtilities::OnCell)
    {
    vtkMedProfile* profile = NULL;
    if(fop != NULL)
      {
      profile = fop->GetProfile();
      }
    this->ComputeCellFamilyVsPointProfileMatch(profile);
    }

  this->MatchComputed = true;
}

int vtkMedFamilyOnEntityOnProfile::CanShallowCopyPointField(vtkMedFieldOnProfile* fop)
{
  vtkMedProfile* profile = (fop != NULL?fop->GetProfile(): NULL);
  if(this->FamilyOnEntity->GetPointOrCell() == vtkMedUtilities::OnCell)
    {
    if(this->PointProfileMatch.find(profile) == this->PointProfileMatch.end())
      {
      this->ComputeCellFamilyVsPointProfileMatch(profile);
      }
    int match = this->PointProfileMatch[profile];
    return match
        == vtkMedFamilyOnEntityOnProfile::ProfileEqualsSupport;
    }
  else
    {
    // this is a point support.
    // The only case when there is shallow copy is if there is at most 1 family
    // and the profile is shared.
    if(this->Profile == profile &&
       this->GetFamilyOnEntity()->GetEntityArray()
        ->GetNumberOfFamilyOnEntity() <= 1)
      {
      return true;
      }
    else
      {
      return false;
      }
    }
}

int vtkMedFamilyOnEntityOnProfile::CanShallowCopyCellField(vtkMedFieldOnProfile* fop)
{
  vtkMedProfile* profile = (fop != NULL?fop->GetProfile(): NULL);
  // cell fields cannot be mapped to cell supports
  if(this->FamilyOnEntity->GetPointOrCell() == vtkMedUtilities::OnPoint)
    {
    return false;
    }

  // this is a cell support.
  if(this->Profile != profile)
    {
    return false;
    }

  // the only case I can shallow copy is if there is only one family
  // defined on those cells.
  if(this->Profile == NULL &&
     this->GetFamilyOnEntity()->GetEntityArray()
    ->GetNumberOfFamilyOnEntity() <= 1)
    return true;

  return false;
}

void  vtkMedFamilyOnEntityOnProfile::ComputeUsedPoints()
{
  this->MedToVTKPointIndexMap.clear();

  //first test a few special cases where no heavy computing is necessary
  vtkMedGrid* grid = this->FamilyOnEntity->GetParentGrid();
  if(this->Profile == NULL)
    {
    // If there is no profile, the entity is on points and there
    // at most 1 point family, then all points are used.
    if(this->FamilyOnEntity->GetPointOrCell() == vtkMedUtilities::OnPoint &&
       this->FamilyOnEntity->GetEntityArray()->GetNumberOfFamilyOnEntity() <= 1)
      {
      this->UseAllPoints = true;
      return;
      }
    // if there is no profile, the entity is on cell
    // and there is at most 1 family on his entity, then all points are used
    if(this->FamilyOnEntity->GetPointOrCell() == vtkMedUtilities::OnCell &&
       this->FamilyOnEntity->GetEntityArray()->GetNumberOfFamilyOnEntity() <= 1)
      {
      this->UseAllPoints = true;
      return;
      }
    }

  vtkSmartPointer<vtkBitArray> flag = vtkSmartPointer<vtkBitArray>::New();
  flag->SetNumberOfTuples(grid->GetNumberOfPoints());

  // initialize the array to false
  for(vtkIdType pid = 0; pid < flag->GetNumberOfTuples(); pid++)
    flag->SetValue(pid, false);

  // for each cell, flag the used points
  if(this->Profile)
    this->Profile->Load();

  vtkMedIntArray* pids=(this->Profile!=NULL?this->Profile->GetIds():NULL);

  med_int famId = this->FamilyOnEntity->GetFamily()->GetId();
  vtkMedEntityArray* array = this->FamilyOnEntity->GetEntityArray();
  vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();

  array->LoadConnectivity();

  vtkIdType pflsize = (pids != NULL ? pids->GetNumberOfTuples():array->GetNumberOfEntity());
  for(vtkIdType pindex=0; pindex<pflsize; pindex++)
    {
    med_int pid = (pids != NULL ? pids->GetValue(pindex)-1 : pindex);
    med_int fid = array->GetFamilyId(pid);
    if(famId==fid)
      {
      // this cell is of the family and on the profile.
      // --> flag all vertices of this cell
      array->GetCellVertices(pid, ids);
      for(int id = 0; id<ids->GetNumberOfIds(); id++)
        {
        vtkIdType subid = ids->GetId(id);
        if(subid < 0 || subid >= flag->GetNumberOfTuples())
          {
          vtkDebugMacro("invalid sub id : " << subid);
          this->SetValid(0);
          break;
          }
        flag->SetValue(subid, 1);
        }
      }
    }

  // now, the flag array contains all vertices used by this support
  this->UseAllPoints = true;
  for(vtkIdType pid = 0; pid<flag->GetNumberOfTuples(); pid++)
    {
    if(flag->GetValue(pid) == false)
      {
      this->UseAllPoints = false;
      break;
      }
    }

  if(!this->UseAllPoints)
    {
    // If all points are not used, I compute the index mapping
    vtkIdType vtk_index = 0;

    for(vtkIdType pid=0; pid < flag->GetNumberOfTuples(); pid++)
      {
      if(flag->GetValue(pid) == true)
        {
        this->MedToVTKPointIndexMap[pid] = vtk_index;
        vtk_index++;
        }
      }
    }
}

void vtkMedFamilyOnEntityOnProfile::ComputeCellFamilyVsCellProfileMatch()
{
  if(this->MatchComputed)
    return;

  // this computes the UseAllPoints flag.
  this->ComputeUsedPoints();

  if(this->Profile == NULL)
    {
    // If there is no profile, then the match is exact if and only
    // if there is 1 cell family on this entity
    if(this->FamilyOnEntity->GetEntityArray()->GetNumberOfFamilyOnEntity() == 1)
      {
      this->IntersectionStatus =
          vtkMedFamilyOnEntityOnProfile::ProfileIncludedInFamily;
      }
    else
      {
      this->IntersectionStatus =
          vtkMedFamilyOnEntityOnProfile::ProfileIntersectsFamily;
      }
    return;
    }

  this->Profile->Load();
  vtkMedIntArray* pids=this->Profile->GetIds();

  if(pids==NULL)
    {
    vtkErrorMacro("Could not load profile indices!");
    this->IntersectionStatus = vtkMedFamilyOnEntityOnProfile::NotComputed;
    this->UseAllPoints = false;
    return;
    }

  med_int famId = this->GetFamilyOnEntity()->GetFamily()->GetId();
  vtkIdType pindex=0;
  vtkMedEntityArray* array=this->FamilyOnEntity->GetEntityArray();
  bool profile_included = true;
  bool profile_intersect = false;
  for(int pindex=0; pindex<pids->GetNumberOfTuples(); pindex++)
    {
    med_int pid=pids->GetValue(pindex)-1;
    med_int fid=array->GetFamilyId(pid);
    if(famId==fid)
      {// the current cell is on the familyand on the profile
      // --> there is an overlap
      profile_intersect = true;
      }
    else
      {
      // the cell is on the profile but not on the family --> no inclusion
      profile_included=false;
      }
    }

  if(profile_included && profile_intersect)
    this->IntersectionStatus = vtkMedFamilyOnEntityOnProfile::ProfileIncludedInFamily;
  else if(profile_intersect)
    this->IntersectionStatus = vtkMedFamilyOnEntityOnProfile::ProfileIntersectsFamily;
  else
    this->IntersectionStatus = vtkMedFamilyOnEntityOnProfile::NoIntersection;
}

void vtkMedFamilyOnEntityOnProfile::
    ComputeCellFamilyVsPointProfileMatch(vtkMedProfile* profile)
{
  // first test if the cache is already set
  if(this->PointProfileMatch.find(profile) != this->PointProfileMatch.end())
    return;

  // this will compute the cell match cache, as well as the UseAllPoints flag.
  this->ComputeCellFamilyVsCellProfileMatch();

  if(profile == NULL)
    {
    // If there is no profile, then the match is at least partial.
    // It is exact if and only
    // if the cell family uses all points
    int match =  (this->UseAllPoints? ProfileEqualsSupport : ProfileLargerThanSupport);
    this->PointProfileMatch[profile] =
        (this->UseAllPoints? ProfileEqualsSupport : ProfileLargerThanSupport);
    return;
    }

  // if profile is not NULL and I use all points --> BadOrNoIntersection
  if(this->UseAllPoints)
    {
    this->PointProfileMatch[profile] = BadOrNoIntersection;
    }

  profile->Load();

  vtkMedIntArray* pids=profile->GetIds();

  if(pids == NULL)
    {
    vtkErrorMacro("profile indices could not be loaded!");
    this->PointProfileMatch[profile] = BadOrNoIntersection;
    return;
    }

  med_int pindex = 0;
  bool exact_match = true;
  vtkIdType numberOfUsedPoints = pids->GetNumberOfTuples();
  for(med_int pindex=0; pindex < pids->GetNumberOfTuples(); pindex++)
    {
    med_int id = pids->GetValue(pindex);
    if(this->MedToVTKPointIndexMap.find(id-1) == this->MedToVTKPointIndexMap.end())
      {
      // The given point profile index is not used by this support.
      // the superposition is at most partial.
      exact_match = false;
      numberOfUsedPoints--;
      }
    }

  // if this profile is smaller than the number of points, I can't match
  // the profile to this support
  if(numberOfUsedPoints < this->MedToVTKPointIndexMap.size())
    {
    this->PointProfileMatch[profile] = BadOrNoIntersection;
    }
  else if(exact_match)
    {
    this->PointProfileMatch[profile] = ProfileEqualsSupport;
    }
  else
    {
    this->PointProfileMatch[profile] = ProfileLargerThanSupport;
    }
}

void  vtkMedFamilyOnEntityOnProfile::ComputePointFamilyVsPointProfileMatch()
{
  if(this->MatchComputed)
    return;

  this->ComputeUsedPoints();

  if(this->Profile == NULL)
    {
    // If there is no profile, then the match is exact if there is at most
    // 1 point family on the grid
    if(this->FamilyOnEntity->GetParentGrid()->GetParentMesh()
      ->GetNumberOfPointFamily() <= 1)
      {
      this->IntersectionStatus = ProfileIncludedInFamily;
      }
    }

  // there is a profile, we have to compute the match between the family and
  // the profile
  vtkMedFamilyOnEntity* foe = this->GetFamilyOnEntity();
  vtkMedEntityArray* pea = foe->GetEntityArray();
  vtkMedIntArray* pIds = NULL;

  if(this->Profile)
    {
    this->Profile->Load();
    pIds=this->Profile->GetIds();
    }

  if(pIds == NULL)
    {
    if(this->FamilyOnEntity->GetEntityArray()->GetNumberOfFamilyOnEntity() > 1)
      {
      this->IntersectionStatus =
          vtkMedFamilyOnEntityOnProfile::ProfileIntersectsFamily;
      }
    else
      {
      this->IntersectionStatus =
          vtkMedFamilyOnEntityOnProfile::ProfileIncludedInFamily;
      }
    return;
    }

  bool profile_intersects=false;
  bool profile_included=true;
  med_int famId=this->FamilyOnEntity->GetFamily()->GetId();
  for(vtkIdType pindex=0; pindex<pIds->GetNumberOfTuples(); pindex++)
    {
    med_int pid=pIds->GetValue(pindex)-1;
    med_int fid = pea->GetFamilyId(pid);
   // med_int fid=grid->GetPointFamilyId(pid);
    if(fid==famId)
      {// the family of the current point is the good one
      profile_intersects=true;
      }
    else
      {
      // we are on the profile and not on the family -->
      // no exact match, but the the profile might be larger than the family.
      profile_included=false;
      }
    }

  if(!profile_intersects)
    {
    this->IntersectionStatus =
        vtkMedFamilyOnEntityOnProfile::NoIntersection;
    }
  else if(profile_included)
    {
    this->IntersectionStatus =
        vtkMedFamilyOnEntityOnProfile::ProfileIncludedInFamily;
    }
  else
    {
    this->IntersectionStatus =
        vtkMedFamilyOnEntityOnProfile::ProfileIntersectsFamily;
    }
}

vtkIdType vtkMedFamilyOnEntityOnProfile::GetVTKPointIndex(vtkIdType medCIndex)
{
  if(this->IntersectionStatus == NotComputed)
    this->ComputeIntersection(NULL);

  if(this->UseAllPoints)
    return medCIndex;

  if(this->MedToVTKPointIndexMap.find(medCIndex)
    == this->MedToVTKPointIndexMap.end())
    {
    vtkDebugMacro("GetVTKPointIndex asked for "
                  << medCIndex << " which has not been mapped");
    return -1;
    }

  return this->MedToVTKPointIndexMap[medCIndex];
}

void vtkMedFamilyOnEntityOnProfile::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
