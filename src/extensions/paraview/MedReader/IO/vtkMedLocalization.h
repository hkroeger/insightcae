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

#ifndef __vtkMedLocalization_h_
#define __vtkMedLocalization_h_

#include "vtkObject.h"
#include "vtkMed.h"
#include "vtkSmartPointer.h"

class vtkMedString;
class vtkMedFile;
class vtkDoubleArray;
class vtkMedInterpolation;

class VTK_EXPORT vtkMedLocalization : public vtkObject
{
public:
  static vtkMedLocalization* New();
  vtkTypeMacro(vtkMedLocalization, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the name of this definition
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // This is the name of this definition
  vtkSetStringMacro(SectionName);
  vtkGetStringMacro(SectionName);

  // Description:
  // This is the name of this definition
  vtkSetStringMacro(InterpolationName);
  vtkGetStringMacro(InterpolationName);

  // Description:
  // This is the type of cell geometry this definition is for.
  vtkSetMacro(GeometryType, med_geometry_type);
  vtkGetMacro(GeometryType, med_geometry_type);

  // Description:
  // This is the number of quadrature points in this definition.
  vtkSetMacro(NumberOfQuadraturePoint, int);
  vtkGetMacro(NumberOfQuadraturePoint, int);

  // Description:
  // The index of this field in the med file
  vtkSetMacro(MedIterator, med_int);
  vtkGetMacro(MedIterator, med_int);

  // Description:
  // The dimension of the space in which integration points are defined.
  vtkSetMacro(SpaceDimension, med_int);
  vtkGetMacro(SpaceDimension, med_int);

  // Description:
  // The number of cell in a section for structural elements
  vtkGetMacro(NumberOfCellInSection, med_int);
  vtkSetMacro(NumberOfCellInSection, med_int);

  // Description:
  // Type of cells that define the section, for structural elements.
  vtkSetMacro(SectionGeometryType, med_geometry_type);
  vtkGetMacro(SectionGeometryType, med_geometry_type);

  // Description:
  // get the raw pointers to the internal arrays.
  // Those arrays are allocated in SecureResources,
  // and cleared in ClearResources
  vtkGetObjectMacro(Weights, vtkDoubleArray);
  vtkGetObjectMacro(PointLocalCoordinates, vtkDoubleArray);
  vtkGetObjectMacro(QuadraturePointLocalCoordinates, vtkDoubleArray);
  vtkGetObjectMacro(ShapeFunction, vtkDoubleArray);

  virtual int GetSizeOfWeights();
  virtual int GetSizeOfPointLocalCoordinates();
  virtual int GetSizeOfQuadraturePointLocalCoordinates();
  virtual int GetSizeOfShapeFunction();

  virtual void BuildShapeFunction();
  virtual void  BuildELNO(med_geometry_type geometry);
  virtual void  BuildCenter(med_geometry_type geometry);

  virtual void  SetParentFile(vtkMedFile*);
  vtkGetObjectMacro(ParentFile, vtkMedFile);

  // Description:
  // The interpolation is the function that define how the interpolate
  // value at integration points from values at reference points (usually nodes)
  virtual void  SetInterpolation(vtkMedInterpolation*);
  vtkGetObjectMacro(Interpolation, vtkMedInterpolation);

protected:
  vtkMedLocalization();
  virtual ~vtkMedLocalization();

  med_int MedIterator;
  med_geometry_type GeometryType;
  int NumberOfQuadraturePoint;
  med_int SpaceDimension;
  med_int NumberOfCellInSection;
  med_geometry_type SectionGeometryType;

  vtkDoubleArray* Weights;
  vtkDoubleArray* PointLocalCoordinates;
  vtkDoubleArray* QuadraturePointLocalCoordinates;
  vtkDoubleArray* ShapeFunction;

  char* Name;
  char* SectionName;
  char* InterpolationName;

  vtkMedFile* ParentFile;

  vtkMedInterpolation* Interpolation;

  int ShapeFunctionIsBuilt;

  virtual void  BuildAsterShapeFunction(int dim,
                                   int nnodes,
                                   const int* med2aster,
                                   const char** varnames,
                                   const char** functions);

  virtual void BuildPoint1();

  virtual void  BuildShapeFunctionFromInterpolation();

private:
  vtkMedLocalization(const vtkMedLocalization&);
     // Not implemented.
  void operator=(const vtkMedLocalization&);
     // Not implemented.
};

#endif //__vtkMedQuadratureDefinition_h_
