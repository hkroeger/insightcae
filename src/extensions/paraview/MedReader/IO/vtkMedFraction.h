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

#ifndef __vtkMedFraction_h_
#define __vtkMedFraction_h_

#include "vtkObject.h"

class vtkIntArray;
class vtkDoubleArray;

class VTK_EXPORT vtkMedFraction : public vtkObject
{
public:
  static vtkMedFraction* New();
  vtkTypeMacro(vtkMedFraction, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // the powers of this polynomial function
  // the number of components of this array is equal to the maximum degree
  // and the number of tuples to the numner of coefficients
  virtual vtkIntArray* GetPowers();

  // Description:
  // The coefficients of this polynomial function
  virtual vtkDoubleArray* GetCoefficients();

  // Description:
  // the powers of this polynomial function
  // the number of components of this array is equal to the maximum degree
  // and the number of tuples to the numner of coefficients
  virtual vtkIntArray* GetDenominatorPowers();

  // Description:
  // The coefficients of this polynomial function
  virtual vtkDoubleArray* GetDenominatorCoefficients();

  // Description:
  // This sets the number of coefficients in this polynom
  void  SetNumberOfCoefficients(int);

  // Description:
  // This sets the number of coefficients in this polynom
  void  SetNumberOfDenominatorCoefficients(int);

  // Description:
  // This sets the maximum degree of this polynom.
  // This also allocates the array to store the powers of this
  // polynom
  void  SetNumberOfVariable(int);
  vtkGetMacro(NumberOfVariable, int);

  // Description:
  // Evaluate the value of this function at this point.
  virtual double Evaluate(double*);
  virtual double Evaluate1(double);
  virtual double Evaluate2(double, double);
  virtual double Evaluate3(double, double, double);

protected :
  vtkMedFraction();
  ~vtkMedFraction();

  vtkIntArray* Powers;
  vtkDoubleArray* Coefficients;

  vtkIntArray* DenominatorPowers;
  vtkDoubleArray* DenominatorCoefficients;

  int NumberOfVariable;

private :
    vtkMedFraction(const vtkMedFraction&); // Not implemented.
    void operator=(const vtkMedFraction&); // Not implemented.
};

#endif //__vtkMedFraction_h_
