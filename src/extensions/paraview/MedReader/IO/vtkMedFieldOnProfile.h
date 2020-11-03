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

#ifndef __vtkMedFieldOnProfile_h_
#define __vtkMedFieldOnProfile_h_

#include "vtkObject.h"
#include "vtkMedSetGet.h"
#include "vtkMed.h"
#include"vtkMedFilter.h"

class vtkMedFieldOverEntity;
class vtkMedString;
class vtkDataArray;
class vtkMedProfile;

class VTK_EXPORT vtkMedFieldOnProfile: public vtkObject
{
public:
  static vtkMedFieldOnProfile* New();
  vtkTypeMacro(vtkMedFieldOnProfile, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the iterator that is used while reading fromt he med file
  vtkSetMacro(MedIterator, med_int);
  vtkGetMacro(MedIterator, med_int);

  // Description:
  // This is the FieldOverEntity this profile is on
  virtual void  SetParentFieldOverEntity(vtkMedFieldOverEntity*);
  vtkGetObjectMacro(ParentFieldOverEntity, vtkMedFieldOverEntity);

  // Description:
  // This is the name of the profile used by this field.
  vtkGetStringMacro(ProfileName);
  vtkSetStringMacro(ProfileName);

  // Description:
  // This is the name of the localization used by this field on this profile.
  vtkGetStringMacro(LocalizationName);
  vtkSetStringMacro(LocalizationName);

  // Description:
  // This stores the size of the profile.
  vtkSetMacro(ProfileSize, med_int);
  vtkGetMacro(ProfileSize, med_int);

  // Description:
  // This stores the number of integration points for this localization.
  vtkSetMacro(NumberOfIntegrationPoint, med_int);
  vtkGetMacro(NumberOfIntegrationPoint, med_int);

  // Description:
  // The number of values of this field on this mesh
  vtkSetMacro(NumberOfValues, med_int);
  vtkGetMacro(NumberOfValues, med_int);

  // Description:
  // the Data that store the values read from file
  virtual void  SetData(vtkDataArray*);
  vtkGetObjectMacro(Data, vtkDataArray);

  // Description:
  // the Profile object associated witht he profile name.
  virtual void  SetProfile(vtkMedProfile*);
  vtkGetObjectMacro(Profile, vtkMedProfile);

  // Description:
  // returns true if the data has been loaded
  int IsLoaded();

  // Description:
  // returns true if the profile name is not MED_NO_PROFILE
  int GetHasProfile();

  // Description:
  // Load the actual data of this field
  void  Load(med_storage_mode mode);

  // Description:
  // Get/Set of the MED Filter for parallel reading.
  //void  SetFilter(const med_filter& filter){this->Filter = filter;}
  //med_filter GetFilter(){return this->Filter;}
  void  SetFilter(vtkMedFilter* filter){this->Filter = filter;}
  vtkMedFilter* GetFilter(){return this->Filter;}
protected:
  vtkMedFieldOnProfile();
  virtual ~vtkMedFieldOnProfile();

  med_int MedIterator;
  vtkMedFieldOverEntity *ParentFieldOverEntity;
  char* ProfileName;
  med_int ProfileSize;
  char* LocalizationName;
  med_int NumberOfIntegrationPoint;
  med_int NumberOfValues;
  vtkDataArray* Data;
  vtkMedProfile* Profile;

  //med_filter Filter;
  vtkMedFilter *Filter;

private:
  vtkMedFieldOnProfile(const vtkMedFieldOnProfile&); // Not implemented.
  void operator=(const vtkMedFieldOnProfile&); // Not implemented.

};

#endif //__vtkMedFieldOnProfile_h_
