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

#ifndef __vtkMedFamilyOnEntityOnProfile_h_
#define __vtkMedFamilyOnEntityOnProfile_h_

// Description :
// This class represents the intersection between a family and an entity and
// a cell profile.
// This is the smallest partition of the the support in a med file.
//
// Fields on points are mapped on cell supports if and only if they
// fully match the cell support.

#include "vtkObject.h"
#include "vtkMed.h"
#include "vtkMedUtilities.h"

class vtkMedFamilyOnEntity;
class vtkMedProfile;

class vtkBitArray;

class VTK_EXPORT vtkMedFamilyOnEntityOnProfile : public vtkObject
{
public :
  static vtkMedFamilyOnEntityOnProfile* New();
  vtkTypeMacro(vtkMedFamilyOnEntityOnProfile, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // This is the family on entity of this support.
  virtual void  SetFamilyOnEntity(vtkMedFamilyOnEntity*);
  vtkGetObjectMacro(FamilyOnEntity, vtkMedFamilyOnEntity);

  // Description:
  // This is the profile of this support.
  virtual void  SetProfile(vtkMedProfile*);
  vtkGetObjectMacro(Profile, vtkMedProfile);

  // Description:
  // This flag informs on the intersection of the family on entity
  // and the profile.
  // The intersection can be :
  // 0 --> not computed
  // 1 --> exact superposition
  // 2 --> partial superposition
  // 3 --> no intersection.
  enum eIntersectionState
    {NotComputed = 0,
    ProfileIncludedInFamily = 1,
    ProfileIntersectsFamily = 2,
    NoIntersection = 3};

  vtkGetMacro(IntersectionStatus, eIntersectionState);
  vtkGetMacro(UseAllPoints, bool);

  // Description:
  // This method computes the IntersectionStatus and the UseAllPoints flags.
  virtual void  ComputeIntersection(vtkMedFieldOnProfile*);

  int CanShallowCopyCellField(vtkMedFieldOnProfile* fop);
  int CanShallowCopyPointField(vtkMedFieldOnProfile* fop);
  int CanShallowCopy(vtkMedFieldOnProfile* fop);
  int CanMapField(vtkMedFieldOnProfile* fop);

  enum ePointProfileVsSupportMatch
    {
    Unknown = 0,
    ProfileLargerThanSupport = 1,
    ProfileEqualsSupport = 2,
    BadOrNoIntersection = 3
    };

  // Description:
  // returns the index to use for this support on the vtk grid
  // from the index stored on the med file -1 (starting to 0)
  vtkIdType GetVTKPointIndex(vtkIdType medCIndex);

  // Description:
  // returns if this point is used by this support.
  bool KeepPoint(med_int index);

  // Description:
  // returns if this cell is used by this support. (only look at the family,
  // not at the profile)
  bool KeepCell(med_int index);

  // Description:
  // This flag is set to false when building the
  // connectivity if it is not valid.
  vtkGetMacro(Valid, int);
  vtkSetMacro(Valid, int);

protected:
  vtkMedFamilyOnEntityOnProfile();
  virtual ~vtkMedFamilyOnEntityOnProfile();

  // Description:
  // Compute the UseAllPoints and IntersectionStatus ivars, depending
  // on the type of field and the type of support.
  virtual void  ComputeCellFamilyVsCellProfileMatch();
  virtual void  ComputePointFamilyVsPointProfileMatch();
  virtual void  ComputeCellFamilyVsPointProfileMatch(vtkMedProfile*);

  // this method compute the UseAllPoints flag, and the MedToVTKPointIndexMap
  // if the flag is set to false.
  virtual void  ComputeUsedPoints();

  vtkMedFamilyOnEntity* FamilyOnEntity;
  vtkMedProfile* Profile;
  eIntersectionState IntersectionStatus;
  bool UseAllPoints;
  int FieldType;
  med_geometry_type FieldGeometryType;

  std::map<med_int, med_int> MedToVTKPointIndexMap;

  std::map<vtkMedProfile*, ePointProfileVsSupportMatch> PointProfileMatch;

  bool MatchComputed;
  int Valid;

private:
  vtkMedFamilyOnEntityOnProfile(const vtkMedFamilyOnEntityOnProfile&);
    // Not implemented.
  void operator=(const vtkMedFamilyOnEntityOnProfile&); // Not implemented.
};

#endif //__vtkMedFamilyOnEntityOnProfile_h_
