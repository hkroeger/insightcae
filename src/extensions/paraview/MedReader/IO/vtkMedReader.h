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

#ifndef __vtkMedReader_h_
#define __vtkMedReader_h_

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkMedSetGet.h"
#include <map>

class vtkMedFile;
class vtkMedDriver;
class vtkMedSelection;
class vtkMedFieldOverEntity;
class vtkMedFieldStep;
class vtkMedField;
class vtkMedMesh;
class vtkMedFamily;
class vtkMedLocalization;
class vtkMedEntityArray;
class vtkMedFamilyOnEntity;
class vtkMedFamilyOnEntityOnProfile;
class vtkMedProfile;
class vtkMedComputeStep;
class vtkMedGrid;
class vtkMedFieldOnProfile;
class vtkMedListOfFieldSteps;
class vtkMedEntity;

class vtkUnstructuredGrid;
class vtkUnsignedCharArray;
class vtkIdList;
class vtkDoubleArray;
class vtkFieldData;
class vtkInformationDataObjectKey;
class vtkMutableDirectedGraph;
class vtkDataSet;

class VTK_EXPORT vtkMedReader: public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkMedReader* New();
  vtkTypeMacro(vtkMedReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the file name to read from
  virtual void SetFileName(const char*);
  vtkGetStringMacro(FileName)

  // Description:
  // Is the given file a MED file?
  virtual int CanReadFile(const char* fname);

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in
  // the format .extension
  virtual const char*
  GetFileExtensions()
  {
    return ".med .rmed";
  }

  // Description:
  // Return a descriptive name for the file format that might be
  // useful in a GUI.
  virtual const char*
  GetDescriptiveName()
  {
    return "MED file (Modele d'Echange de Donnees)";
  }

  // Description:
  // use these methods to enable/disable point arrays
  virtual int GetPointFieldArrayStatus(const char* name);
  virtual void SetPointFieldArrayStatus(const char* name, int status);
  virtual int GetNumberOfPointFieldArrays();
  virtual const char* GetPointFieldArrayName(int);

  // Description:
  // use these methods to enable/disable cell arrays
  virtual int GetCellFieldArrayStatus(const char* name);
  virtual void SetCellFieldArrayStatus(const char* name, int status);
  virtual int GetNumberOfCellFieldArrays();
  virtual const char* GetCellFieldArrayName(int);

  // Description:
  // use these methods to enable/disable quadrature arrays
  virtual int GetQuadratureFieldArrayStatus(const char* name);
  virtual void SetQuadratureFieldArrayStatus(const char* name, int status);
  virtual int GetNumberOfQuadratureFieldArrays();
  virtual const char* GetQuadratureFieldArrayName(int);

  // Description:
  // use these methods to enable/disable quadrature arrays
  virtual int GetElnoFieldArrayStatus(const char* name);
  virtual void SetElnoFieldArrayStatus(const char* name, int status);
  virtual int GetNumberOfElnoFieldArrays();
  virtual const char* GetElnoFieldArrayName(int);

  // Description:
  // use this method to enable/disable cell types
  // the key is encoded with the vtkMedUtilities::EntityKey method
  // which returns a string
  // CELL_TYPE/MED_ENTITE_MAILLAGE/MED_GEOMETRIE_ELEMENT
  virtual void SetEntityStatus(const char* key, int flag);
  virtual int GetEntityStatus(const vtkMedEntity&);

  // Description:
  // use this method to enable/disable a family support
  // the key is formatted by the vtkMedUtilities::FamilyKey method which
  // returns a string
  // GROUP/MESH_NAME/OnPoint/FAMILY_NAME or MESH_NAME/OnCell/FAMILY_NAME
  virtual void SetGroupStatus(const char* key, int flag);
  virtual int GetGroupStatus(const char* key);

  // Description:
  // the animation mode modify the way the reader answers time requests from
  // the pipeline.
  // Default (0) : this is PhysicalTime if there are times, or Iterations
  // if there are only iterations
  // PhysicalTime (1) : the reader aggregates all physical times available
  // in the file.
  // for a given time t, the reader will return for each field the
  // ComputeStep with the highest time inferior
  // to t, and with the highest iteration.
  // Iteration (2) : you need to also set the TimeIndexForIterations field.
  // In this case, the reader will understand
  // time requests has a request to iterate over iterations for the given time.
  // Modes (3) : the reader will output the selected fields at all selected
  // frequencies, and will output a fake TimeRange of -PI/+PI.
  // Use the *FrequencyArray* interface to select them.
  //BTX
  enum eAnimationMode
  {
    Default=0, PhysicalTime=1, Iteration=2, Modes=3
  };
  //ETX
  vtkSetMacro(AnimationMode, int);
  vtkGetMacro(AnimationMode, int);

  // Description:
  // in med files, the ComputeSteps are defined by a
  // pair <iteration, physicalTime>.
  // the TimeIndexForIterations ivar is used only if the TimeMode is
  // Iteration.
  // it fixes the PhysicalTime index when iterating over iterations : only
  // iterations with the corresponding physical time will be iterated over.
  // The index given here is an index into the AvailableTimes array
  // that is returned be the GetAvailableTimes Method.
  vtkSetMacro(TimeIndexForIterations, double);
  vtkGetMacro(TimeIndexForIterations, double);

  // Description:
  // use these methods to enable/disable a given frequency for modal analysis
  virtual int GetFrequencyArrayStatus(const char* name);
  virtual void SetFrequencyArrayStatus(const char* name, int status);
  virtual int GetNumberOfFrequencyArrays();
  virtual const char* GetFrequencyArrayName(int);

  // Description:
  // returns the available physical times. Use this to get the times when in
  // iteration mode.
  virtual vtkDoubleArray* GetAvailableTimes();

  // Description:
  // Build the graph used to pass information to the client on the supports
  virtual void BuildSIL(vtkMutableDirectedGraph*);

  // Description:
  // Every time the SIL is updated a this will return a different value.
  virtual int GetSILUpdateStamp();

  // Description:
  // reset the selection arrays without having to rebuild the SIL.
  virtual void ClearSelections();

  // Description:
  // The CacheStrategy indicates to the reader if it
  // should cache some of the arrays.
  //BTX
  enum eCacheStrategy
  {
    CacheNothing, CacheGeometry, CacheGeometryAndFields
  };
  //ETX
  vtkSetMacro(CacheStrategy, int)
  vtkGetMacro(CacheStrategy, int)

  // Description :
  // release arrays read from MED (point coordinates, profile ids, family ids)
  void ClearMedSupports();

  // Description :
  // release arrays read from MED (fields)
  void ClearMedFields();

  // Description:
  // If this flag is set, the reader will output a vector field for each
  // field in the file that has 2 or more components by extracting the
  // first 3 compoenents
  vtkSetMacro(GenerateVectors, int);
  vtkGetMacro(GenerateVectors, int);

protected:
  vtkMedReader();
  virtual ~vtkMedReader();

  // Description:
  // This is called by the superclass.
  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**,
      vtkInformationVector*);

  // Description:
  // This is called by the superclass.
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**,
      vtkInformationVector*);

  // Description:
  // This is called by the superclass.
  virtual int RequestData(vtkInformation*, vtkInformationVector**,
      vtkInformationVector*);

  // Description:
  // Gather all compute steps in the fields
  virtual void GatherComputeSteps();

  // Description:
  // Give the animation steps to the pipeline
  virtual void AdvertiseTime(vtkInformation*);

  // Description:
  // returns 1 if at least one the families of this mesh is selected,
  // 0 otherwise.
  virtual int IsMeshSelected(vtkMedMesh*);

  // Description:
  // returns if the field is selected.
  virtual int IsFieldSelected(vtkMedField*);
  virtual int IsPointFieldSelected(vtkMedField*);
  virtual int IsCellFieldSelected(vtkMedField*);
  virtual int IsQuadratureFieldSelected(vtkMedField*);
  virtual int IsElnoFieldSelected(vtkMedField*);

  virtual void AddQuadratureSchemeDefinition(vtkInformation*,
      vtkMedLocalization*);

  //BTX
  enum
  {
    Initialize,
    StartRequest,
    AfterCreateMedSupports,
    EndBuildVTKSupports,
    EndRequest
  };
  //ETX
  virtual void ClearCaches(int when);

  virtual void CreateMedSupports();

  virtual bool BuildVTKSupport(vtkMedFamilyOnEntityOnProfile*,
                               int doBuildSupport);

  virtual void MapFieldsOnSupport(vtkMedFamilyOnEntityOnProfile* foe,
                                  int doMapField);

  virtual void CreateVTKFieldOnSupport(vtkMedFieldOnProfile*,
                                       vtkMedFamilyOnEntityOnProfile*,
                                       int doCreateField);

  virtual void MapFieldOnSupport(vtkMedFieldOnProfile*,
                                 vtkMedFamilyOnEntityOnProfile*,
                                 int doCreateField);

  // Description:
  // Necessary call for the initialization of the filters used for the MED
  // library in their parallel reads
  virtual void InitializeParallelRead();

  // Description:
  // This method is called after all info from all med files are read.
  // it links information coming from different files :
  // for instance, it creates the vtkMedFamilyOnEntityOnProfile instances
  // to link mesh and field.
  virtual void  LinkMedInfo();

  virtual vtkMedProfile* GetProfile(const char*);

  virtual vtkMedLocalization* GetLocalization(const char*);

  virtual int GetLocalizationKey(vtkMedFieldOnProfile*);

  virtual void InitializeQuadratureOffsets(vtkMedFieldOnProfile*,
                                           vtkMedFamilyOnEntityOnProfile*);

  virtual void SetVTKFieldOnSupport(vtkMedFieldOnProfile*,
                                    vtkMedFamilyOnEntityOnProfile*);

  // Description:
  // returns the current grid to use for this mesh.
  vtkMedGrid* FindGridStep(vtkMedMesh* mesh);

  virtual void  GatherFieldSteps(vtkMedField*, vtkMedListOfFieldSteps&);

  // Description:
  // returns the index of the given frequency in the AvailableTimes array.
  vtkIdType GetFrequencyIndex(double);

  virtual void ChooseRealAnimationMode();

  // Description :
  // Load the connectivity of this entities, and also those of the
  // sub-entities in the case of non nodal connectivity.
  virtual void LoadConnectivity(vtkMedEntityArray*);

  virtual void InitializeCellGlobalIds();

  // Description :
  // returns 1 if any point/cell family of this mesh is selected.
  int HasMeshAnyCellSelectedFamily(vtkMedMesh*);
  int HasMeshAnyPointSelectedFamily(vtkMedMesh*);

  // Description:
  // use this method to enable/disable a family support
  // the key is formatted by the vtkMedUtilities::FamilyKey method which
  // returns a string
  // FAMILY/MESH_NAME/OnPoint/FAMILY_NAME or MESH_NAME/OnCell/FAMILY_NAME
  virtual void SetFamilyStatus(const char* key, int flag);
  virtual int GetFamilyStatus(vtkMedMesh*, vtkMedFamily*);

  // Description:
  // This is a helper function that is called when requesting a family status
  // if a group status has been set after the last family status update.
  virtual void SelectFamiliesFromGroups();

  // Description:
  // Instanciate a new vtkDataSet and initialize it to the points
  // of this support.
  vtkDataSet* CreateUnstructuredGridForPointSupport(
      vtkMedFamilyOnEntityOnProfile* foep);

  // Field selections
  vtkMedSelection* PointFields;
  vtkMedSelection* CellFields;
  vtkMedSelection* QuadratureFields;
  vtkMedSelection* ElnoFields;

  // Support selection
  vtkMedSelection* Entities;
  vtkMedSelection* Groups;

  // name of the file to read from
  char* FileName;

  // time management
  int AnimationMode;
  double TimeIndexForIterations;
  vtkDoubleArray* AvailableTimes;
  double TimePrecision;
  vtkMedSelection* Frequencies;

  int CacheStrategy;
  int GenerateVectors;

  //BTX
  class vtkMedReaderInternal;
  vtkMedReaderInternal* Internal;
  //ETX

private:
  vtkMedReader(const vtkMedReader&); // Not implemented.
  void operator=(const vtkMedReader&); // Not implemented.
};

#endif //__vtkMedReader_h_
