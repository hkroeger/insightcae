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

#ifndef __vtkMedDriver_h_
#define __vtkMedDriver_h_

#include "vtkObject.h"
#include "vtkMed.h"

class vtkMedFile;
class vtkMedMesh;
class vtkMedField;
class vtkMedFamily;
class vtkMedPolarGrid;
class vtkMedRegularGrid;
class vtkMedCurvilinearGrid;
class vtkMedUnstructuredGrid;
class vtkMedLocalization;
class vtkMedProfile;
class vtkMedFieldOverEntity;
class vtkMedEntityArray;
class vtkMedFieldStep;
class vtkMedGrid;
class vtkMedInterpolation;
class vtkMedFieldOnProfile;
class vtkMedStructElement;
class vtkMedConstantAttribute;
class vtkMedVariableAttribute;

class VTK_EXPORT vtkMedDriver: public vtkObject
{
public:
  static vtkMedDriver* New();
  vtkTypeMacro(vtkMedDriver, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the med file that use this driver
  virtual void  SetMedFile(vtkMedFile*);
  vtkGetObjectMacro(MedFile, vtkMedFile);

  // Description :
  // This gives the File ID to read in parallel.
  vtkGetMacro(ParallelFileId, med_idt);

  // Description:
  // open the file for reading. Returns 0 on success, or error code.
  virtual int RestrictedOpen();
  virtual int Open();
  virtual void Close();
  virtual bool CanReadFile();

  // Description:
  // Get the Version numbers from the file
  virtual void ReadFileVersion(int* major, int* minor, int* release);

  // Description:
  // load all meta data associated with this file.
  virtual void ReadFileInformation(vtkMedFile*);

  // Description:
  // load all meta data associated with this mesh.
  virtual void ReadMeshInformation(vtkMedMesh*);

  // Description:
  // load all meta data associated with this field.
  virtual void ReadFieldInformation(vtkMedField*);

  // Description:
  // load all meta data associated with this regular (Cartesian or polar) grid.
  virtual void ReadRegularGridInformation(vtkMedRegularGrid*);

  // Description:
  // load all meta data associated with this standard grid.
  virtual void ReadCurvilinearGridInformation(vtkMedCurvilinearGrid*);

  // Description:
  // load all meta data associated with this unstructured grid.
  virtual void ReadUnstructuredGridInformation(vtkMedUnstructuredGrid*);

  // Description:
  // load all meta data associated with this grid.
  // (call one of the above method depending on the type of grid)
  virtual void ReadGridInformation(vtkMedGrid*);

  // Description:
  // load all meta data associated with this family.
  virtual void ReadFamilyInformation(vtkMedMesh*, vtkMedFamily*);

  // Description:
  // load all meta data associated with this family.
  virtual void ReadProfileInformation(vtkMedProfile*);

  // Description:
  // load information on a field and a given cell type at a given step
  virtual void ReadFieldOverEntityInformation(vtkMedFieldOverEntity*);

  // Description:
  // load information related to the given quadrature scheme definition
  virtual void ReadLocalizationInformation(vtkMedLocalization*);

  // Description:
  // load all information associated witht this field step
  virtual void ReadFieldStepInformation(vtkMedFieldStep*, bool readAllEntityInfo);

  // Description:
  // load all information related to this interpolation function
  virtual void ReadInterpolationInformation(vtkMedInterpolation*);

  // Description:
  // load all information related to this field on this profile
  virtual void ReadFieldOnProfileInformation(vtkMedFieldOnProfile*);

  // Description:
  // read all information related to the structural elements models
  // in this file
  virtual void ReadStructElementInformation(vtkMedStructElement*);

  // Description:
  // read all information related to the support mesh
  // in this file
  virtual void ReadSupportMeshInformation(vtkMedMesh*);

  // Description:
  // read all information related to the support mesh
  // in this file
  virtual void ReadConstantAttributeInformation(vtkMedConstantAttribute*);

  // Description:
  // read all information related to the support mesh
  // in this file
  virtual void ReadVariableAttributeInformation(vtkMedVariableAttribute*);

  // Description:
  // load the field on this profile
  virtual void LoadField(vtkMedFieldOnProfile* fop, med_storage_mode mode);

  // Description:
  // Load the indices of the profile
  virtual void LoadProfile(vtkMedProfile* profile);

  // Description:
  // Load the values of the given step
//  void Load(vtkMedFieldOnProfile*);

  virtual void LoadFamilyIds(vtkMedEntityArray*);

  virtual void LoadCoordinates(vtkMedGrid*);

  virtual void LoadPointGlobalIds(vtkMedGrid*);

  //virtual void LoadPointFamilyIds(vtkMedGrid*);

  virtual void LoadConnectivity(vtkMedEntityArray*);

  virtual void LoadCellGlobalIds(vtkMedEntityArray*);

  virtual void  LoadVariableAttribute(vtkMedVariableAttribute*,
                                      vtkMedEntityArray*);

  //BTX
  class FileOpen
  {
  public:
    FileOpen(vtkMedDriver* driver)
    {
      this->Driver = driver;
      this->Driver->Open();
    }
    ~FileOpen()
    {
      this->Driver->Close();
    }
  protected:
    vtkMedDriver* Driver;
    vtkMedFile* File;
  };
  //ETX

  //BTX
  class FileRestrictedOpen
  {
  public:
    FileRestrictedOpen(vtkMedDriver* driver)
    {
      this->Driver = driver;
      this->Driver->RestrictedOpen();
    }
    ~FileRestrictedOpen()
    {
      this->Driver->Close();
    }
  protected:
    vtkMedDriver* Driver;
    vtkMedFile* File;
  };
  //ETX

protected:
  vtkMedDriver();
  ~vtkMedDriver();

  // name of the file to read from
  vtkMedFile * MedFile;

  int OpenLevel;

  med_idt FileId;
  med_idt ParallelFileId;

private:
  vtkMedDriver(const vtkMedDriver&); // Not implemented.
  void operator=(const vtkMedDriver&); // Not implemented.
};

#endif //__vtkMedDriver_h_
