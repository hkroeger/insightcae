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

#ifndef __vtkMedField_h_
#define __vtkMedField_h_

#include "vtkObject.h"
#include "vtkMedSetGet.h"
#include "vtkMed.h"

#include "vtkSmartPointer.h"

#include <set>

class vtkStringArray;

class vtkMedInterpolation;
class vtkMedFieldOverEntity;
class vtkMedString;
class vtkMedFieldStep;
class vtkMedComputeStep;
template <class T>
class vtkMedComputeStepMap;
class vtkMedFile;

class VTK_EXPORT vtkMedField: public vtkObject
{
public:
  static vtkMedField* New();
  vtkTypeMacro(vtkMedField, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The number of component of this field
  virtual void  SetNumberOfComponent(int);
  vtkGetMacro(NumberOfComponent, int);

  // Description:
  // The type of data stored in this field
  vtkSetMacro(DataType, med_field_type);
  vtkGetMacro(DataType, med_field_type);

  // Description:
  // The name of this field
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // The name of this mesh this field is on
  vtkGetStringMacro(MeshName);
  vtkSetStringMacro(MeshName);

  // Description:
  // The name of this mesh this field is on
  vtkGetStringMacro(TimeUnit);
  vtkSetStringMacro(TimeUnit);

  // Description:
  // The units of each component of this field
  vtkGetObjectMacro(Unit, vtkStringArray);

  // Description:
  // The name of each component of this field
  vtkGetObjectMacro(ComponentName, vtkStringArray);

  // Description:
  // add a cell type as support to this field
  void  AddFieldStep(vtkMedFieldStep*);
  void  ClearFieldStep();
  vtkMedFieldStep* GetFieldStep(const vtkMedComputeStep&);
  vtkMedFieldStep* FindFieldStep(const vtkMedComputeStep&, int);
  med_int GetNumberOfFieldStep();
  vtkMedFieldStep* GetFieldStep(med_int);
  void  GatherFieldTimes(std::set<med_float>&);
  void  GatherFieldIterations(med_float,std::set<med_int>&);

  // Description:
  // returns if the field is on point, cell, quadrature point or elno
  //BTX
  enum {
    UnknownFieldType = 0x00,
    PointField = 0x01,
    CellField = 0x02,
    QuadratureField = 0x04,
    ElnoField = 0x08};
  //ETX
  //Description:
  // returns the type of field this is. The returned code is and OR between
  // the different possible types.
  vtkGetMacro(FieldType, int);

  // This computes the FieldType
  // (currently, it does it by looking only at the first compute step)
  virtual void  ComputeFieldType();

  // Description:
  // This returns true if the FieldType is composed of several types
  virtual int HasManyFieldTypes();

  // Description:
  // returns the first support type this field is on.
  virtual int GetFirstType();

  // Description:
  // This methods extracts from the other field all the fields that are
  // on the given support type and add them to the current field.
  // It also updates the other FieldType ivar.
  virtual void  ExtractFieldType(vtkMedField* otherfield, int type);

  // Description:
  // The index of this field in the med file
  vtkSetMacro(MedIterator, med_int);
  vtkGetMacro(MedIterator, med_int);

  // Description:
  // if the mesh is local or not.
  vtkSetMacro(Local, med_int);
  vtkGetMacro(Local, med_int);

  // Description:
  // The interpolation functions associated with this field
  vtkGetObjectVectorMacro(Interpolation, vtkMedInterpolation);
  vtkSetObjectVectorMacro(Interpolation, vtkMedInterpolation);

  // Description:
  // This stores the file this field is stored on.
  virtual void  SetParentFile(vtkMedFile*);
  vtkGetObjectMacro(ParentFile, vtkMedFile);

protected:
  vtkMedField();
  virtual ~vtkMedField();

  vtkSetMacro(FieldType, int);

  int NumberOfComponent;
  med_field_type DataType;
  med_int MedIterator;
  med_int Local;
  char* Name;
  char* MeshName;
  char* TimeUnit;
  int FieldType;
  vtkMedFile* ParentFile;

  //BTX
  vtkMedComputeStepMap<vtkMedFieldStep>* FieldStep;
  vtkObjectVector<vtkMedInterpolation>* Interpolation;
  //ETX

  vtkStringArray* Unit;
  vtkStringArray* ComponentName;

private:
  vtkMedField(const vtkMedField&); // Not implemented.
  void operator=(const vtkMedField&); // Not implemented.

};

#endif //__vtkMedField_h_
