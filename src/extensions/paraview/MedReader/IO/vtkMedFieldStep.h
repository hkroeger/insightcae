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

#ifndef __vtkMedFieldStep_h_
#define __vtkMedFieldStep_h_

#include "vtkObject.h"
#include "vtkMedSetGet.h"
#include "vtkMed.h"
#include "vtkMedUtilities.h"

class vtkMedString;
class vtkMedMesh;
class vtkDataArray;
class vtkMedFieldOverEntity;
class vtkMedField;

class VTK_EXPORT vtkMedFieldStep: public vtkObject
{
public:
  static vtkMedFieldStep* New();
  vtkTypeMacro(vtkMedFieldStep, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This iterator is used when reading information from the med file
  vtkSetMacro(MedIterator, med_int);
  vtkGetMacro(MedIterator, med_int);

  // Description:
  // The compute step of this field
  void  SetComputeStep(const vtkMedComputeStep& cs)
    {
    this->ComputeStep = cs;
    }
  const vtkMedComputeStep& GetComputeStep() const
    {
    return this->ComputeStep;
    }

  // Description:
  // The compute step of the mesh supporting this field at this step
  void  SetMeshComputeStep(const vtkMedComputeStep& cs)
    {
    this->MeshComputeStep = cs;
    }
  const vtkMedComputeStep& GetMeshComputeStep() const
    {
    return this->MeshComputeStep;
    }

  // Description:
  // Set the number of steps of this field over these cells.
  vtkGetObjectVectorMacro(FieldOverEntity, vtkMedFieldOverEntity);
  vtkSetObjectVectorMacro(FieldOverEntity, vtkMedFieldOverEntity);

  // Description:
  // returns the vtkMedFieldOverEntity for the given Type and Geometry;
  virtual vtkMedFieldOverEntity*
          GetFieldOverEntity(const vtkMedEntity&);

  // Description:
  // The parent field is the one that owns this step
  virtual void  SetParentField(vtkMedField*);
  vtkGetObjectMacro(ParentField, vtkMedField);

  // Description:
  // The parent field is the one that owns this step
  virtual void  SetPreviousStep(vtkMedFieldStep*);
  vtkGetObjectMacro(PreviousStep, vtkMedFieldStep);

  virtual void  LoadInformation();

  // Description:
  // This flag is used to delay loading information on all entity as long as possible
  vtkSetMacro(EntityInfoLoaded, med_int);
  vtkGetMacro(EntityInfoLoaded, med_int);

protected:
  vtkMedFieldStep();
  virtual ~vtkMedFieldStep();

  int EntityInfoLoaded;
  med_int MedIterator;
  vtkMedComputeStep ComputeStep;
  vtkMedComputeStep MeshComputeStep;
  vtkMedField* ParentField;
  vtkMedFieldStep* PreviousStep;

  //BTX
  vtkObjectVector<vtkMedFieldOverEntity>* FieldOverEntity;
  //ETX

private:
  vtkMedFieldStep(const vtkMedFieldStep&); // Not implemented.
  void operator=(const vtkMedFieldStep&); // Not implemented.
};

#endif //__vtkMedFieldStep_h_
