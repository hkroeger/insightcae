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

#include "vtkExtractGroup.h"

#include "vtkObjectFactory.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkDataArraySelection.h"
#include "vtkMedUtilities.h"
#include "vtkTimeStamp.h"
#include "vtkInEdgeIterator.h"
#include "vtkMedReader.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkExecutive.h"
#include "vtkVariantArray.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkCompositeDataIterator.h"

#include <map>
#include <deque>

vtkStandardNewMacro(vtkExtractGroup);

vtkCxxSetObjectMacro(vtkExtractGroup, SIL, vtkMutableDirectedGraph);

vtkExtractGroup::vtkExtractGroup()
{
  this->SIL=NULL;
  this->Entities=vtkDataArraySelection::New();
  this->Families=vtkDataArraySelection::New();
  this->Groups=vtkDataArraySelection::New();
  this->PruneOutput=0;
}

vtkExtractGroup::~vtkExtractGroup()
{
  this->Entities->Delete();
  this->Families->Delete();
  this->Groups->Delete();
}

int vtkExtractGroup::ModifyRequest(vtkInformation* request, int when)
{
#warning ok to remove?
  //request->Set(vtkDemandDrivenPipeline::REQUEST_REGENERATE_INFORMATION(), 1);
  return this->Superclass::ModifyRequest(request, when);
}

int vtkExtractGroup::RequestInformation(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  vtkInformation* outInfo=outputVector->GetInformationObject(0);

  vtkInformation* inputInfo=inputVector[0]->GetInformationObject(0);

  vtkMutableDirectedGraph* old_SIL=this->GetSIL();

  if(inputInfo->Has(vtkDataObject::SIL()))
    {
    this->SetSIL(vtkMutableDirectedGraph::SafeDownCast(inputInfo->Get(
        vtkDataObject::SIL())));
    }
  else
    {
    vtkMutableDirectedGraph* sil=vtkMutableDirectedGraph::New();
    this->BuildDefaultSIL(sil);
    this->SetSIL(sil);
    sil->Delete();
    }

  if(this->GetSIL()!=old_SIL||this->GetSIL()->GetMTime()>this->SILTime)
    {
    this->ClearSelections();
    this->SILTime.Modified();
    outInfo->Set(vtkDataObject::SIL(), this->GetSIL());
    }

  return 1;
}

vtkIdType vtkExtractGroup::FindVertex(const char* name)
{
  vtkStringArray* names=vtkStringArray::SafeDownCast(
      this->GetSIL()->GetVertexData()->GetAbstractArray("Names"));

  return names->LookupValue(name);
}

void vtkExtractGroup::ClearSelections()
{
  this->Families->RemoveAllArrays();
  this->Entities->RemoveAllArrays();
  this->Groups->RemoveAllArrays();
}

void vtkExtractGroup::BuildDefaultSIL(vtkMutableDirectedGraph* sil)
{
  sil->Initialize();

  vtkSmartPointer<vtkVariantArray> childEdge=
      vtkSmartPointer<vtkVariantArray>::New();
  childEdge->InsertNextValue(0);

  vtkSmartPointer<vtkVariantArray> crossEdge=
      vtkSmartPointer<vtkVariantArray>::New();
  crossEdge->InsertNextValue(1);

  // CrossEdge is an edge linking hierarchies.
  vtkUnsignedCharArray* crossEdgesArray=vtkUnsignedCharArray::New();
  crossEdgesArray->SetName("CrossEdges");
  sil->GetEdgeData()->AddArray(crossEdgesArray);
  crossEdgesArray->Delete();
  std::deque<vtkStdString> names;

  // Now build the hierarchy.
  vtkIdType rootId=sil->AddVertex();
  names.push_back("SIL");

  // Add a global entry to encode global names for the families
  vtkIdType globalFamilyRoot=sil->AddChild(rootId, childEdge);
  names.push_back("Families");

  // Add a global entry to encode global names for the families
  vtkIdType globalGroupRoot=sil->AddChild(rootId, childEdge);
  names.push_back("Groups");

  // Add the groups subtree
  vtkIdType groupsRoot=sil->AddChild(rootId, childEdge);
  names.push_back("GroupTree");

  // Add the attributes subtree
  vtkIdType attributesRoot=sil->AddChild(rootId, childEdge);
  names.push_back("Attributes");

  // Add a global entry to encode names for the cell types
  vtkIdType globalEntityRoot=sil->AddChild(rootId, childEdge);
  names.push_back("Entity");

  // Add the cell types subtree
  vtkIdType entityTypesRoot=sil->AddChild(rootId, childEdge);
  names.push_back("EntityTree");

  // This array is used to assign names to nodes.
  vtkStringArray* namesArray=vtkStringArray::New();
  namesArray->SetName("Names");
  namesArray->SetNumberOfTuples(sil->GetNumberOfVertices());
  sil->GetVertexData()->AddArray(namesArray);
  namesArray->Delete();
  std::deque<vtkStdString>::iterator iter;
  vtkIdType cc;
  for(cc=0, iter=names.begin(); iter!=names.end(); ++iter, ++cc)
    {
    namesArray->SetValue(cc, (*iter).c_str());
    }

}

int vtkExtractGroup::RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector)

{
  vtkInformation* inputInfo=inputVector[0]->GetInformationObject(0);
  vtkMultiBlockDataSet* inmb=vtkMultiBlockDataSet::SafeDownCast(inputInfo->Get(
      vtkDataObject::DATA_OBJECT()));

  if(inmb==NULL)
    return 0;

  vtkMultiBlockDataSet* outmb=this->GetOutput();

  outmb->CopyStructure(inmb);

  vtkCompositeDataIterator* iterator = inmb->NewIterator();
#warning ok to remove?
  //iterator->SetVisitOnlyLeaves(true);
  iterator->InitTraversal();
  while(!iterator->IsDoneWithTraversal())
    {
    vtkDataObject* indo = iterator->GetCurrentDataObject();
    if(indo == NULL)
      continue;

    if(indo->GetFieldData()->HasArray("BLOCK_NAME"))
      {

      vtkStringArray* path = vtkStringArray::SafeDownCast(
          indo->GetFieldData()->GetAbstractArray("BLOCK_NAME"));

      if(this->IsBlockSelected(path))
        {
        vtkMultiBlockDataSet* parent = vtkMedUtilities::GetParent(outmb, path);
        int nb = parent->GetNumberOfBlocks();
        parent->SetNumberOfBlocks(nb+1);
        vtkDataObject *outdo = indo->NewInstance();
        outdo->ShallowCopy(indo);
        parent->SetBlock(nb, outdo);
        outdo->Delete();
        }
      }
    iterator->GoToNextItem();
    }

  if(PruneOutput)
    {
    this->PruneEmptyBlocks(outmb);
    }
  return 1;
}

void vtkExtractGroup::SetGroupStatus(const char* key, int flag)
{
  vtkIdType index=this->Groups->GetArrayIndex(key);
  if(index==-1)
    {
    index = this->Groups->AddArray(key);
    this->Modified();
    }
  int status=this->Groups->GetArraySetting(index);
  if(status!=flag)
    {
    if(flag)
      {
      this->Groups->EnableArray(key);
      }
    else
      {
      this->Groups->DisableArray(key);
      }
    this->Modified();
    }
  this->GroupSelectionTime.Modified();
}

void vtkExtractGroup::PruneEmptyBlocks(vtkMultiBlockDataSet* mb)
{
  if(mb==NULL)
    return;
  vtkIdType nn=0;
  while(nn<mb->GetNumberOfBlocks())
    {
    bool remove=false;
    vtkDataObject* dataObj=mb->GetBlock(nn);
    if(dataObj==NULL)
      {
      remove=true;
      }
    else
      {
      vtkMultiBlockDataSet* child=vtkMultiBlockDataSet::SafeDownCast(dataObj);
      if(child!=NULL)
        {
        this->PruneEmptyBlocks(child);
        if(child->GetNumberOfBlocks()==0)
          {
          remove=true;
          }
        }
      }
    if(remove)
      {
      mb->RemoveBlock(nn);
      }
    else
      {
      nn++;
      }
    }
}

int vtkExtractGroup::IsBlockSelected(vtkStringArray* path)
{
  const char* meshName = (path->GetNumberOfValues()>0?
                          path->GetValue(0) : NULL);
  const char* cellOrPoint = (path->GetNumberOfValues()>1?
                             path->GetValue(1) : NULL);
  const char* familyName = (path->GetNumberOfValues()>2?
                            path->GetValue(2) : NULL);

  if(!this->IsFamilySelected(meshName, cellOrPoint, familyName))
    {
    return 0;
    }

  bool isOnPoint = (strcmp(cellOrPoint, vtkMedUtilities::OnPointName)==0);

  const char* entityName = (isOnPoint || path->GetNumberOfValues()<=3 ? NULL :
                            path->GetValue(3));

  if(isOnPoint)
    return true;

  return IsEntitySelected(entityName);

}

int vtkExtractGroup::IsEntitySelected(const char* entityKey)
{
  return this->Entities->GetArraySetting(entityKey);
}

int vtkExtractGroup::IsFamilySelected(const char* meshName,
    const char* pointOrCellKey, const char* familyName)
{
  if(this->FamilySelectionTime <= this->GroupSelectionTime)
    {
    this->SelectFamiliesFromGroups();
    }

  int
      pointOrCell= (strcmp(vtkMedUtilities::OnPointName, pointOrCellKey)==0?
                    vtkMedUtilities::OnPoint
                    : vtkMedUtilities::OnCell);

  std::string name=
      vtkMedUtilities::FamilyKey(meshName, pointOrCell, familyName);

  return this->Families->GetArraySetting(name.c_str());
}

void vtkExtractGroup::SelectFamiliesFromGroups()
{
  this->Families->DisableAllArrays();
  vtkStringArray* names=vtkStringArray::SafeDownCast(
      this->GetSIL()->GetVertexData()->GetAbstractArray("Names"));

  for(int index = 0; index < this->Groups->GetNumberOfArrays(); index++)
    {
    if(this->Groups->GetArraySetting(index) == 0)
      continue;

    const char* name = this->Groups->GetArrayName(index);
    vtkIdType silindex = this->FindVertex(name);

    vtkInEdgeIterator* it = vtkInEdgeIterator::New();

    this->GetSIL()->GetInEdges(silindex, it);
    while(it->HasNext())
      {
      vtkIdType famId = it->Next().Source;
      vtkStdString famName = names->GetValue(famId);
      if(strncmp(famName, "FAMILY", 6)==0)
        {
        this->Families->EnableArray(famName.c_str());
        }
      }
    it->Delete();
    }

  this->FamilySelectionTime.Modified();
}

void vtkExtractGroup::SetEntityStatus(const char* key, int flag)
{
  vtkIdType index=this->Entities->GetArrayIndex(key);
  if(index==-1)
    {
    index = this->Entities->AddArray(key);
    this->Modified();
    }
  int status=this->Entities->GetArraySetting(index);
  if(status!=flag)
    {
    if(flag)
      {
      this->Entities->EnableArray(key);
      }
    else
      {
      this->Entities->DisableArray(key);
      }
    this->Modified();
    }
}

void vtkExtractGroup::SetFamilyStatus(const char* key, int flag)
{
  vtkIdType index=this->Families->GetArrayIndex(key);
  if(index==-1)
    {
    return;
    }
  int status=this->Families->GetArraySetting(index);
  if(status!=flag)
    {
    if(flag)
      {
      this->Families->EnableArray(key);
      }
    else
      {
      this->Families->DisableArray(key);
      }
    }
}

int vtkExtractGroup::GetSILUpdateStamp()
{
  return this->SILTime;
}

void vtkExtractGroup::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
