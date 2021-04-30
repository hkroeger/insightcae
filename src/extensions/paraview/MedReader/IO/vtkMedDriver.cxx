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

#include "vtkMedDriver.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"

#include "vtkMedFile.h"
#include "vtkMedCartesianGrid.h"
#include "vtkMedPolarGrid.h"
#include "vtkMedCurvilinearGrid.h"
#include "vtkMedUnstructuredGrid.h"
#include "vtkMedField.h"
#include "vtkMedMesh.h"
#include "vtkMedFamily.h"
#include "vtkMedUtilities.h"
#include "vtkMedEntityArray.h"
#include "vtkMedLocalization.h"
#include "vtkMedProfile.h"
#include "vtkMedFieldOverEntity.h"
#include "vtkMedFieldStep.h"
#include "vtkMedGroup.h"
#include "vtkMedIntArray.h"
#include "vtkMedLink.h"

#ifdef MedReader_HAVE_PARALLEL_INFRASTRUCTURE
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include <vtkMPICommunicator.h>
#include <vtkMPI.h>
#endif

vtkCxxSetObjectMacro(vtkMedDriver, MedFile, vtkMedFile);

vtkStandardNewMacro(vtkMedDriver)

vtkMedDriver::vtkMedDriver()
{
  this->MedFile = NULL;
  this->OpenLevel = 0;
  this->FileId = -1;
}

vtkMedDriver::~vtkMedDriver()
{
  if (this->OpenLevel > 0)
    {
    vtkWarningMacro("The file has not be closed before destructor.");
    this->OpenLevel = 1;
    this->Close();
    }
  this->SetMedFile(NULL);
}

int vtkMedDriver::RestrictedOpen()
{
  int res = 0;
  if (this->MedFile == NULL || this->MedFile->GetFileName() == NULL)
    {
    vtkDebugMacro("Error : FileName has not been set ");
    return -1;
    }

  if (this->OpenLevel <= 0)
    {

    med_bool hdfok;
    med_bool medok;

    med_err conforme = MEDfileCompatibility(this->MedFile->GetFileName(),
                                            &hdfok, &medok);
    if (!hdfok)
      {
      vtkErrorMacro("The file " << this->MedFile->GetFileName()
          << " is not a HDF5 file, aborting.");
      return -1;
      }

    if (!medok)
      {
      vtkErrorMacro("The file " << this->MedFile->GetFileName()
          << " has not been written with the"
          << " same version as the one currently used to read it, this may lead"
          << " to errors. Please use the medimport tool.");
      return -1;
      }

    if(conforme < 0)
      {
      vtkErrorMacro("The file " << this->MedFile->GetFileName()
                    << " is not compatible, please import it to the new version using medimport.");
      return -1;
      }

    this->FileId = MEDfileOpen(this->MedFile->GetFileName(), MED_ACC_RDONLY);
    if (this->FileId < 0)
      {
      vtkDebugMacro("Error : unable to open file "
                    << this->MedFile->GetFileName());
      res = -2;
      }
    this->OpenLevel = 0;

    } // OpenLevel
  this->OpenLevel++;
  this->ParallelFileId = -1;
  return res;
}

int vtkMedDriver::Open()
{
  int res = 0;
  if (this->MedFile == NULL || this->MedFile->GetFileName() == NULL)
    {
    vtkDebugMacro("Error : FileName has not been set ");
    return -1;
    }

  if (this->OpenLevel <= 0)
    {

    med_bool hdfok;
    med_bool medok;

    med_err conforme = MEDfileCompatibility(this->MedFile->GetFileName(),
                                            &hdfok, &medok);
    if (!hdfok)
      {
      vtkErrorMacro("The file " << this->MedFile->GetFileName()
          << " is not a HDF5 file, aborting.");
      return -1;
      }

    if (!medok)
      {
      vtkErrorMacro("The file " << this->MedFile->GetFileName()
          << " has not been written with the"
          << " same version as the one currently used to read it, this may lead"
          << " to errors. Please use the medimport tool.");
      return -1;
      }

    if(conforme < 0)
      {
      vtkErrorMacro("The file " << this->MedFile->GetFileName()
                    << " is not compatible, please import it to the new version using medimport.");
      return -1;
      }

    this->FileId = MEDfileOpen(this->MedFile->GetFileName(), MED_ACC_RDONLY);
    if (this->FileId < 0)
      {
      vtkDebugMacro("Error : unable to open file "
                    << this->MedFile->GetFileName());
      res = -2;
      }
    this->OpenLevel = 0;

    this->ParallelFileId = -1;

#ifdef MedReader_HAVE_PARALLEL_INFRASTRUCTURE
    // the following code opens the file in parallel
    vtkMultiProcessController* controller =
    		vtkMultiProcessController::GetGlobalController();
    int lpID = 0;
    if (controller == NULL)
      {
    return -3;
      }
    else
      {
      lpID = controller->GetLocalProcessId();
      }

    vtkMPICommunicator* commu = vtkMPICommunicator::SafeDownCast(
                  controller->GetCommunicator() );
    if (commu == NULL)
      {
      //vtkErrorMacro("Communicator is NULL in Open");
      return -3;
      }
    MPI_Comm* mpi_com = NULL;
    mpi_com = commu->GetMPIComm()->GetHandle();
    if (mpi_com == NULL)
      {
      vtkErrorMacro("MPI communicator is NULL in Open");
      return -3;
      }

    if (controller->GetNumberOfProcesses() > 1)
      {
      int major, minor, release;
      if (MEDfileNumVersionRd(this->FileId, &major, &minor, &release) < 0)
        {
        vtkErrorMacro("Impossible to read the version of this file");
        return -1;
        }

    if (major >= 3)
      {
        this->ParallelFileId = MEDparFileOpen(this->MedFile->GetFileName(),
                            MED_ACC_RDONLY,
                            *mpi_com,
                            MPI_INFO_NULL);
        }
    else
        {
        vtkErrorMacro("Parallel access is not allowed in MED files prior to version 3");
        return -1;
        }
      }

    if (this->ParallelFileId < 0)
      {
      vtkDebugMacro("Error : unable to parallel-open file "
                    << this->MedFile->GetFileName());
      }
#endif

    } // OpenLevel
  this->OpenLevel++;
  return res;
}

void vtkMedDriver::Close()
{
  this->OpenLevel--;
  if (this->OpenLevel == 0)
    {
    if (MEDfileClose(this->FileId) < 0)
      {
      vtkErrorMacro("Error: unable to close the current file.");
      }
    this->FileId = -1;

    if (this->ParallelFileId != -1)
    {
      if (MEDfileClose(this->ParallelFileId) < 0)
      {
      vtkErrorMacro("Error: unable to parallel-close the current file.");
      }
    }
    this->ParallelFileId = -1;
    }
}

bool vtkMedDriver::CanReadFile()
{
  bool canRead = (this->RestrictedOpen() >= 0);
  this->Close();
  return canRead;
}

void vtkMedDriver::ReadFileVersion(int* major, int* minor, int* release)
{
  FileRestrictedOpen open(this);

  med_int amajor, aminor, arelease;
  if (MEDfileNumVersionRd(this->FileId, &amajor, &aminor, &arelease) < 0)
    {
    vtkErrorMacro("Impossible to read the version of this file");
    return;
    }
  *major = amajor;
  *minor = aminor;
  *release = arelease;
}

void vtkMedDriver::ReadRegularGridInformation(vtkMedRegularGrid* grid)
{
  vtkErrorMacro("vtkMedDriver::ReadInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadCurvilinearGridInformation(vtkMedCurvilinearGrid* grid)
{
  vtkErrorMacro("vtkMedDriver::ReadInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadUnstructuredGridInformation(vtkMedUnstructuredGrid* grid)
{
  vtkErrorMacro("vtkMedDriver::ReadInformation not Implemented !");
  return;
}

// Description:
// load all Information data associated with this standard grid.
void vtkMedDriver::ReadGridInformation(vtkMedGrid* grid)
{
  if(vtkMedRegularGrid::SafeDownCast(grid) != NULL)
    {
    this->ReadRegularGridInformation(vtkMedRegularGrid::SafeDownCast(grid));
    }
  if(vtkMedCurvilinearGrid::SafeDownCast(grid) != NULL)
    {
    this->ReadCurvilinearGridInformation(vtkMedCurvilinearGrid::SafeDownCast(grid));
    }
  if(vtkMedUnstructuredGrid::SafeDownCast(grid) != NULL)
    {
    this->ReadUnstructuredGridInformation(vtkMedUnstructuredGrid::SafeDownCast(grid));
    }
}

void vtkMedDriver::ReadFamilyInformation(vtkMedMesh* mesh, vtkMedFamily* family)
{
  vtkErrorMacro("vtkMedDriver::ReadFamilyInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadFileInformation(vtkMedFile* file)
{
  vtkErrorMacro("vtkMedDriver::ReadFileInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadProfileInformation(vtkMedProfile* profile)
{
  vtkErrorMacro("vtkMedDriver::ReadProfileInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadFieldInformation(vtkMedField* field)
{
  vtkErrorMacro("vtkMedDriver::ReadFieldInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadFieldOverEntityInformation(vtkMedFieldOverEntity* fieldOverEntity)
{
  vtkErrorMacro("vtkMedDriver::ReadFieldOverEntityInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadMeshInformation(vtkMedMesh* mesh)
{
  vtkErrorMacro("vtkMedDriver::ReadMeshInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadLocalizationInformation(vtkMedLocalization* loc)
{
  vtkErrorMacro("vtkMedDriver::ReadLocalizationInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadInterpolationInformation(vtkMedInterpolation* interp)
{
  vtkErrorMacro("vtkMedDriver::ReadInterpolationInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadFieldStepInformation(vtkMedFieldStep* step, bool readAllEntityInfo)
{
  vtkErrorMacro("vtkMedDriver::ReadFieldStepInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadFieldOnProfileInformation(vtkMedFieldOnProfile* fop)
{
  vtkErrorMacro("vtkMedDriver::ReadFieldOnProfileInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadStructElementInformation(
    vtkMedStructElement*)
{
  vtkErrorMacro("vtkMedDriver::ReadStructElementInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadSupportMeshInformation(
    vtkMedMesh*)
{
  vtkErrorMacro("vtkMedDriver::ReadSupportMeshInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadConstantAttributeInformation(vtkMedConstantAttribute*)
{
  vtkErrorMacro("vtkMedDriver::ReadConstantAttributeInformation not Implemented !");
  return;
}

void vtkMedDriver::ReadVariableAttributeInformation(vtkMedVariableAttribute*)
{
  vtkErrorMacro("vtkMedDriver::ReadVariableAttributeInformation not Implemented !");
  return;
}

void vtkMedDriver::LoadPointGlobalIds(vtkMedGrid* grid)
{
  vtkErrorMacro("vtkMedDriver::LoadPointGlobalIds not Implemented !");
  return;
}

void vtkMedDriver::LoadFamilyIds(vtkMedEntityArray* array)
{
  vtkErrorMacro("vtkMedDriver::LoadFamilyIds not Implemented !");
  return;
}

void vtkMedDriver::LoadCoordinates(vtkMedGrid* grid)
{
  vtkErrorMacro("vtkMedDriver::LoadCoordinates not Implemented !");
  return;
}

void vtkMedDriver::LoadProfile(vtkMedProfile* profile)
{
  vtkErrorMacro("vtkMedDriver::LoadProfile not Implemented !");
  return;
}

void vtkMedDriver::LoadConnectivity(vtkMedEntityArray* array)
{
  vtkErrorMacro("vtkMedDriver::LoadConnectivity not Implemented !");
  return;
}

void vtkMedDriver::LoadCellGlobalIds(vtkMedEntityArray* array)
{
  vtkErrorMacro("vtkMedDriver::LoadGlobalIds not Implemented !");
  return;
}

void vtkMedDriver::LoadField(vtkMedFieldOnProfile* foe, med_storage_mode mode)
{
  vtkErrorMacro("vtkMedDriver::LoadFieldOnProfile not Implemented !");
  return;
}

void vtkMedDriver::LoadVariableAttribute(vtkMedVariableAttribute*,
                                         vtkMedEntityArray*)
{
  vtkErrorMacro("vtkMedDriver::LoadVariableAttribute not Implemented !");
  return;
}

void vtkMedDriver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  PRINT_IVAR(os, indent, OpenLevel);
  PRINT_IVAR(os, indent, FileId);
}
