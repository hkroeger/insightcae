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

#ifndef __vtkMedFamilyOnEntity_h_
#define __vtkMedFamilyOnEntity_h_

// Description :
// This class represents the intersection between a family and an entity.

#include "vtkObject.h"
#include "vtkMed.h"
#include "vtkMedUtilities.h"

class vtkMedEntityArray;
class vtkMedFamily;
class vtkMedGrid;
class vtkMedFamilyOnEntityOnProfile;
class vtkMedIntArray;

class VTK_EXPORT vtkMedFamilyOnEntity : public vtkObject
{
public :
  static vtkMedFamilyOnEntity* New();
  vtkTypeMacro(vtkMedFamilyOnEntity, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // This is the family of this family on entity.
  virtual void  SetFamily(vtkMedFamily*);
  vtkGetObjectMacro(Family, vtkMedFamily);

  // Description:
  // This is the entity array this family on entity is on.
  virtual void  SetEntityArray(vtkMedEntityArray*);
  vtkGetObjectMacro(EntityArray, vtkMedEntityArray);

  // Description:
  // This is the grid this family on entity is reffering to
  virtual void  SetParentGrid(vtkMedGrid*);
  vtkGetObjectMacro(ParentGrid, vtkMedGrid);

  // Description :
  //  Returns vtkMedUtilities::OnPoint (0) or vtkMedUtilities::OnCell (1)
  virtual int  GetPointOrCell();

  // Description :
  // Returns true if the family is on points or if the entity is MED_POINT1
  // of if it is MED_BALL or MED_PARTICLE elements
  virtual int  GetVertexOnly();

  // Description:
  // returns the entity descriptor associated with this family on entity
  virtual vtkMedEntity GetEntity();

  // Description:
  // Fields can use profile to be stored on a subset of entities.
  // a priori, profiles and families are independent notions.
  // In case there are profiles, we create the intersection of the profile and
  // the FamilyOnEntity to be able to map the field on the geometry.
  // BEFORE calling those function, you have to have assigned a profile
  // to the vtkMedFamilyOnEntityOnProfile
  void  AddFamilyOnEntityOnProfile(vtkMedFamilyOnEntityOnProfile*);
  int GetNumberOfFamilyOnEntityOnProfile();
  vtkMedFamilyOnEntityOnProfile* GetFamilyOnEntityOnProfile(vtkMedProfile*);
  vtkMedFamilyOnEntityOnProfile* GetFamilyOnEntityOnProfile(int index);

protected:
  vtkMedFamilyOnEntity();
  virtual ~vtkMedFamilyOnEntity();

  vtkMedFamily* Family;
  vtkMedEntityArray* EntityArray;
  vtkMedGrid* ParentGrid;

  std::map<vtkMedProfile*, vtkSmartPointer<vtkMedFamilyOnEntityOnProfile> >
      FamilyOnEntityOnProfile;

private:
  vtkMedFamilyOnEntity(const vtkMedFamilyOnEntity&); // Not implemented.
  void operator=(const vtkMedFamilyOnEntity&); // Not implemented.

};

#endif //__vtkMedFamilyOnEntity_h_
