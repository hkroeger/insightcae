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

#ifndef vtkExtractGroup_h__
#define vtkExtractGroup_h__

#include "vtkMultiBlockDataSetAlgorithm.h"
class vtkMutableDirectedGraph;
class vtkInformation;
class vtkInformationVector;
class vtkDataArraySelection;
class vtkTimeStamp;
class vtkStringArray;

class VTK_EXPORT vtkExtractGroup: public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkExtractGroup* New();
vtkTypeMacro(vtkExtractGroup, vtkMultiBlockDataSetAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Every time the SIL is updated a this will return a different value.
  virtual int GetSILUpdateStamp();

  // Description:
  // use this method to enable/disable cell types
  // the key is encoded with the vtkMedUtilities::CellTypeKey method
  // which returns a string
  // CELL_TYPE/MED_ENTITE_MAILLAGE/MED_GEOMETRIE_ELEMENT
  virtual void SetEntityStatus(const char* key, int flag);

  // Description:
  // use this method to enable/disable a family support
  // the key is formatted by the vtkMedUtilities::FamilyKey method which
  // returns a string
  // GROUP/MESH_NAME/OnPoint/FAMILY_NAME or GROUP/MESH_NAME/OnCell/FAMILY_NAME
  virtual void SetGroupStatus(const char* key, int flag);

  // Description :
  // If set to 1, this filter will prune the empty parts in the output.
  // If not, then empty datasets will be kept
  vtkSetMacro(PruneOutput, int);
  vtkGetMacro(PruneOutput, int);

  int ModifyRequest(vtkInformation* request, int when);

protected:
  vtkExtractGroup();
  ~vtkExtractGroup();

  int RequestInformation(vtkInformation *request,
      vtkInformationVector **inputVector, vtkInformationVector *outputVector);

  int RequestData(vtkInformation *request, vtkInformationVector **inputVector,
      vtkInformationVector *outputVector);

  // Description :
  // returns 1 if this cell type is to be passed through
  int IsEntitySelected(const char*);

  // Description :
  // returns 1 if this family is to be passed through
  int IsFamilySelected(const char* meshName, const char* cellOrPoint,
      const char* familyName);

  // Description :
  // removes empty blocks from the vtkMultiBlockDataSet.
  void PruneEmptyBlocks(vtkMultiBlockDataSet* mb);

  // Description:
  // This SIL stores the structure of the mesh/groups/cell types
  // that can be selected.
  virtual void SetSIL(vtkMutableDirectedGraph*);
  vtkGetObjectMacro(SIL, vtkMutableDirectedGraph);

  virtual void BuildDefaultSIL(vtkMutableDirectedGraph*);

  // Description:
  // use this method to enable/disable a family support
  // the key is formatted by the vtkMedUtilities::FamilyKey method which
  // returns a string
  // FAMILY/MESH_NAME/OnPoint/FAMILY_NAME or MESH_NAME/OnCell/FAMILY_NAME
  virtual void SetFamilyStatus(const char* key, int flag);

  // Description:
  // Update the Family status from te group status.
  // The family status is lazily updated when GetFamilyStatus is called.
  virtual void SelectFamiliesFromGroups();

  // Description:
  // returns true if the block described by the given information
  // is selected.
  virtual int IsBlockSelected(vtkStringArray* path);

  virtual void ClearSelections();

  virtual vtkIdType FindVertex(const char* name);

  vtkMutableDirectedGraph* SIL;

  // Support selection
  vtkDataArraySelection* Entities;
  vtkDataArraySelection* Families;
  vtkDataArraySelection* Groups;

  vtkTimeStamp SILTime;
  vtkTimeStamp FamilySelectionTime;
  vtkTimeStamp GroupSelectionTime;

  int PruneOutput;

private:
  vtkExtractGroup(const vtkExtractGroup&);
  void operator=(const vtkExtractGroup&); // Not implemented.
};

#endif
