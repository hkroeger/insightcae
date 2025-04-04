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

#ifndef __vtkMedDriver30_h_
#define __vtkMedDriver30_h_

#include "vtkObject.h"
#include "vtkMed.h"
#include "vtkMedDriver.h"

class vtkMedFile;
class vtkMedMesh;
class vtkMedField;
class vtkMedFamily;
class vtkMedRegularGrid;
class vtkMedCurvilinearGrid;
class vtkMedUnstructuredGrid;
class vtkMedLocalization;
class vtkMedProfile;
class vtkMedFieldOverEntity;
class vtkMedEntityArray;
class vtkMedFieldStep;
class vtkMedLink;
class vtkMedStructElement;
class vtkMedConstantAttribute;
class vtkMedVariableAttribute;

class VTK_EXPORT vtkMedDriver30: public vtkMedDriver
{
public:
  static vtkMedDriver30* New();
  vtkTypeMacro(vtkMedDriver30, vtkMedDriver)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // load all meta data associated with this file.
  virtual void ReadFileInformation(vtkMedFile*) override;

  // Description:
  // load all meta data associated with this mesh.
  virtual void ReadMeshInformation(vtkMedMesh*) override;

  // Description:
  // load all meta data associated with this field.
  virtual void ReadFieldInformation(vtkMedField*) override;

  // Description:
  // load all meta data associated with this regular (Cartesian or polar) grid.
  virtual void ReadRegularGridInformation(vtkMedRegularGrid*) override;

  // Description:
  // load all meta data associated with this standard grid.
  virtual void ReadCurvilinearGridInformation(vtkMedCurvilinearGrid*);

  // Description:
  // load all meta data associated with this unstructured grid.
  virtual void ReadUnstructuredGridInformation(vtkMedUnstructuredGrid*) override;

  // Description:
  // load all meta data associated with this family.
  virtual void ReadFamilyInformation(vtkMedMesh*, vtkMedFamily*) override;

  // Description:
  // load all meta data associated with this family.
  virtual void ReadProfileInformation(vtkMedProfile*) override;

  // Description:
  // load information on a field and a given cell type at a given step
  virtual void ReadFieldOverEntityInformation(vtkMedFieldOverEntity*) override;

  // Description:
  // load information related to the given quadrature scheme definition
  virtual void ReadLocalizationInformation(vtkMedLocalization*) override;

  // Description:
  // load all information associated witht this field step
  virtual void ReadFieldStepInformation(vtkMedFieldStep*, bool readAllEntityInfo) override;

  // Description:
  // load all information related to this interpolation function
  virtual void ReadInterpolationInformation(vtkMedInterpolation*) override;

  // Description:
  // load all information related to this field on this profile
  virtual void ReadFieldOnProfileInformation(vtkMedFieldOnProfile*) override;

  // Description:
  // load all information related to this field on this profile
  virtual void ReadLinkInformation(vtkMedLink*);

  // Description:
  // read all information related to the structural elements models
  // in this file
  virtual void ReadStructElementInformation(vtkMedStructElement*) override;

  // Description:
  // read all information related to the support mesh
  // in this file
  virtual void ReadSupportMeshInformation(vtkMedMesh*) override;

  // Description:
  // read all information related to the support mesh
  // in this file
  virtual void ReadConstantAttributeInformation(vtkMedConstantAttribute*) override;

  // Description:
  // read all information related to the support mesh
  // in this file
  virtual void ReadVariableAttributeInformation(vtkMedVariableAttribute*) override;

  // Description:
  // load the field on this profile
  virtual void LoadField(vtkMedFieldOnProfile*, med_storage_mode mode) override;

  // Description:
  // Load the indices of the profile
  virtual void LoadProfile(vtkMedProfile* profile) override;

  // Description:
  // Load the values of the given step
//  void Load(vtkMedFieldOnProfile*);

  virtual void LoadFamilyIds(vtkMedEntityArray*) override;

  virtual void LoadCoordinates(vtkMedGrid*) override;

  virtual void LoadPointGlobalIds(vtkMedGrid*) override;

  //virtual void LoadFamilyIds(vtkMedGrid*);

  virtual void LoadConnectivity(vtkMedEntityArray*) override;

  virtual void LoadCellGlobalIds(vtkMedEntityArray*) override;

  virtual void LoadRegularGridCoordinates(vtkMedRegularGrid*);

  virtual void  LoadVariableAttribute(vtkMedVariableAttribute*,
                                      vtkMedEntityArray*) override;

protected:
  vtkMedDriver30();
  ~vtkMedDriver30();

  void ReadNumberOfEntity(vtkMedUnstructuredGrid*,
      med_entity_type, med_connectivity_mode);

private:
  vtkMedDriver30(const vtkMedDriver30&); // Not implemented.
  void operator=(const vtkMedDriver30&); // Not implemented.
};

#endif //__vtkMedDriver30_h_
