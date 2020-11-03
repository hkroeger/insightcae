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

#include "vtkMedLocalization.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkFunctionParser.h"

#include "vtkMedUtilities.h"
#include "vtkMedSetGet.h"
#include "vtkMedFile.h"
#include "vtkMedDriver.h"
#include "vtkMedInterpolation.h"
#include "vtkMedFraction.h"

#include "med.h"

// SEG2
const static int SEG2_dim = 1;
const static int SEG2_nnode = 2;
static const int SEG2_aster2med[SEG2_nnode] =
{0, 1};
static const char* SEG2_varnames[SEG2_dim] = {"x"};
static const char* SEG2_functions[SEG2_nnode] =
{"1/2*(1-x)",
 "1/2*(1+x)"};

// SEG3
const static int SEG3_dim = 1;
const static int SEG3_nnode = 3;
static const int SEG3_aster2med[SEG3_nnode] =
{0, 1, 2};
static const char* SEG3_varnames[SEG3_dim] = {"x"};
static const char* SEG3_functions[SEG3_nnode] =
{"-1/2*(1-x)*x",
  "1/2*(1+x)*x",
  "(1+x)*(1-x)"};

// SEG4
const static int SEG4_dim = 1;
const static int SEG4_nnode = 4;
static const int SEG4_aster2med[SEG4_nnode] =
{0, 1, 2, 3};
static const char* SEG4_varnames[SEG4_dim] = {"x"};
static const char* SEG4_functions[SEG4_nnode] =
{"16/9*(1-x)*(x+1/3)*(x-1/3)",
"-16/9*(1+x)*(1/3-x)*(x+1/3)",
"16/27*(x-1)*(x+1)*(x-1/3)",
"-16/27*(x-1)*(x+1)*(x+1/3)"};

// TRIA3
const static int TRIA3_dim = 2;
const static int TRIA3_nnode = 3;
static const int TRIA3_aster2med[TRIA3_nnode] =
{0, 1, 2};
static const char* TRIA3_varnames[TRIA3_dim] = {"x", "y"};
static const char* TRIA3_functions[TRIA3_nnode] =
{"1-x-y",
 "x",
 "y"};

// TRIA6
const static int TRIA6_dim = 2;
const static int TRIA6_nnode = 6;
static const int TRIA6_aster2med[TRIA6_nnode] =
{0, 1, 2, 3, 4, 5};
static const char* TRIA6_varnames[TRIA6_dim] = {"x", "y"};
static const char* TRIA6_functions[TRIA6_nnode] =
{"-(1-x-y)*(1-2*(1-x-y))",
 "-x*(1-2*x)",
 "-y*(1-2*y)",
 "4*x*(1-x-y)",
 "4*x*y",
 "4*y*(1-x-y)"};

// TRIA7
const static int TRIA7_dim = 2;
const static int TRIA7_nnode = 7;
static const int TRIA7_aster2med[TRIA7_nnode] =
{0, 1, 2, 3, 4, 5, 6};
static const char* TRIA7_varnames[TRIA7_dim] = {"x", "y"};
static const char* TRIA7_functions[TRIA7_nnode] =
{"1-3*(x+y)+2*(x*x+y*y)+7*x*y-3*x*y*(x+y)",
 "x*(-1+2*x+3*y-3*y*(x+y))",
 "y*(-1+2*x+3*y-3*x*(x+y))",
 "4*x*(1-x-4*y+3*y*(x+y))",
 "4*x*y*(-2+3*(x+y))",
 "4*y*(1-4*x-y+3*x*(x+y))",
 "27*x*y*(1-x-y)"};

// QUAD4
const static int QUAD4_dim = 2;
const static int QUAD4_nnode = 4;
static const int QUAD4_aster2med[QUAD4_nnode] =
{0, 1, 2, 3};
static const char* QUAD4_varnames[QUAD4_dim] = {"x", "y"};
static const char* QUAD4_functions[QUAD4_nnode] =
{"(1-x)*(1-y)/4",
 "(1+x)*(1-y)/4",
 "(1+x)*(1+y)/4",
 "(1-x)*(1+y)/4"};

// QUAD8
const static int QUAD8_dim = 2;
const static int QUAD8_nnode = 8;
static const int QUAD8_aster2med[QUAD8_nnode] =
{0, 1, 2, 3, 4, 5, 6, 7};
static const char* QUAD8_varnames[QUAD8_dim] = {"x", "y"};
static const char* QUAD8_functions[QUAD8_nnode] =
{"(1-x)*(1-y)*(-1-x-y)/4",
 "(1+x)*(1-y)*(-1+x-y)/4",
 "(1+x)*(1+y)*(-1+x+y)/4",
 "(1-x)*(1+y)*(-1-x+y)/4",
 "(1-x*x)*(1-y)/2",
 "(1+x)*(1-y*y)/2",
 "(1-x*x)*(1+y)/2",
 "(1-x)*(1-y*y)/2"};

// QUAD9
const static int QUAD9_dim = 2;
const static int QUAD9_nnode = 9;
static const int QUAD9_aster2med[QUAD9_nnode] =
{0, 1, 2, 3, 4, 5, 6, 7, 8};
static const char* QUAD9_varnames[QUAD9_dim] = {"x", "y"};
static const char* QUAD9_functions[QUAD9_nnode] =
{"x*y*(x-1)*(y-1)/4",
 "x*y*(x+1)*(y-1)/4",
 "x*y*(x+1)*(y+1)/4",
 "x*y*(x-1)*(y+1)/4",
 "(1-x*x)*y*(y-1)/2",
 "x*(x+1)*(1-y*y)/2",
 "(1-x*x)*y*(y+1)/2",
 "x*(x-1)*(1-y*y)/2",
 "(1-x*x)*(1-y*y)"};

// PENTA6
const static int PENTA6_dim = 3;
const static int PENTA6_nnode = 6;
static const int PENTA6_aster2med[PENTA6_nnode] =
{0, 1, 2, 3, 4, 5};
static const char* PENTA6_varnames[PENTA6_dim] = {"x", "y", "z"};
static const char* PENTA6_functions[PENTA6_nnode] =
{"1/2*y*(1-x)",
 "1/2*z*(1-x)",
 "1/2*(1-y-z)*(1-x)",
 "1/2*y*(1+x)",
 "1/2*z*(1+x)",
 "1/2*(1-y-z)*(1+x)"};

// PENTA15
const static int PENTA15_dim = 3;
const static int PENTA15_nnode = 15;
static const int PENTA15_aster2med[PENTA15_nnode] =
{0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 13, 14, 9, 10, 11};
static const char* PENTA15_varnames[PENTA15_dim] = {"x", "y", "z"};
static const char* PENTA15_functions[PENTA15_nnode] =
{"y*(1-x)*(2*y-2-x)/2",
 "z*(1-x)*(2*z-2-x)/2",
 "(x-1)*(1-y-z)*(x+2*y+2*z)/2",
 "y*(1+x)*(2*y-2+x)/2",
 "z*(1+x)*(2*z-2+x)/2",
 "(-x-1)*(1-y-z)*(-x+2*y+2*z)/2",
 "2*y*z*(1-x)",
 "2*z*(1-y-z)*(1-x)",
 "2*y*(1-y-z)*(1-x)",
 "2*y*z*(1+x)",
 "2*z*(1-y-z)*(1+x)",
 "2*y*(1-y-z)*(1+x)",
 "y*(1-x*x)",
 "z*(1-x*x)",
 "(1-y-z)*(1-x*x)"};


// PENTA18
const static int PENTA18_dim = 3;
const static int PENTA18_nnode = 18;
const static int PENTA18_aster2med[PENTA18_nnode] =
{0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 13, 14, 9, 10, 11, 15, 16, 17};
static const char* PENTA18_varnames[PENTA18_dim] = {"x", "y", "z"};
static const char* PENTA18_functions[PENTA18_nnode] =
{"x*y*(x−1)*(2*y−1)/2",
 "x*z*(x−1)*(2*z−1)/2",
 "x*(x−1)*(zy−1)*(2*z2*y−1)/2",
 "x*y*(x1)*(2*y−1)/2",
 "x*z*(x1)*(2*z−1)/2",
 "x*(x1)*(zy−1)*(2*z2*y−1)/2",
 "2*x*y*z*(x−1)",
 "−2*x*z*(x−1)*(zy−1)",
 "−2*x*y*(x−1)*(zy−1)",
 "2*x*y*z*(x1)",
 "−2*x*z*(x1)*(zy−1)",
 "−2*x*y*(x1)*(zy−1)",
 "y*(1−x*x)*(2*y−1)",
 "z*(1−x*x)*(2*z−1)",
 "(1−x*x)*(zy−1)*(2*z2*y−1)",
 "4*y*z*(1−x*x)",
 "4*z*(x−1)*(zy−1)",
 "4*y*(x−1)*(zy−1)"};

// HEXA8
const static int HEXA8_dim = 3;
const static int HEXA8_nnode = 8;
static const int HEXA8_aster2med[HEXA8_nnode] =
{0, 1, 2, 3, 4, 5, 6, 7};
static const char* HEXA8_varnames[HEXA8_dim] = {"x", "y", "z"};
static const char* HEXA8_functions[HEXA8_nnode] =
{"1/8*(1-x)*(1-y)*(1-z)",
 "1/8*(1+x)*(1-y)*(1-z)",
 "1/8*(1+x)*(1+y)*(1-z)",
 "1/8*(1-x)*(1+y)*(1-z)",
 "1/8*(1-x)*(1-y)*(1+z)",
 "1/8*(1+x)*(1-y)*(1+z)",
 "1/8*(1+x)*(1+y)*(1+z)",
 "1/8*(1-x)*(1+y)*(1+z)"
   };

// HEXA20
const static int HEXA20_dim = 3;
const static int HEXA20_nnode = 20;
static const int HEXA20_aster2med[HEXA20_nnode] =
{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18, 19, 12, 13, 14, 15};
static const char* HEXA20_varnames[HEXA20_dim] = {"x", "y", "z"};
static const char* HEXA20_functions[HEXA20_nnode] =
{"1/8*(1-x)*(1-y)*(1-z)*(-2-x-y-z)",
 "1/8*(1+x)*(1-y)*(1-z)*(-2+x-y-z)",
 "1/8*(1+x)*(1+y)*(1-z)*(-2+x+y-z)",
 "1/8*(1-x)*(1+y)*(1-z)*(-2-x+y-z)",
 "1/8*(1-x)*(1-y)*(1+z)*(-2-x-y+z)",
 "1/8*(1+x)*(1-y)*(1+z)*(-2+x-y+z)",
 "1/8*(1+x)*(1+y)*(1+z)*(-2+x+y+z)",
 "1/8*(1-x)*(1+y)*(1+z)*(-2-x+y+z)",
 "1/4*(1-x*x)*(1-y)*(1-z)",
 "1/4*(1-y*y)*(1+x)*(1-z)",
 "1/4*(1-x*x)*(1+y)*(1-z)",
 "1/4*(1-y*y)*(1-x)*(1-z)",
 "1/4*(1-x*x)*(1-y)*(1+z)",
 "1/4*(1-y*y)*(1+x)*(1+z)",
 "1/4*(1-x*x)*(1+y)*(1+z)",
 "1/4*(1-y*y)*(1-x)*(1+z)",
 "1/4*(1-z*z)*(1-x)*(1-y)",
 "1/4*(1-z*z)*(1+x)*(1-y)",
 "1/4*(1-z*z)*(1+x)*(1+y)",
 "1/4*(1-z*z)*(1-x)*(1+y)"
  };
// HEXA27
const static int HEXA27_dim = 3;
const static int HEXA27_nnode = 27;
static const int HEXA27_aster2med[HEXA27_nnode] =
{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18, 19, 12, 13, 14, 15, 24, 22,
 21, 23, 20, 25, 26};
static const char* HEXA27_varnames[HEXA27_dim] = {"x", "y", "z"};
static const char* HEXA27_functions[HEXA27_nnode] =
{"1/8*x*(x-1)*y*(y-1)*z*(z-1)",
 "1/8*x*(x+1)*y*(y-1)*z*(z-1)",
 "1/8*x*(x+1)*y*(y+1)*z*(z-1)",
 "1/8*x*(x-1)*y*(y+1)*z*(z-1)",
 "1/8*x*(x-1)*y*(y-1)*z*(z+1)",
 "1/8*x*(x+1)*y*(y-1)*z*(z+1)",
 "1/8*x*(x+1)*y*(y+1)*z*(z+1)",
 "1/8*x*(x-1)*y*(y+1)*z*(z+1)",
 "1/4*(1-x*x)*y*(y-1)*z*(z-1)",
 "1/4*x*(x+1)*(1-y*y)*z*(z-1)",
 "1/4*(1-x*x)*y*(y+1)*z*(z-1)",
 "1/4*x*(x-1)*(1-y*y)*z*(z-1)",
 "1/4*(1-x*x)*y*(y-1)*z*(z+1)",
 "1/4*x*(x+1)*(1-y*y)*z*(z+1)",
 "1/4*(1-x*x)*y*(y+1)*z*(z+1)",
 "1/4*x*(x-1)*(1-y*y)*z*(z+1)",
 "1/4*x*(x-1)*y*(y-1)*(1-z*z)",
 "1/4*x*(x+1)*y*(y-1)*(1-z*z)",
 "1/4*x*(x+1)*y*(y+1)*(1-z*z)",
 "1/4*x*(x-1)*y*(y+1)*(1-z*z)",
 "1/2*x*(x-1)*(1-y*y)*(1-z*z)",
 "1/2*x*(x+1)*(1-y*y)*(1-z*z)",
 "1/2*(1-x*x)*y*(y-1)*(1-z*z)",
 "1/2*(1-x*x)*y*(y+1)*(1-z*z)",
 "1/2*(1-x*x)*(1-y*y)*z*(z-1)",
 "1/2*(1-x*x)*(1-y*y)*z*(z+1)",
 "(1-x*x)*(1-y*y)*(1-z*z)"
  };

// TETRA4
const static int TETRA4_dim = 3;
const static int TETRA4_nnode = 4;
static const int TETRA4_aster2med[TETRA4_nnode] =
{0, 1, 2, 3};
static const char* TETRA4_varnames[TETRA4_dim] = {"x", "y", "z"};
static const char* TETRA4_functions[TETRA4_nnode] =
{
  "y",
  "z",
  "1-x-y-z",
  "x"
};

// TETRA10
const static int TETRA10_dim = 3;
const static int TETRA10_nnode = 10;
static const int TETRA10_aster2med[TETRA10_nnode] =
{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static const char* TETRA10_varnames[TETRA10_dim] = {"x", "y", "z"};
static const char* TETRA10_functions[TETRA10_nnode] =
{
  "y*(2*y-1)",
  "z*(2*z-1)",
  "(1-x-y-z)*(1-2*x-2*y-2*z)",
  "x*(2*x-1)",
  "4*y*z",
  "4*z*(1-x-y-z)",
  "4*y*(1-x-y-z)",
  "4*x*y",
  "4*x*z",
  "4*x*(1-x-y-z)"
};

// PYRA5
const static int PYRA5_dim = 3;
const static int PYRA5_nnode = 5;
static const int PYRA5_aster2med[PYRA5_nnode] =
{0, 1, 2, 3, 4};
static const char* PYRA5_varnames[PYRA5_dim] = {"x", "y", "z"};
static const char* PYRA5_functions[PYRA5_nnode] =
{
"(-x+y+z-1)*(-x-y+z-1)/(4*(1-z))",
"(-x-y+z-1)*( x-y+z-1)/(4*(1-z))",
"( x-y+z-1)*( x+y+z-1)/(4*(1-z))",
"( x+y+z-1)*(-x+y+z-1)/(4*(1-z))",
"z"
};

// PYRA13
const static int PYRA13_dim = 3;
const static int PYRA13_nnode = 13;
static const int PYRA13_aster2med[PYRA13_nnode] =
{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
static const char* PYRA13_varnames[PYRA13_dim] = {"x", "y", "z"};
static const char* PYRA13_functions[PYRA13_nnode] =
{
  "(-x+y+z-1)*(-x-y+z-1)*( x-1/2)/(2*(1-z))",
  "(-x-y+z-1)*( x-y+z-1)*( y-1/2)/(2*(1-z))",
  "( x-y+z-1)*( x+y+z-1)*(-x-1/2)/(2*(1-z))",
  "( x+y+z-1)*(-x+y+z-1)*(-y-1/2)/(2*(1-z))",
  "2*z*(z-1/2)",
  "-(-x+y+z-1)*(-x-y+z-1)*( x-y+z-1)/(2*(1-z))",
  "-(-x-y+z-1)*( x-y+z-1)*( x+y+z-1)/(2*(1-z))",
  "-( x-y+z-1)*( x+y+z-1)*(-x+y+z-1)/(2*(1-z))",
  "-( x+y+z-1)*(-x+y+z-1)*(-x-y+z-1)/(2*(1-z))",
  "(-x+y+z-1)*(-x-y+z-1)*z/(1-z)",
  "(-x-y+z-1)*( x-y+z-1)*z/(1-z)",
  "( x-y+z-1)*( x+y+z-1)*z/(1-z)",
  "( x+y+z-1)*(-x+y+z-1)*z/(1-z)"
};

vtkCxxSetObjectMacro(vtkMedLocalization, ParentFile, vtkMedFile);
vtkCxxSetObjectMacro(vtkMedLocalization, Interpolation, vtkMedInterpolation);

vtkStandardNewMacro(vtkMedLocalization)

vtkMedLocalization::vtkMedLocalization()
{
  this->GeometryType = MED_NONE;
  this->NumberOfQuadraturePoint = 0;
  this->Weights = vtkDoubleArray::New();
  this->PointLocalCoordinates = vtkDoubleArray::New();
  this->QuadraturePointLocalCoordinates = vtkDoubleArray::New();
  this->ShapeFunction = vtkDoubleArray::New();
  this->Name = NULL;
  this->SectionName = NULL;
  this->InterpolationName = NULL;
  this->MedIterator = -1;
  this->ParentFile = NULL;
  this->SpaceDimension = 3;
  this->NumberOfCellInSection = 0;
  this->SectionGeometryType = MED_NONE;
  this->Interpolation = NULL;
  this->ShapeFunctionIsBuilt = 0;
}

vtkMedLocalization::~vtkMedLocalization()
{
  this->SetName(NULL);
  this->SetSectionName(NULL);
  this->SetInterpolationName(NULL);
  this->Weights->Delete();
  this->PointLocalCoordinates->Delete();
  this->QuadraturePointLocalCoordinates->Delete();
  this->ShapeFunction->Delete();
  this->SetInterpolation(NULL);

}

int vtkMedLocalization::GetSizeOfWeights()
{
  return this->NumberOfQuadraturePoint;
}

int vtkMedLocalization::GetSizeOfPointLocalCoordinates()
{
  return vtkMedUtilities::GetNumberOfPoint(this->GeometryType)
      * vtkMedUtilities::GetDimension(this->GeometryType);
}

int vtkMedLocalization::GetSizeOfQuadraturePointLocalCoordinates()
{
  return this->NumberOfQuadraturePoint * vtkMedUtilities::GetDimension(
      this->GeometryType);
}

int vtkMedLocalization::GetSizeOfShapeFunction()
{
  return this->NumberOfQuadraturePoint * vtkMedUtilities::GetNumberOfPoint(
      this->GeometryType);
}

void vtkMedLocalization::BuildShapeFunction()
{
  if(this->ShapeFunctionIsBuilt)
    return;

  if(this->Interpolation == NULL)
    {
    // If there is no interpolation given for this localization,
    // I build the default aster shape function

    switch (this->GeometryType)
    {
      case MED_POINT1:
        BuildPoint1();
        return;
      case MED_SEG2:
        BuildAsterShapeFunction(SEG2_dim, SEG2_nnode,
                           (const int *) SEG2_aster2med,
                           (const char**)SEG2_varnames,
                           (const char**)SEG2_functions);
        break;
      case MED_SEG3:
        BuildAsterShapeFunction(SEG3_dim, SEG3_nnode,
                           (const int *) SEG3_aster2med,
                           (const char**)SEG3_varnames,
                           (const char**)SEG3_functions);
        break;
      case MED_SEG4:
        BuildAsterShapeFunction(SEG4_dim, SEG4_nnode,
                           (const int *) SEG4_aster2med,
                           (const char**)SEG4_varnames,
                           (const char**)SEG4_functions);
        break;
      case MED_TRIA3:
        BuildAsterShapeFunction(TRIA3_dim, TRIA3_nnode,
                           (const int *) TRIA3_aster2med,
                           (const char**)TRIA3_varnames,
                           (const char**)TRIA3_functions);
        break;
      case MED_TRIA6:
        BuildAsterShapeFunction(TRIA6_dim, TRIA6_nnode,
                           (const int *) TRIA6_aster2med,
                           (const char**)TRIA6_varnames,
                           (const char**)TRIA6_functions);
        break;
      case MED_TRIA7:
        BuildAsterShapeFunction(TRIA7_dim, TRIA7_nnode,
                           (const int *) TRIA7_aster2med,
                           (const char**)TRIA7_varnames,
                           (const char**)TRIA7_functions);
        break;
      case MED_QUAD4:
        BuildAsterShapeFunction(QUAD4_dim, QUAD4_nnode,
                           (const int *) QUAD4_aster2med,
                           (const char**)QUAD4_varnames,
                           (const char**)QUAD4_functions);
        break;
      case MED_QUAD8:
        BuildAsterShapeFunction(QUAD8_dim, QUAD8_nnode,
                           (const int *) QUAD8_aster2med,
                           (const char**)QUAD8_varnames,
                           (const char**)QUAD8_functions);
        break;
      case MED_QUAD9:
        BuildAsterShapeFunction(QUAD9_dim, QUAD9_nnode,
                           (const int *) QUAD9_aster2med,
                           (const char**)QUAD9_varnames,
                           (const char**)QUAD9_functions);
        break;
      case MED_HEXA8:
        BuildAsterShapeFunction(HEXA8_dim, HEXA8_nnode,
                           (const int *) HEXA8_aster2med,
                           (const char**)HEXA8_varnames,
                           (const char**)HEXA8_functions);
        break;
      case MED_HEXA20:
        BuildAsterShapeFunction(HEXA20_dim, HEXA20_nnode,
                           (const int *) HEXA20_aster2med,
                           (const char**)HEXA20_varnames,
                           (const char**)HEXA20_functions);
        break;
      case MED_HEXA27:
        BuildAsterShapeFunction(HEXA27_dim, HEXA27_nnode,
                           (const int *) HEXA27_aster2med,
                           (const char**)HEXA27_varnames,
                           (const char**)HEXA27_functions);
        break;
      case MED_TETRA4:
        BuildAsterShapeFunction(TETRA4_dim, TETRA4_nnode,
                           (const int *) TETRA4_aster2med,
                           (const char**)TETRA4_varnames,
                           (const char**)TETRA4_functions);
        break;
      case MED_TETRA10:
        BuildAsterShapeFunction(TETRA10_dim, TETRA10_nnode,
                           (const int *) TETRA10_aster2med,
                           (const char**)TETRA10_varnames,
                           (const char**)TETRA10_functions);
        break;
      case MED_PENTA6:
        BuildAsterShapeFunction(PENTA6_dim, PENTA6_nnode,
                           (const int *) PENTA6_aster2med,
                           (const char**)PENTA6_varnames,
                           (const char**)PENTA6_functions);
        break;
      case MED_PENTA15:
        BuildAsterShapeFunction(PENTA15_dim, PENTA15_nnode,
                           (const int *) PENTA15_aster2med,
                           (const char**)PENTA15_varnames,
                           (const char**)PENTA15_functions);
        break;
      case MED_PYRA5:
        BuildAsterShapeFunction(PYRA5_dim, PYRA5_nnode,
                           (const int *) PYRA5_aster2med,
                           (const char**)PYRA5_varnames,
                           (const char**)PYRA5_functions);
        break;
      case MED_PYRA13:
        BuildAsterShapeFunction(PYRA13_dim, PYRA13_nnode,
                           (const int *) PYRA13_aster2med,
                           (const char**)PYRA13_varnames,
                           (const char**)PYRA13_functions);
        break;
      default:
        vtkErrorMacro("ERROR in vtkMedLocalization::BuildShapeFunction. "
                      << this->GeometryType
                      << " : Cell geometry not supported !!! ");
        return;
      }
    }
  else
    {
    this->BuildShapeFunctionFromInterpolation();
    }
  this->ShapeFunctionIsBuilt = 1;
}

void  vtkMedLocalization::BuildShapeFunctionFromInterpolation()
{
  int nnodes = this->GeometryType % 100;
  int dim = this->GeometryType / 100;
  this->ShapeFunction->SetNumberOfValues(this->GetSizeOfShapeFunction());

  int qpindex;
  int nodeindex;

  vtkMedFraction* func;

  switch(dim)
    {
    case 0 :
      this->ShapeFunction->SetValue(0, 1);
      break;
    default :
      for(qpindex=0; qpindex < this->NumberOfQuadraturePoint; qpindex++ )
        {
        double *coord = new double[dim];
        for(int dimid=0; dimid<dim; dimid++)
          {
          coord[dimid] = this->QuadraturePointLocalCoordinates
                         ->GetValue((qpindex * dim)+dimid);
          }

        for(nodeindex=0; nodeindex < nnodes; nodeindex++)
          {
          func = this->Interpolation->GetBasisFunction(nodeindex);
          this->ShapeFunction->SetValue(
              qpindex*nnodes + nodeindex, func->Evaluate(coord));
          }
        }
    }
}

void  vtkMedLocalization::BuildAsterShapeFunction(int dim,
                                 int nnodes,
                                 const int* aster2med,
                                 const char** varnames,
                                 const char** functions)
{
  this->ShapeFunction->SetNumberOfValues(
      this->NumberOfQuadraturePoint * nnodes);

  std::vector<vtkSmartPointer<vtkFunctionParser> > parsers;
  parsers.resize(nnodes);
  for(int nodeindex=0; nodeindex < nnodes; nodeindex++)
    {
    parsers[nodeindex] = vtkSmartPointer<vtkFunctionParser>::New();
    parsers[nodeindex]->SetFunction(functions[nodeindex]);
    }

  for(int qpindex=0; qpindex < this->NumberOfQuadraturePoint; qpindex++ )
    {

    for(int nodeindex=0; nodeindex < nnodes; nodeindex++)
      {
      int mednodeindex = aster2med[nodeindex];
      vtkFunctionParser* parser = parsers[mednodeindex];
      for(int dimid=0; dimid<dim; dimid++)
        {
        const char* varname = varnames[dimid];
        const double coord = this->QuadraturePointLocalCoordinates
                             ->GetValue((qpindex * dim)+dimid);

        parser->SetScalarVariableValue(varname, coord);
        }

      double w = parser->GetScalarResult();

      this->ShapeFunction->SetValue(
          qpindex*nnodes + mednodeindex, w);
      }
    }
}

void vtkMedLocalization::BuildPoint1()
{
  this->Weights->SetNumberOfValues(1);
  this->ShapeFunction->SetNumberOfValues(1);
  this->Weights->SetValue(0, 1);
  this->ShapeFunction->SetValue(0, 1);
}

void vtkMedLocalization::BuildCenter(med_geometry_type geometry)
{
  this->GeometryType = geometry;
  this->NumberOfQuadraturePoint = 1;
  int npts = vtkMedUtilities::GetNumberOfPoint(this->GeometryType);
  this->ShapeFunction->SetNumberOfValues(npts);
  this->Weights->SetNumberOfValues(1);
  for (int i = 0; i < npts; i++)
    {
    this->ShapeFunction->SetValue(i, 1.0 / (double) npts);
    }
  this->Weights->SetValue(0, 1);

}

void vtkMedLocalization::BuildELNO(med_geometry_type geometry)
{
  this->GeometryType = geometry;
  this->NumberOfQuadraturePoint = vtkMedUtilities::GetNumberOfPoint(geometry);

  int np2 = this->NumberOfQuadraturePoint * this->NumberOfQuadraturePoint;
  this->ShapeFunction->SetNumberOfValues(np2);
  this->Weights->SetNumberOfValues(this->NumberOfQuadraturePoint);

  for (int i = 0; i < np2; i++)
    {
    this->ShapeFunction->SetValue(i, 0);
    }
  for (int i = 0; i < this->NumberOfQuadraturePoint; i++)
    {
    this->ShapeFunction->SetValue(i + i * this->NumberOfQuadraturePoint, 1.0);
    }
  double w = 1.0 / (double) this->NumberOfQuadraturePoint;
  for (int i = 0; i < this->NumberOfQuadraturePoint; i++)
    {
    this->Weights->SetValue(i, w);
    }
}

void vtkMedLocalization::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  PRINT_IVAR(os, indent, GeometryType);
  PRINT_IVAR(os, indent, NumberOfQuadraturePoint);
  PRINT_IVAR(os, indent, MedIterator);
}
