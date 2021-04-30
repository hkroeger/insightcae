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

#ifndef __vtkMedFieldOverEntity_h_
#define __vtkMedFieldOverEntity_h_

#include "vtkObject.h"
#include "vtkMed.h"

#include "vtkMedSetGet.h"
#include "vtkMedUtilities.h"

class vtkMedFieldOnProfile;
class vtkMedFieldStep;

class VTK_EXPORT vtkMedFieldOverEntity: public vtkObject
{
public:
  static vtkMedFieldOverEntity* New();
  vtkTypeMacro(vtkMedFieldOverEntity, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // the support of the cells : one of
  void  SetEntity(const vtkMedEntity& entity){this->Entity = entity;}
  const vtkMedEntity& GetEntity(){return this->Entity;}

  // Description:
  // This is the vtkMedFieldStep that owns this vtkMedFieldOverEntity
  virtual void  SetParentStep(vtkMedFieldStep*);
  vtkGetObjectMacro(ParentStep, vtkMedFieldStep);

  // Description:
  // This array store for each profile the field over this profile
  vtkGetObjectVectorMacro(FieldOnProfile, vtkMedFieldOnProfile);
  vtkSetObjectVectorMacro(FieldOnProfile, vtkMedFieldOnProfile);

  // Description:
  // This flag is set during the information pass, and tells if
  // there is a profile of not.
  // Note that if there is no profile, a dummy vtkMedFieldOnProfile
  // is created to store the actual data.
  vtkGetMacro(HasProfile, int);
  vtkSetMacro(HasProfile, int);

protected:
  vtkMedFieldOverEntity();
  virtual ~vtkMedFieldOverEntity();

  vtkMedFieldStep* ParentStep;

  vtkMedEntity  Entity;

  int HasProfile;

  //BTX
  vtkObjectVector<vtkMedFieldOnProfile>* FieldOnProfile;
  //ETX

private:
  vtkMedFieldOverEntity(const vtkMedFieldOverEntity&); // Not implemented.
  void operator=(const vtkMedFieldOverEntity&); // Not implemented.
};

#endif //__vtkMedFieldOverEntity_h_
