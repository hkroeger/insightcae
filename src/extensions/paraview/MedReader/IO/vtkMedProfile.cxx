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

#include "vtkMedProfile.h"

#include "vtkObjectFactory.h"
#include "vtkMedIntArray.h"
#include "vtkMedSetGet.h"
#include "vtkMedUtilities.h"
#include "vtkMedFile.h"
#include "vtkMedDriver.h"

vtkCxxSetObjectMacro(vtkMedProfile,Ids,vtkMedIntArray);
vtkCxxSetObjectMacro(vtkMedProfile,ParentFile,vtkMedFile);

vtkStandardNewMacro(vtkMedProfile);

vtkMedProfile::vtkMedProfile()
{
  this->NumberOfElement = 0;
  this->Ids = NULL;
  this->Name = NULL;
  this->MedIterator = -1;
  this->GeometryType = MED_NO_GEOTYPE;
  this->ParentFile = NULL;
}

vtkMedProfile::~vtkMedProfile()
{
  this->SetName(NULL);
  this->SetIds(NULL);
}

int vtkMedProfile::IsLoaded()
{
  return this->Ids != NULL && this->Ids->GetNumberOfComponents()
      == 1 && this->Ids->GetNumberOfTuples() == this->NumberOfElement
      && this->NumberOfElement > 0;
}

void  vtkMedProfile::Load()
{
  this->ParentFile->GetMedDriver()->LoadProfile(this);
}

void vtkMedProfile::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  PRINT_IVAR(os, indent, NumberOfElement);
  PRINT_IVAR(os, indent, MedIterator);
  PRINT_OBJECT(os, indent, Ids);
}
