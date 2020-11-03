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

#ifndef __vtkMedInterpolation_h_
#define __vtkMedInterpolation_h_

#include "vtkObject.h"
#include "vtkMed.h"
#include "vtkMedSetGet.h"

class vtkMedString;
class vtkMedFraction;

class VTK_EXPORT vtkMedInterpolation : public vtkObject
{
public:
  static vtkMedInterpolation* New();
  vtkTypeMacro(vtkMedInterpolation, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // name of the interpolation function
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // This is the iterator that should be used to read this interpolation
  // in the med file
  vtkSetMacro(MedIterator, med_int);
  vtkGetMacro(MedIterator, med_int);

  // Description:
  // Type geometrique des mailles
  vtkSetMacro(GeometryType, med_geometry_type);
  vtkGetMacro(GeometryType, med_geometry_type);

  // Description:
  // 1 if the basis functions are relative to the vertices of the cell.
  vtkSetMacro(IsCellNode, int);
  vtkGetMacro(IsCellNode, int);

  // Description:
  // Maximum degree of any coefficient of any basis function
  vtkSetMacro(MaximumDegree, int);
  vtkGetMacro(MaximumDegree, int);

  // Description:
  // Maximum number of coefficients for any basis function
  vtkSetMacro(MaximumNumberOfCoefficient, int);
  vtkGetMacro(MaximumNumberOfCoefficient, int);

  // Description:
  // Maximum number of coefficients for any basis function
  vtkSetMacro(NumberOfVariable, int);
  vtkGetMacro(NumberOfVariable, int);

  // Description:
  // The basis functions
  vtkGetObjectVectorMacro(BasisFunction, vtkMedFraction);
  vtkSetObjectVectorMacro(BasisFunction, vtkMedFraction);

protected :
  vtkMedInterpolation();
  ~vtkMedInterpolation();

  med_int MedIterator;
  med_geometry_type GeometryType;
  int IsCellNode;
  int MaximumNumberOfCoefficient;
  int MaximumDegree;
  int NumberOfVariable;
  char* Name;
  vtkObjectVector<vtkMedFraction>* BasisFunction;
};

#endif //__vtkMedInterpolation_h_
