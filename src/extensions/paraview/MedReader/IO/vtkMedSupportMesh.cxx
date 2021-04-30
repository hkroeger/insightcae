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

#include "vtkMedSupportMesh.h"

#include "vtkObjectFactory.h"
#include "vtkMedUtilities.h"
#include "vtkMedFile.h"

#include "vtkStringArray.h"

#include <map>
#include <string>
#include <vector>
using namespace std;

vtkCxxSetObjectMacro(vtkMedSupportMesh, ParentFile, vtkMedFile);

vtkCxxRevisionMacro(vtkMedSupportMesh, "$Revision: 1.1.2.2 $")
vtkStandardNewMacro(vtkMedSupportMesh)

vtkMedSupportMesh::vtkMedSupportMesh()
{
  this->MedIterator = -1;
  this->Name = NULL;
  this->ParentFile = NULL;
  this->AxisName = vtkStringArray::New();
  this->AxisUnit = vtkStringArray::New();
  this->Description = NULL;
  this->AxisType = MED_UNDEF_AXIS_TYPE;
  this->SpaceDimension = 0;
  this->MeshDimension = 0;
}

vtkMedSupportMesh::~vtkMedSupportMesh()
{
  this->SetName(NULL);
  this->SetDescription(NULL);
  this->SetParentFile(NULL);
  this->AxisName->Delete();
  this->AxisUnit->Delete();
}

void vtkMedSupportMesh::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
