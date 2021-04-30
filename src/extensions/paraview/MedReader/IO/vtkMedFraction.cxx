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

#include "vtkMedFraction.h"

#include "vtkObjectFactory.h"

#include "vtkIntArray.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkMedFraction)

vtkMedFraction::vtkMedFraction()
{
  this->Coefficients = vtkDoubleArray::New();
  this->Powers = vtkIntArray::New();
  this->DenominatorCoefficients = vtkDoubleArray::New();
  this->DenominatorPowers = vtkIntArray::New();
  this->NumberOfVariable = 0;
}

vtkMedFraction::~vtkMedFraction()
{
  this->Coefficients->Delete();
  this->Powers->Delete();
  this->DenominatorCoefficients->Delete();
  this->DenominatorPowers->Delete();
}

vtkIntArray* vtkMedFraction::GetPowers()
{
  return this->Powers;
}

vtkDoubleArray* vtkMedFraction::GetCoefficients()
{
  return this->Coefficients;
}

vtkIntArray* vtkMedFraction::GetDenominatorPowers()
{
  return this->DenominatorPowers;
}

vtkDoubleArray* vtkMedFraction::GetDenominatorCoefficients()
{
  return this->DenominatorCoefficients;
}

void vtkMedFraction::SetNumberOfCoefficients(int ncoeff)
{
  this->Powers->SetNumberOfTuples(ncoeff);
  this->Coefficients->SetNumberOfTuples(ncoeff);

  int* powers = this->Powers->GetPointer(0);
  memset(powers, 0, ncoeff*this->Powers->GetNumberOfComponents()*sizeof(int));

  double* coeffs = this->Coefficients->GetPointer(0);
  memset(powers, 0, ncoeff*sizeof(double));
}

void vtkMedFraction::SetNumberOfDenominatorCoefficients(int ncoeff)
{
  this->DenominatorPowers->SetNumberOfTuples(ncoeff);
  this->DenominatorCoefficients->SetNumberOfTuples(ncoeff);

  int* powers = this->DenominatorPowers->GetPointer(0);
  memset(powers, 0,
         ncoeff*this->DenominatorPowers->GetNumberOfComponents()*sizeof(int));

  double* coeffs = this->DenominatorCoefficients->GetPointer(0);
  memset(powers, 0, ncoeff*sizeof(double));
}

void vtkMedFraction::SetNumberOfVariable(int nbofvariable)
{
  int nvar;
  if(nbofvariable <= 0)
    {
    nvar = 1;
    }
  else
    {
    nvar = nbofvariable;
    }

  this->Powers->SetNumberOfComponents(nvar);
  this->DenominatorPowers->SetNumberOfComponents(nvar);

  // force an allocation
  this->Powers->SetNumberOfTuples(
      this->Powers->GetNumberOfTuples());
  int* powers = this->Powers->GetPointer(0);
  memset(powers, 0,
         nvar*this->Powers->GetNumberOfComponents()*sizeof(int));

  this->DenominatorPowers->SetNumberOfTuples(
      this->DenominatorPowers->GetNumberOfTuples());
  int* denom_powers = this->DenominatorPowers->GetPointer(0);
  memset(denom_powers, 0,
         nvar*this->DenominatorPowers->GetNumberOfComponents()*sizeof(int));

  this->NumberOfVariable = nbofvariable;
}

double vtkMedFraction::Evaluate(double* coord)
{
  if(this->Coefficients->GetNumberOfTuples() == 0)
    {
    return 0.0;
    }

  if(this->NumberOfVariable == 0)
    {
    return this->Coefficients->GetValue(0);
    }

  double res = 0.0;
  for(int coeffid = 0; coeffid <
      this->Coefficients->GetNumberOfTuples(); coeffid++)
    {
    double prod = this->Coefficients->GetValue(coeffid);
    for(int varid=0; varid<this->NumberOfVariable; varid++)
      {
      prod *= pow(coord[varid],
                  this->Powers->GetValue(
                      this->NumberOfVariable*coeffid+varid));
      }
    res += prod;
    }

  double denom_res = 0.0;

  if(this->DenominatorCoefficients->GetNumberOfTuples() == 0)
    {
    denom_res = 1.0;
    }
  else
    {
    for(int coeffid = 0; coeffid <
        this->DenominatorCoefficients->GetNumberOfTuples(); coeffid++)
      {
      double prod = this->DenominatorCoefficients->GetValue(coeffid);
      for(int varid=0; varid<this->NumberOfVariable; varid++)
        {
        prod *= pow(coord[varid],
                    this->DenominatorPowers->GetValue(
                        this->NumberOfVariable*coeffid+varid));
        }
      denom_res += prod;
      }
    }

  return res / denom_res;
}

double vtkMedFraction::Evaluate1(double coord)
{
  if(this->NumberOfVariable != 1)
    {
    vtkErrorMacro("Evaluate1 can only be called if the NumberOfVariable is 1");
    }

  return this->Evaluate(&coord);
}

double vtkMedFraction::Evaluate2(double x, double y)
{
  if(this->NumberOfVariable != 2)
    {
    vtkErrorMacro("Evaluate2 can only be called if the NumberOfVariable is 2");
    }

  double coord[2] = {x, y};

  return this->Evaluate(coord);
}

double vtkMedFraction::Evaluate3(double x, double y, double z)
{
  if(this->NumberOfVariable != 3)
    {
    vtkErrorMacro("Evaluate3 can only be called if the NumberOfVariable is 3");
    }

  double coord[3] = {x, y, z};

  return this->Evaluate(coord);
}

void vtkMedFraction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
