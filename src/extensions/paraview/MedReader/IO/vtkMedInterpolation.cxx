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

#include "vtkMedInterpolation.h"

#include "vtkObjectFactory.h"

#include "vtkMedUtilities.h"
#include "vtkMedFraction.h"

vtkStandardNewMacro(vtkMedInterpolation)

vtkCxxGetObjectVectorMacro(vtkMedInterpolation, BasisFunction,
                           vtkMedFraction);
vtkCxxSetObjectVectorMacro(vtkMedInterpolation, BasisFunction,
                           vtkMedFraction);

vtkMedInterpolation::vtkMedInterpolation()
{
  this->GeometryType = MED_UNDEF_GEOTYPE;
  this->IsCellNode = 1;
  this->MaximumNumberOfCoefficient = 0;
  this->MaximumDegree = 0;
  this->NumberOfVariable = 0;
  this->Name = NULL;
  this->BasisFunction = new vtkObjectVector<vtkMedFraction>();
}

vtkMedInterpolation::~vtkMedInterpolation()
{
  delete this->BasisFunction;
  this->SetName(NULL);
}

void vtkMedInterpolation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
