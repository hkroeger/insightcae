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

#include "vtkMedReader.h"

#include "vtkMedFile.h"
#include "vtkMedDriver.h"
#include "vtkMedUtilities.h"
#include "vtkMedFamily.h"
#include "vtkMedGroup.h"
#include "vtkMedMesh.h"
#include "vtkMedUnstructuredGrid.h"
#include "vtkMedCurvilinearGrid.h"
#include "vtkMedRegularGrid.h"
#include "vtkMedEntityArray.h"
#include "vtkMedField.h"
#include "vtkMedFieldStep.h"
#include "vtkMedFieldOverEntity.h"
#include "vtkMedProfile.h"
#include "vtkMedIntArray.h"
#include "vtkMedLocalization.h"
#include "vtkMedFamilyOnEntity.h"
#include "vtkMedFieldOnProfile.h"
#include "vtkMedSelection.h"
#include "vtkMedFamilyOnEntityOnProfile.h"
#include "vtkMedLink.h"
#include "vtkMedInterpolation.h"
#include "vtkMedStructElement.h"
#include "vtkMedConstantAttribute.h"

#include "vtkObjectFactory.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkStringArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkUnsignedCharArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkSmartPointer.h"
#include "vtkVariantArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkExecutive.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "vtkQuadratureSchemeDefinition.h"
#include "vtkCellType.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkConfigure.h"
#include "vtkMultiProcessController.h"
#include "vtkCommunicator.h"

// #include "vtkSMDoubleVectorProperty.h"
#include "vtkInformationDataObjectKey.h"

#include <deque>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <set>
#include <algorithm>

#undef DBGOUT

using namespace std;

struct VTKField
{
  vtkSmartPointer<vtkDataArray> DataArray;
  vtkSmartPointer<vtkDataArray> Vectors;
  vtkSmartPointer<vtkIdTypeArray> QuadratureIndexArray;
};

class vtkMedListOfFieldSteps : public std::list<vtkMedFieldStep*>
{};

typedef int LocalizationKey;

class vtkMedReader::vtkMedReaderInternal
{
public:
  int NumberOfPieces;
  int CurrentPieceNumber;
  int GhostLevel;
  double UpdateTimeStep;
  vtkTimeStamp FileNameMTime;
  vtkTimeStamp MetaDataMTime;
  vtkTimeStamp GroupSelectionMTime;
  vtkTimeStamp FamilySelectionMTime;
  int SILUpdateStamp;
  int RealAnimationMode;
  vtkMedSelection* Families;

  // this stores the aggregation of all compute steps from
  // both meshes and fields.
  std::map<med_float, std::set<med_int> > GlobalComputeStep;

  // Store the vtkMutableDirectedGraph that represents links between family, groups and cell types
  vtkMutableDirectedGraph* SIL;

  // this map is used to keep clean data sets in the cache, without any field.
  // for each support, store the vtkDataSet
  map<vtkMedFamilyOnEntityOnProfile*, vtkSmartPointer<vtkDataSet> > DataSetCache;

  // this is the current dataset for the given support.
  map<vtkMedFamilyOnEntityOnProfile*, vtkDataSet*> CurrentDataSet;

  // for each support, cache the VTK arrays that correspond to a given field at the given step.
  map<vtkMedFamilyOnEntityOnProfile*, map<vtkMedFieldOnProfile*, VTKField> > FieldCache;
  //map<vtkMedFamilyOnEntity*, map<vtkMedFieldOnProfile*, bool> > FieldMatchCache;

  // This list keep tracks of all the currently selected supports
  set<vtkMedFamilyOnEntityOnProfile*> UsedSupports;

  // this map keeps for each support, the quadrature offset array so that
  // different fields on the same support can use
  // the same offset array, provided they use the same gauss points definitions
  map<vtkMedFamilyOnEntityOnProfile*,
      map<LocalizationKey, vtkSmartPointer<vtkIdTypeArray> > >
      QuadratureOffsetCache;

  map<vtkMedFamilyOnEntityOnProfile*,
      map<vtkMedFieldOnProfile*, LocalizationKey> > QuadOffsetKey;

  std::map<std::string, vtkSmartPointer<vtkMedFile> > MedFiles;

  vtkMedReaderInternal()
  {
    this->SIL=vtkMutableDirectedGraph::New();
    this->SILUpdateStamp=-1;
    this->RealAnimationMode=vtkMedReader::PhysicalTime;
    this->Families=vtkMedSelection::New();
    this->FamilySelectionMTime.Modified();
    this->GroupSelectionMTime.Modified();
  }
  ~vtkMedReaderInternal()
  {
    this->SIL->Delete();
    this->Families->Delete();
  }

  void ClearSupportCache(vtkMedFamilyOnEntityOnProfile* foep)
  {
    //this->Med2VTKPointIndex.erase(foep);
    this->QuadratureOffsetCache.erase(foep);
    //this->FieldMatchCache.erase(foe);
    this->FieldCache.erase(foep);
    this->CurrentDataSet.erase(foep);
    this->DataSetCache.erase(foep);
  }

  vtkIdType GetChild(vtkIdType parent, const vtkStdString childName)
  {
    vtkStringArray* names=vtkStringArray::SafeDownCast(
        this->SIL->GetVertexData()->GetArray("Names"));
    if(names==NULL)
      return -1;
    vtkIdType nedges=this->SIL->GetOutDegree(parent);
    for(vtkIdType id=0; id<nedges; id++)
      {
      vtkOutEdgeType edge=this->SIL->GetOutEdge(parent, id);
      if(names->GetValue(edge.Target)==childName)
        return edge.Target;
      }
    return -1;
  }

  vtkIdType GetGroupId(const char* key)
  {
    vtkStdString meshname, celltypename, groupname;
    vtkMedUtilities::SplitGroupKey(key, meshname, celltypename, groupname);
    vtkIdType root=GetChild(0, "SIL");
    if(root==-1)
      return -1;
    vtkIdType mesh=GetChild(root, meshname);
    if(mesh==-1)
      return -1;
    vtkIdType type=GetChild(mesh, celltypename);
    if(type==-1)
      return -1;
    return GetChild(type, groupname);

  }

};

vtkStandardNewMacro(vtkMedReader);

vtkMedReader::vtkMedReader()
{
  this->FileName=NULL;
  this->SetNumberOfInputPorts(0);
  this->PointFields=vtkMedSelection::New();
  this->CellFields=vtkMedSelection::New();
  this->QuadratureFields=vtkMedSelection::New();
  this->ElnoFields=vtkMedSelection::New();
  this->Entities=vtkMedSelection::New();
  this->Groups=vtkMedSelection::New();
  this->Frequencies=vtkMedSelection::New();
  this->AnimationMode=Default;
  this->TimeIndexForIterations=0;
  this->CacheStrategy=CacheGeometry;
  this->Internal=new vtkMedReaderInternal;
  this->TimePrecision=0.00001;
  this->AvailableTimes=vtkDoubleArray::New();
  this->GenerateVectors = 0;
}

vtkMedReader::~vtkMedReader()
{
  this->SetFileName(NULL);
  this->PointFields->Delete();
  this->CellFields->Delete();
  this->QuadratureFields->Delete();
  this->ElnoFields->Delete();
  this->Entities->Delete();
  this->Groups->Delete();
  this->Frequencies->Delete();
  delete this->Internal;
  this->AvailableTimes->Delete();
}

int vtkMedReader::GetSILUpdateStamp()
{
  return this->Internal->SILUpdateStamp;
}

void vtkMedReader::SetFileName(const char* fname)
{
  int modified=0;
  if(fname==this->FileName)
    return;
  if(fname&&this->FileName&&!strcmp(fname, this->FileName))
    return;
  modified=1;
  if(this->FileName)
    delete[] this->FileName;
  if (fname)
    {
    size_t fnl=strlen(fname)+1;
    char* dst=new char[fnl];
    const char* src=fname;
    this->FileName=dst;
    do
      {
      *dst++=*src++;
      }
    while (--fnl);
    }
  else
    {
    this->FileName=0;
    }
  if (modified)
    {
    this->Modified();
    this->Internal->MedFiles.clear();
    this->Internal->FileNameMTime.Modified();
    }
}

int vtkMedReader::CanReadFile(const char* fname)
{
  // the factory give a driver only when it can read the file version,
  // or it returns a NULL pointer.
  vtkSmartPointer<vtkMedFile> file=vtkSmartPointer<vtkMedFile>::New();
  file->SetFileName(fname);

  if(!file->CreateDriver())
    return 0;

  return 1;
}

int vtkMedReader::RequestInformation(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

#warning correct?
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(),-1);

  if(this->Internal->MetaDataMTime <= this->Internal->FileNameMTime)
    {
    this->ClearCaches(Initialize);

    vtkMedFile* file=vtkMedFile::New();
    file->SetFileName(this->FileName);
    this->Internal->MedFiles[this->FileName] = file;
    file->Delete();

    std::list<vtkMedFile*> file_stack;
    file_stack.push_back(file);

    // if the file cannot create a driver, this means that the filename is not
    // valid, or that I do not know how to read this file.
    while (file_stack.size() > 0)
      {
      vtkMedFile* file = file_stack.front();
      file_stack.pop_front();
      // This reads information from disk
      file->ReadInformation();

      // add all files linked to in the current file to the files to read.
      for (int linkid=0; linkid<file->GetNumberOfLink(); linkid++)
        {
        vtkMedLink* link = file->GetLink(linkid);
        const char* filename = link->GetFullLink(file->GetFileName());
        if(this->Internal->MedFiles.find(filename) == this->Internal->MedFiles.end())
          {
          vtkMedFile* newfile = vtkMedFile::New();
          newfile->SetFileName(filename);
          this->Internal->MedFiles[filename] = newfile;
          file_stack.push_back(newfile);
          newfile->Delete();
          }
        }
      }

    // This computes some meta information, like which field use which
    // support, but do not read large data from disk.
    this->LinkMedInfo();

    // This computes the initial global id of each cell type.
    this->InitializeCellGlobalIds();

    this->ClearSelections();

    this->BuildSIL(this->Internal->SIL);
    this->Internal->SILUpdateStamp++;

    this->GatherComputeSteps();

    this->Internal->MetaDataMTime.Modified();
    }

  outInfo->Set(vtkDataObject::SIL(), this->Internal->SIL);
  request->AppendUnique(vtkExecutive::KEYS_TO_COPY(),
                        vtkDataObject::SIL());
  request->AppendUnique(vtkExecutive::KEYS_TO_COPY(),
                        vtkMedUtilities::BLOCK_NAME());
  this->AdvertiseTime(outInfo);
  return 1;
}




int vtkMedReader::RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  if(this->FileName==NULL)
    {
    vtkWarningMacro( << "FileName must be set and meta data loaded");
    return 0;
    }

  vtkInformation *info=outputVector->GetInformationObject(0);

  vtkMultiBlockDataSet *output=vtkMultiBlockDataSet::SafeDownCast(info->Get(
      vtkDataObject::DATA_OBJECT()));

  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
    this->Internal->NumberOfPieces=info->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }
  else
    {
    vtkMultiProcessController* controller =
    		vtkMultiProcessController::GetGlobalController();
    if(controller)
      {
      this->Internal->NumberOfPieces=controller->GetNumberOfProcesses();
      }
    else
      {
      this->Internal->NumberOfPieces = 1;
      }
    }

  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    this->Internal->CurrentPieceNumber=info->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }
  else
    {
    this->Internal->CurrentPieceNumber=0;
    vtkMultiProcessController* controller =
    		vtkMultiProcessController::GetGlobalController();
    if(controller)
      {
      this->Internal->CurrentPieceNumber= controller->GetLocalProcessId();
      }
    else
      {
      this->Internal->CurrentPieceNumber=0;
      }
    }

  if (info->Has(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
    this->Internal->GhostLevel=info->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    }
  else
    {
    this->Internal->GhostLevel=0;
    }

#warning correct?
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
    this->Internal->UpdateTimeStep=info->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }
  else
    {
    this->Internal->UpdateTimeStep=0;
    }

  this->InitializeParallelRead();
  output->Initialize();

  this->ChooseRealAnimationMode();

  std::list<vtkMedDriver::FileOpen> openlist;
  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fileit = this->Internal->MedFiles.begin();
  while(fileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = fileit->second;
    openlist.push_back(vtkMedDriver::FileOpen(file->GetMedDriver()));
    fileit++;
    }

  // clear the dataset cache of unneeded geometry
  this->ClearCaches(StartRequest);

  // This call create the vtkMedSupports, but do not create the corresponding vtkDataSet;
  this->CreateMedSupports();
  this->ClearCaches(AfterCreateMedSupports);
  // This call creates the actual vtkDataSet that corresponds to each support
  int supportId = 0;
  int progress=0;

  // sorted copy
  std::vector<vtkMedFamilyOnEntityOnProfile*> usedSupports(
    this->Internal->UsedSupports.begin(),
    this->Internal->UsedSupports.end() );
  std::sort(
    usedSupports.begin(), usedSupports.end(),
      [](
         vtkMedFamilyOnEntityOnProfile* foep1,
         vtkMedFamilyOnEntityOnProfile* foep2 )
      { return
         std::string(foep1->GetFamilyOnEntity()->GetFamily()->GetName())
            <
         std::string(foep2->GetFamilyOnEntity()->GetFamily()->GetName()); });

  int maxprogress=2*this->Internal->UsedSupports.size();
  supportId = 0;
  int it_counter = 0;
  for(auto it=
      usedSupports.begin(); it
      !=usedSupports.end(); it++)
    {
    ostringstream sstr;
    vtkMedFamilyOnEntityOnProfile* foep = *it;
    sstr<<"Support : "<<vtkMedUtilities::SimplifyName(
        foep->GetFamilyOnEntity()->GetFamily()->GetName());
    std::cout<<sstr.str()<<std::endl;
    this->SetProgressText(sstr.str().c_str());
    int doBuildSupportField = 1;
    it_counter++;
    this->BuildVTKSupport(foep, doBuildSupportField);
    this->UpdateProgress((float) progress/((float) maxprogress-1));
    progress++;
    supportId++;
    }

  this->ClearCaches(EndBuildVTKSupports);
  // This call maps the fields to the supports
  for(auto it=
      usedSupports.begin(); it
      !=usedSupports.end(); it++)
    {
    vtkMedFamilyOnEntityOnProfile* foep = *it;
    if((foep->GetValid() == 0) && (this->Internal->NumberOfPieces == 1))
      continue;
    ostringstream sstr;
    sstr<<"Loading fields on "<<vtkMedUtilities::SimplifyName(
        foep->GetFamilyOnEntity()->GetFamily()->GetName());
    std::cout<<sstr.str()<<std::endl;
    this->SetProgressText(sstr.str().c_str());
    int doMapField = 1;
    this->MapFieldsOnSupport(*it, doMapField);
    this->UpdateProgress((float) progress/((float) maxprogress-1));
    progress++;
    supportId++;
    }

  // This call clean up caches (what is actually done depends of the CacheStrategy)
  this->ClearCaches(EndRequest);
  return 1;
}

void vtkMedReader::InitializeCellGlobalIds()
{
  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fileit = this->Internal->MedFiles.begin();
  while(fileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = fileit->second;
    fileit++;
    for(int m=0; m<file->GetNumberOfMesh(); m++)
      {
      vtkMedMesh* mesh = file->GetMesh(m);
      med_int nstep = mesh->GetNumberOfGridStep();
      for(med_int stepid=0; stepid<nstep; stepid++)
        {
        vtkMedGrid* grid = mesh->GetGridStep(stepid);
        grid->InitializeCellGlobalIds();
        }
      }
    }
}

// Method to create the filters for the MED parallel read functions
// It is defined here as we have all information for initialization
void vtkMedReader::InitializeParallelRead()
{
  // If there is only one process for reading no need to enter here
  if (this->Internal->NumberOfPieces <= 1)
    {
    return;
    }

  // FIRST: Generate filters for the cells

  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
  meshfit = this->Internal->MedFiles.begin();
  while(meshfit != this->Internal->MedFiles.end())
    {
    vtkMedFile* meshfile = meshfit->second;
#ifdef DBGOUT
    std::cout<<"init file : "<<meshfile->GetFileName()<<std::endl;
#endif
    meshfit++;
    med_idt pFileID = meshfile->GetMedDriver()->GetParallelFileId();

    for(int mid=0; mid<meshfile->GetNumberOfMesh(); mid++)
      {
      vtkMedMesh* mesh = meshfile->GetMesh(mid);
#ifdef DBGOUT
      std::cout<<" init mesh : "<<mesh->GetDescription()<<std::endl;
#endif
      for(int gid=0; gid<mesh->GetNumberOfGridStep(); gid++)
        {
        vtkMedGrid* grid = mesh->GetGridStep(gid);
#ifdef DBGOUT
        std::cout<<"  init grid : np = "<<grid->GetNumberOfPoints()<<std::endl;
#endif
        // read point family data and create EntityArrays

        for(int eid=0; eid < grid->GetNumberOfEntityArray(); eid++)
         {
          vtkMedEntityArray* array = grid->GetEntityArray(eid);
#ifdef DBGOUT
          std::cout<<"   init array : "<<eid<<", type="<<array->GetEntity().GeometryType<<std::endl;
#endif


          // Next continue is to avoid to create filters for the
          // points, at the moment we charge the points in all nodes
          if (array->GetEntity().GeometryType == MED_POINT1) // !MED_NODE
            continue;

          med_int nbofconstituentpervalue = vtkMedUtilities::GetNumberOfNodes(
                                            array->GetEntity().GeometryType);
          if (nbofconstituentpervalue == -1)
            vtkErrorMacro("Still not implemented for MED_POLYGON and MED_POLYHEDRON"); // à gerer

          // Calculating block sizes
          int nEntity = array->GetNumberOfEntity();
          int block_size = ( nEntity / this->Internal->NumberOfPieces );
          med_size    start  = block_size * this->Internal->CurrentPieceNumber + 1;
          med_size    stride = block_size;
          med_size    count  = 1;
          med_size    blocksize = block_size;
          med_size    lastblocksize = (nEntity % this->Internal->NumberOfPieces);
          if ((this->Internal->NumberOfPieces ==
              this->Internal->CurrentPieceNumber+1) && (lastblocksize != 0))
            {
            blocksize += lastblocksize;
            stride    += lastblocksize;
            }
          lastblocksize = 0;

          vtkMedFilter *filter = vtkMedFilter::New();
          filter->SetFilterSizes( start, stride, count, blocksize, lastblocksize );
          array->SetFilter(filter);
         }//entity array
        }// grid step
      }//mesh
    }//mesh file

  // SECOND: Filters for the Fields

  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fieldfileit;
  // link the FieldOnProfile with the profiles
  fieldfileit = this->Internal->MedFiles.begin();
  while(fieldfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* fieldfile = fieldfileit->second;
    fieldfileit++;
    med_idt pFileID = fieldfile->GetMedDriver()->GetParallelFileId();

    for(int fid=0; fid<fieldfile->GetNumberOfField(); fid++)
      {
      vtkMedField* field = fieldfile->GetField(fid);

      if (field->GetFieldType() == vtkMedField::CellField)
      {
      for(int sid = 0; sid< field->GetNumberOfFieldStep(); sid++)
        {
        vtkMedFieldStep* step = field->GetFieldStep(sid);

        for(int foeid = 0; foeid < step->GetNumberOfFieldOverEntity(); foeid++)
        // TODO : seul le premier pas de temps est dispo au debut
          {
          vtkMedFieldOverEntity* fieldOverEntity = step->GetFieldOverEntity(foeid);

          for(int fopid = 0; fopid < fieldOverEntity->GetNumberOfFieldOnProfile(); fopid++)
            {
            vtkMedFieldOnProfile* fop = fieldOverEntity->GetFieldOnProfile(fopid);
            // Here implement the filters as before:
            // 1- Modify vtkMedFieldOnProfile to contain a filter
            // 2- Create the filters here only if they are on CELLs (use GetFieldType)
            med_int nbofconstituentpervalue = field->GetNumberOfComponent();

            int nVectors = fop->GetNumberOfValues();

            int block_size = ( nVectors / this->Internal->NumberOfPieces );
            int    start  = block_size * this->Internal->CurrentPieceNumber + 1;
            int    stride = block_size;
            int    count  = 1;
            int    blocksize = block_size;
            int    lastblocksize = (nVectors % this->Internal->NumberOfPieces);
            if ((this->Internal->NumberOfPieces ==
                 this->Internal->CurrentPieceNumber+1) && (lastblocksize != 0))
              {
              blocksize += lastblocksize;
              stride    += lastblocksize;
              }
            lastblocksize = 0;

            vtkMedFilter *filter = vtkMedFilter::New();
            filter->SetFilterSizes( start, stride, count, blocksize, lastblocksize );
            fop->SetFilter(filter);
            }
          }
        }
      } // end IF
      }
    }

}

void  vtkMedReader::LinkMedInfo()
{
  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fieldfileit;
  // link the FieldOnProfile with the profiles
  fieldfileit = this->Internal->MedFiles.begin();
  while(fieldfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* fieldfile = fieldfileit->second;
    fieldfileit++;
#ifdef DBGOUT
    std::cout<<"fieldfile "<<fieldfile->GetFileName()<<", "<<fieldfile->GetComment()<<std::endl;
#endif

    for(int fid=0; fid<fieldfile->GetNumberOfField(); fid++)
      {
      vtkMedField* field = fieldfile->GetField(fid);
#ifdef DBGOUT
      std::cout<<" field "<<field->GetName()<<std::endl;
#endif

      for(int sid = 0; sid< field->GetNumberOfFieldStep(); sid++)
        {
        vtkMedFieldStep* step = field->GetFieldStep(sid);
#ifdef DBGOUT
        std::cout << "  step "<<sid<<std::endl;
#endif

        for(int foeid = 0; foeid < step->GetNumberOfFieldOverEntity(); foeid++)
          {
          vtkMedFieldOverEntity* fieldOverEntity = step->GetFieldOverEntity(foeid);
#ifdef DBGOUT
          std::cout<<"   fieldOverEntity "<<foeid<<std::endl;
#endif

          for(int fopid = 0; fopid < fieldOverEntity->GetNumberOfFieldOnProfile(); fopid++)
            {
            vtkMedFieldOnProfile* fop = fieldOverEntity->GetFieldOnProfile(fopid);
#ifdef DBGOUT
            std::cout<<"    fop '"<<fop->GetProfileName()<<"'"<<std::endl;
#endif

            std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
                profilefileit = this->Internal->MedFiles.begin();
            while(profilefileit != this->Internal->MedFiles.end() && fop->GetProfile() == NULL)
              {
              vtkMedFile* profilefile = profilefileit->second;
#ifdef DBGOUT
              std::cout<<"     profilefile "<<profilefile->GetFileName()<<std::endl;
#endif
              profilefileit++;

              for(int pid = 0; pid < profilefile->GetNumberOfProfile(); pid++)
                {
                vtkMedProfile *profile = profilefile->GetProfile(pid);
#ifdef DBGOUT
                std::cout<<"      profile "<<profile->GetName()<<" => "<<(strcmp(profile->GetName(), fop->GetProfileName())==0?"OK":"nope")<<std::endl;
#endif
                if(strcmp(profile->GetName(), fop->GetProfileName()) == 0)
                  {
                  fop->SetProfile(profile);
                  break;
                  }
                }
              }
            }
          }
        }
      }
    }

  // first, add a familyOnEntityOnProfile to all FamilyOnEntity with a NULL
  // profile. This is used if no field is mapped to this support.
  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      meshfit = this->Internal->MedFiles.begin();
  while(meshfit != this->Internal->MedFiles.end())
    {
    vtkMedFile* meshfile = meshfit->second;
#ifdef DBGOUT
    std::cout<<"meshfile "<<meshfile->GetFileName()<<std::endl;
#endif
    meshfit++;

    for(int mid=0; mid<meshfile->GetNumberOfMesh(); mid++)
      {
      vtkMedMesh* mesh = meshfile->GetMesh(mid);
#ifdef DBGOUT
      std::cout<<" mesh "<<mesh->GetName()<<std::endl;
#endif

      for(int gid=0; gid<mesh->GetNumberOfGridStep(); gid++)
        {
        vtkMedGrid* grid = mesh->GetGridStep(gid);
#ifdef DBGOUT
        std::cout<<"  grid np="<<grid->GetNumberOfPoints()<<endl;
#endif
        // read point family data and create EntityArrays

        for(int eid=0; eid < grid->GetNumberOfEntityArray(); eid++)
          {
          vtkMedEntityArray* array = grid->GetEntityArray(eid);
#ifdef DBGOUT
          std::cout<<"   array eid="<<eid<<" geometry name="<<array->GetEntity().GeometryName<<std::endl;
#endif

          for(int fid=0; fid < array->GetNumberOfFamilyOnEntity(); fid++)
            {
            vtkMedFamilyOnEntity* foe = array->GetFamilyOnEntity(fid);
#ifdef DBGOUT
            std::cout<<"    foe family name="<<foe->GetFamily()->GetName()<<std::endl;
#endif
            if(foe->GetFamilyOnEntityOnProfile((vtkMedProfile*)NULL) == NULL)
              {
              vtkMedFamilyOnEntityOnProfile* foep =
                  vtkMedFamilyOnEntityOnProfile::New();
              foep->SetFamilyOnEntity(foe);
              foep->SetProfile(NULL);
              foe->AddFamilyOnEntityOnProfile(foep);
              foep->Delete();
              }
            }//family on entity
          }//entity array
        }// grid step
      }//mesh
    }//mesh file

  fieldfileit = this->Internal->MedFiles.begin();
  while(fieldfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* fieldfile = fieldfileit->second;
#ifdef DBGOUT
    std::cout<<"fieldfile "<<fieldfile->GetFileName()<<std::endl;
#endif
    fieldfileit++;

    for(int fieldid=0; fieldid < fieldfile->GetNumberOfField(); fieldid++)
      {
      vtkMedField* field = fieldfile->GetField(fieldid);
#ifdef DBGOUT
      std::cout<<" field "<<field->GetName()<<std::endl;
#endif

      for(int fstepid=0; fstepid < field->GetNumberOfFieldStep(); fstepid++)
        {
        vtkMedFieldStep* step = field->GetFieldStep(fstepid);

        vtkMedComputeStep meshcs = step->GetMeshComputeStep();

#ifdef DBGOUT
        std::cout<<"  step meshcs.TimeOrFrequency="<<meshcs.TimeOrFrequency<<std::endl;
#endif

        for(int foeid=0; foeid<step->GetNumberOfFieldOverEntity() ;foeid++)
          {
          vtkMedFieldOverEntity* fieldOverEntity = step->GetFieldOverEntity(foeid);
          const vtkMedEntity& fieldentity = fieldOverEntity->GetEntity();
#ifdef DBGOUT
          std::cout<<"   fieldOverEntity GeometryName='"<<fieldentity.GeometryName<<"'"<<std::endl;
#endif

          for (int fopid = 0;
               fopid < fieldOverEntity->GetNumberOfFieldOnProfile(); fopid++)
            {
            vtkMedFieldOnProfile* fop =
                fieldOverEntity->GetFieldOnProfile(fopid);
#ifdef DBGOUT
            std::cout<<"    fop ProfileName="<<fop->GetProfileName()<<std::endl;
#endif

            std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
                meshfileit = this->Internal->MedFiles.begin();
            while(meshfileit != this->Internal->MedFiles.end())
              {
              vtkMedFile* meshfile = meshfileit->second;
              meshfileit++;

#ifdef DBGOUT
              std::cout<<"     meshfile "<<meshfile->GetFileName()<<std::endl;
#endif

              if(field->GetLocal() == 1 && (meshfile != fieldfile))
                continue;

              for(int mid=0; mid<meshfile->GetNumberOfMesh(); mid++)
                {
                vtkMedMesh* mesh = meshfile->GetMesh(mid);
#ifdef DBGOUT
                std::cout<<"      mesh "<<mesh->GetName()<<std::endl;
#endif

                // the field must be on this mesh.
                if(strcmp(mesh->GetName(),
                          field->GetMeshName()) != 0)
                  continue;

                vtkMedGrid* grid = mesh->GetGridStep(meshcs);
                if(grid == NULL)
                  {
                  vtkErrorMacro("the field " << field->GetName()
                                << " at step iteration:"
                                << step->GetComputeStep().IterationIt
                                << " and time "
                                << step->GetComputeStep().TimeIt
                                << " uses mesh at step "
                                << meshcs.TimeIt << " " << meshcs.IterationIt
                                << "which does not exists!");
                  continue;
                  }

                for(int eid=0; eid < grid->GetNumberOfEntityArray(); eid++)
                  {
                  vtkMedEntityArray* array = grid->GetEntityArray(eid);
#ifdef DBGOUT
                  std::cout<<"       array "<<array->GetEntity().GeometryName<<std::endl;
#endif

                  // if the support is on points,
                  // the field must also be on points
                  if(array->GetEntity().EntityType == MED_NODE &&
                     fieldentity.EntityType != MED_NODE)
                    continue;

                  if(array->GetEntity().EntityType != MED_NODE &&
                     fieldentity.EntityType == MED_NODE)
                    continue;

                  // for fields not on points, the geometry type
                  // of the support must match
                  if(array->GetEntity().EntityType != MED_NODE &&
                     array->GetEntity().GeometryType != fieldentity.GeometryType)
                    continue;

                  for(int fid = 0; fid < array->GetNumberOfFamilyOnEntity(); fid++)
                    {
                    vtkMedFamilyOnEntity* foe = array->GetFamilyOnEntity(fid);
#ifdef DBGOUT
                    std::cout<<"        foe "<<foe->GetFamily()->GetName()<<std::endl;
#endif

                    if(foe->GetFamilyOnEntityOnProfile(fop->GetProfile()) == NULL)
                      {
                      vtkMedFamilyOnEntityOnProfile* foep =
                          vtkMedFamilyOnEntityOnProfile::New();
                      foep->SetProfile(fop->GetProfile());
                      foep->SetFamilyOnEntity(foe);
                      foe->AddFamilyOnEntityOnProfile(foep);
#ifdef DBGOUT
                      std::cout<<"         foep profile="<<foep->GetProfile()->GetName()<<std::endl;
#endif
                      foep->Delete();
                      }
#ifdef DBGOUT
                    else
                        std::cout<<"         foep (none)"<<std::endl;
#endif

                    // also add the family on entity with no profile.
                    if(foe->GetFamilyOnEntityOnProfile((vtkMedProfile*)NULL) == NULL)
                      {
                      vtkMedFamilyOnEntityOnProfile* foep =
                          vtkMedFamilyOnEntityOnProfile::New();
                      foep->SetProfile(NULL);
                      foep->SetFamilyOnEntity(foe);
                      foe->AddFamilyOnEntityOnProfile(foep);
#ifdef DBGOUT
                      std::cout<<"         foep profile=null"<<std::endl;
#endif
                      foep->Delete();
                      }
#ifdef DBGOUT
                    else
                        std::cout<<"         foep (none)"<<std::endl;
#endif

                    }//familyOnEntity
                  }//entityArray
                }//mesh
              }//mesh file
            }//field on profile
          }//fieldOverEntity
        }//fiedstep
      }// fieldid
    }//fieldfileit

  // Now, link localizations and interpolations
  fieldfileit = this->Internal->MedFiles.begin();
  while(fieldfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* fieldfile = fieldfileit->second;
    fieldfileit++;
#ifdef DBGOUT
    std::cout<<"fieldfile "<<fieldfile->GetFileName()<<std::endl;
#endif

    for(int locid = 0; locid < fieldfile->GetNumberOfLocalization(); locid++)
      {
      vtkMedLocalization* loc = fieldfile->GetLocalization(locid);
#ifdef DBGOUT
      std::cout<<" loc "<<loc->GetName()<<" ip name="<<loc->GetInterpolationName()<<std::endl;
#endif

      for(int fid = 0; fid < fieldfile->GetNumberOfField() &&
                    loc->GetInterpolation() == NULL; fid++)
        {
        vtkMedField* field = fieldfile->GetField(fid);
#ifdef DBGOUT
        std::cout<<"  field "<<field->GetName()<<std::endl;
#endif
        for(int interpid = 0; interpid < field->GetNumberOfInterpolation();
        interpid++)
          {
          vtkMedInterpolation* interp = field->GetInterpolation(interpid);
#ifdef DBGOUT
          std::cout<<"   interp "<<interp->GetName()<<std::endl;
#endif
          if(strcmp(loc->GetInterpolationName(),
                    interp->GetName()) == 0)
            {
#ifdef DBGOUT
              std::cout<<"    >> set"<<std::endl;
#endif
            loc->SetInterpolation(interp);
            break;
            }
          }
        }
      }
    }

  // now that the interpolation is set, build the  shape functions.
  fieldfileit = this->Internal->MedFiles.begin();
  while(fieldfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* fieldfile = fieldfileit->second;
    fieldfileit++;

    for(int locid = 0; locid < fieldfile->GetNumberOfLocalization(); locid++)
      {
      vtkMedLocalization* loc = fieldfile->GetLocalization(locid);
      loc->BuildShapeFunction();
#ifdef DBGOUT
      std::cout<<"set shape fieldfile="<<fieldfile->GetFileName()<<" loc="<<loc->GetName()<<std::endl;
#endif
      }
    }

  // set the supportmesh pointer in the structural element
  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fileit = this->Internal->MedFiles.begin();
  while(fileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = fileit->second;
    fileit++;

    for(int structelemit = 0;
        structelemit < file->GetNumberOfStructElement();
        structelemit++)
      {
      vtkMedStructElement* structElem =
          file->GetStructElement(structelemit);

      for(int supportmeshit = 0;
          supportmeshit < file->GetNumberOfSupportMesh();
          supportmeshit++)
        {
        vtkMedMesh* supportMesh =
            file->GetSupportMesh(supportmeshit);

        if(strcmp(supportMesh->GetName(), structElem->GetName()) == 0 )
          {
          structElem->SetSupportMesh(supportMesh);
#ifdef DBGOUT
          std::cout<<"set support mesh"<<std::endl;
#endif
          break;
          }
        }
      }
    }

  // set the pointer to the profile used by the constant attributes
  fileit = this->Internal->MedFiles.begin();
  while(fileit != this->Internal->MedFiles.end())
  {
  vtkMedFile* file = fileit->second;
  fileit++;

  for(int structelemit = 0;
      structelemit < file->GetNumberOfStructElement();
      structelemit++)
    {
    vtkMedStructElement* structElem =
        file->GetStructElement(structelemit);

    for(int cstattit = 0; cstattit < structElem->GetNumberOfConstantAttribute(); cstattit++)
      {
      vtkMedConstantAttribute* cstatt = structElem->GetConstantAttribute(cstattit);

      for(int profit = 0;
          profit < file->GetNumberOfProfile();
          profit++)
        {
        vtkMedProfile* profile =
            file->GetProfile(profit);

        if(strcmp(profile->GetName(), cstatt->GetProfileName()) == 0 )
          {
          cstatt->SetProfile(profile);
#ifdef DBGOUT
          std::cout<<"set profile"<<std::endl;
#endif
          break;
          }
        }
      }
    }
  }

  meshfit = this->Internal->MedFiles.begin();
  while(meshfit != this->Internal->MedFiles.end())
  {
  vtkMedFile* meshfile = meshfit->second;
  meshfit++;

  for(int mid=0; mid<meshfile->GetNumberOfMesh(); mid++)
    {
    vtkMedMesh* mesh = meshfile->GetMesh(mid);

    for(int gid=0; gid<mesh->GetNumberOfGridStep(); gid++)
      {
      vtkMedGrid* grid = mesh->GetGridStep(gid);
      // read point family data and create EntityArrays

      for(int eid=0; eid < grid->GetNumberOfEntityArray(); eid++)
        {
        vtkMedEntityArray* array = grid->GetEntityArray(eid);
        if(array->GetEntity().EntityType != MED_STRUCT_ELEMENT)
          continue;

        for(int structelemit = 0; structelemit < meshfile->GetNumberOfStructElement(); structelemit++)
          {
          vtkMedStructElement* structelem = meshfile->GetStructElement(structelemit);
          if(structelem->GetGeometryType() == array->GetEntity().GeometryType)
            {
            array->SetStructElement(structelem);
#ifdef DBGOUT
            std::cout<<"set struct elem"<<std::endl;
#endif
            break;
            }
          }
        }
      }
    }
  }
}

int vtkMedReader::GetFrequencyArrayStatus(const char* name)
{
  return this->Frequencies->GetKeyStatus(name);
}

void vtkMedReader::SetFrequencyArrayStatus(const char* name, int status)
{
  if(this->Frequencies->GetKeyStatus(name) == status)
    {
    return;
    }

  this->Frequencies->SetKeyStatus(name, status);

  this->Modified();
}

int vtkMedReader::GetNumberOfFrequencyArrays()
{
  return this->Frequencies->GetNumberOfKey();
}

const char* vtkMedReader::GetFrequencyArrayName(int index)
{
  return this->Frequencies->GetKey(index);
}

struct compTimes
{
  bool operator()(pair<double, int> i, pair<double, int> j)
  {
    if(i.first!=j.first)
      return (i.first<j.first);
    return i.second<j.second;
  }
};

vtkDoubleArray* vtkMedReader::GetAvailableTimes()
{
  this->AvailableTimes->Initialize();
  this->AvailableTimes->SetNumberOfComponents(1);

  std::set<std::string> newFrequencies;

  int tid = 0;
  std::map<med_float, std::set<med_int> >::iterator it =
      this->Internal->GlobalComputeStep.begin();
  while(it != this->Internal->GlobalComputeStep.end())
    {
    double time = it->first;
    this->AvailableTimes->InsertNextValue(time);
    string name = vtkMedUtilities::GetModeKey(tid, time, this->Internal->GlobalComputeStep.size()-1);
    this->Frequencies->AddKey(name.c_str());
    newFrequencies.insert(name);
    tid++;
    it++;
    }

  // now check if old frequencies have been removed
  for(int f = 0; f < this->Frequencies->GetNumberOfKey(); f++)
    {
    const char* name = this->Frequencies->GetKey(f);
    if(newFrequencies.find(name) == newFrequencies.end())
      {
      this->Frequencies->RemoveKeyByIndex(f);
      f--;
      }
    }

  return this->AvailableTimes;
}

void vtkMedReader::ChooseRealAnimationMode()
{
  if(this->AnimationMode!=Default)
    {
    this->Internal->RealAnimationMode=this->AnimationMode;
    return;
    }

  // if there is exactly one physical time and more than one iteration
  // set the animation mode to iteration, else default to physical time.
  if (this->Internal->GlobalComputeStep.size() == 1 &&
      this->Internal->GlobalComputeStep[0].size() > 1)
    {
    this->Internal->RealAnimationMode=Iteration;
    return;
    }

  this->Internal->RealAnimationMode=PhysicalTime;
}

int vtkMedReader::GetEntityStatus(const vtkMedEntity& entity)
{
  if (entity.EntityType==MED_NODE)
    return 1;
  if(entity.EntityType == MED_DESCENDING_FACE
     || entity.EntityType == MED_DESCENDING_EDGE)
    return 0;

  return this->Entities->GetKeyStatus(vtkMedUtilities::EntityKey(entity).c_str());
}

int vtkMedReader::GetFamilyStatus(vtkMedMesh* mesh, vtkMedFamily* family)
{
  if(!mesh||!family)
    return 0;

  if(this->Internal->GroupSelectionMTime > this->Internal->FamilySelectionMTime)
    {
    this->SelectFamiliesFromGroups();
    }

  int status =  this->Internal->Families->GetKeyStatus(vtkMedUtilities::FamilyKey(
      mesh->GetName(), family->GetPointOrCell(),
      family->GetName()).c_str());

  return status;
}

int vtkMedReader::IsMeshSelected(vtkMedMesh* mesh)
{
  for(int fam=0; fam<mesh->GetNumberOfPointFamily(); fam++)
    {
    if(this->GetFamilyStatus(mesh, mesh->GetPointFamily(fam))!=0)
      return 1;
    }

  for(int fam=0; fam<mesh->GetNumberOfCellFamily(); fam++)
    {
    if(this->GetFamilyStatus(mesh, mesh->GetCellFamily(fam))!=0)
      return 1;
    }
  return 0;
}

void vtkMedReader::GatherComputeSteps()
{
  this->Internal->GlobalComputeStep.clear();

  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fieldfileit = this->Internal->MedFiles.begin();
  while(fieldfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = fieldfileit->second;
    fieldfileit++;

    // first loop over all fields to gather their compute steps
    for(int fieldId=0; fieldId<file->GetNumberOfField(); fieldId++)
      {
      vtkMedField* field=file->GetField(fieldId);

      for(int stepId=0; stepId<field->GetNumberOfFieldStep(); stepId++)
        {
        vtkMedFieldStep* step=field->GetFieldStep(stepId);
        const vtkMedComputeStep& cs = step->GetComputeStep();
        this->Internal->GlobalComputeStep[cs.TimeOrFrequency].insert(cs.IterationIt);
        }
      }//fields

    // then loop over all meshes to gather their grid steps too.
    // for meshes, do not add the MED_UNDEF_DT time
    for(int meshId=0; meshId<file->GetNumberOfMesh(); meshId++)
      {
      vtkMedMesh* mesh=file->GetMesh(meshId);

      for(int stepId=0; stepId<mesh->GetNumberOfGridStep(); stepId++)
        {
        vtkMedGrid* grid=mesh->GetGridStep(stepId);
        const vtkMedComputeStep& cs = grid->GetComputeStep();
        if(cs.TimeOrFrequency != MED_UNDEF_DT || cs.TimeIt != MED_NO_DT)
          {
          this->Internal->GlobalComputeStep[cs.TimeOrFrequency].insert(cs.IterationIt);
          }
        }
      }//mesh
    }
  if(this->Internal->GlobalComputeStep.size() == 0)
    {
    this->Internal->GlobalComputeStep[MED_UNDEF_DT].insert(MED_NO_IT);
    }
}

int vtkMedReader::IsFieldSelected(vtkMedField* field)
{
  return this->IsPointFieldSelected(field)||this->IsCellFieldSelected(field)
      ||this->IsQuadratureFieldSelected(field) || this->IsElnoFieldSelected(field);
}

int vtkMedReader::IsPointFieldSelected(vtkMedField* field)
{
  return field->GetFieldType()==vtkMedField::PointField
      &&this->GetPointFieldArrayStatus(vtkMedUtilities::SimplifyName(
          field->GetName()).c_str());
}

int vtkMedReader::IsCellFieldSelected(vtkMedField* field)
{
  return field->GetFieldType()==vtkMedField::CellField
      &&this->GetCellFieldArrayStatus(vtkMedUtilities::SimplifyName(
          field->GetName()).c_str());
}

vtkMedProfile* vtkMedReader::GetProfile(const char* pname)
{
  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fileit = this->Internal->MedFiles.begin();
  while(fileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = fileit->second;
    fileit++;
    vtkMedProfile* profile = file->GetProfile(pname);
    if(profile != NULL)
      return profile;
    }
  return NULL;
}

vtkMedLocalization* vtkMedReader::GetLocalization(const char* lname)
{
  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fileit = this->Internal->MedFiles.begin();
  while(fileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = fileit->second;
    fileit++;
    vtkMedLocalization* loc = file->GetLocalization(lname);
    if(loc != NULL)
      return loc;
    }
  return NULL;

}

int vtkMedReader::GetLocalizationKey(vtkMedFieldOnProfile* fop)
{
  vtkMedLocalization* def=this->GetLocalization(fop->GetLocalizationName());

  // This is not a quadrature field with explicit definition.
  // There are two possible cases : either the intergration point is
  // at the center of the cell
  //1 quadrature point at the cell center
  int nloc = 0;
  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fileit = this->Internal->MedFiles.begin();
  while(fileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = fileit->second;
    fileit++;

    if(def && def->GetParentFile() == file)
      return nloc + def->GetMedIterator() - 1;

    nloc += file->GetNumberOfLocalization();
    }

  // center of a cell
  if(fop->GetNumberOfIntegrationPoint()==1)
    return nloc + 1 + fop->GetParentFieldOverEntity()->GetEntity().GeometryType;

  // or it is an elno field (field stored on nodes of the cells,
  // but with discontinuities at the vertices)
  return -fop->GetParentFieldOverEntity()->GetEntity().GeometryType;//ELNO
}

int vtkMedReader::IsQuadratureFieldSelected(vtkMedField* field)
{
  return field->GetFieldType()==vtkMedField::QuadratureField
      &&this->GetQuadratureFieldArrayStatus(vtkMedUtilities::SimplifyName(
          field->GetName()).c_str());
}

int vtkMedReader::IsElnoFieldSelected(vtkMedField* field)
{
  return field->GetFieldType()==vtkMedField::ElnoField
      &&this->GetElnoFieldArrayStatus(vtkMedUtilities::SimplifyName(
          field->GetName()).c_str());
}

// Description:
// Give the animation steps to the pipeline
void vtkMedReader::AdvertiseTime(vtkInformation* info)
{
  this->ChooseRealAnimationMode();

  if(this->Internal->RealAnimationMode==PhysicalTime)
    {
    // I advertise the union of all times available
    // in all selected fields and meshes
    set<double> timeset;

    std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
        fieldfileit = this->Internal->MedFiles.begin();
    while(fieldfileit != this->Internal->MedFiles.end())
      {
      vtkMedFile* file = fieldfileit->second;
      fieldfileit++;

      // first loop over all fields to gather their compute steps
      for(int fieldId=0; fieldId<file->GetNumberOfField(); fieldId++)
        {
        vtkMedField* field=file->GetField(fieldId);

        if(!this->IsFieldSelected(field))
          continue;

        field->GatherFieldTimes(timeset);
        }//fields

      // then loop over all meshes to gather their grid steps too.
      for(int meshId=0; meshId<file->GetNumberOfMesh(); meshId++)
        {
        vtkMedMesh* mesh=file->GetMesh(meshId);

        if(!this->IsMeshSelected(mesh))
          continue;

        mesh->GatherGridTimes(timeset);
        }//meshes
      }

    if(timeset.size() > 0)
      {
      // remove MED_UNDEF_DT if there are other time step
      if(timeset.size() > 1)
        timeset.erase(MED_UNDEF_DT);

      vector<double> times;
      set<double>::iterator it = timeset.begin();
      while(it != timeset.end())
        {
        times.push_back(*it);
        it++;
        }
      sort(times.begin(), times.end());

      info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0],
          times.size());
      double timeRange[2];
      timeRange[0]=times[0];
      timeRange[1]=times[times.size()-1];
      info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), &timeRange[0],
          2);
      }
    else
      {
      info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      }
    }
  else if(this->Internal->RealAnimationMode==Iteration)
    {
    // I advertise the union of all iterations available at the given
    // Time for all selected fields.
    set<med_int> iterationsets;
    med_float time = MED_UNDEF_DT;
    if(this->TimeIndexForIterations >= 0 &&
       this->TimeIndexForIterations <
       this->AvailableTimes->GetNumberOfTuples())
      {
      time = this->AvailableTimes->
                     GetValue((vtkIdType)this->TimeIndexForIterations);
      }

    std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
        fieldfileit = this->Internal->MedFiles.begin();
    while(fieldfileit != this->Internal->MedFiles.end())
      {
      vtkMedFile* file = fieldfileit->second;
      fieldfileit++;

      for(int fieldId=0; fieldId<file->GetNumberOfField(); fieldId++)
        {
        vtkMedField* field=file->GetField(fieldId);
        if(!this->IsFieldSelected(field))
          continue;

        field->GatherFieldIterations(time, iterationsets);
        }
      // then loop over all meshes to gather their grid steps too.
      for(int meshId=0; meshId<file->GetNumberOfMesh(); meshId++)
        {
        vtkMedMesh* mesh=file->GetMesh(meshId);

        if(!this->IsMeshSelected(mesh))
          continue;

        mesh->GatherGridIterations(time, iterationsets);
        }//meshes
      }

    if(iterationsets.size()>0)
      {
      // remove MED_NO_IT if there are other available iterations.
      if(iterationsets.size()>1)
        iterationsets.erase(MED_NO_IT);

      vector<double> iterations;
      set<med_int>::iterator it=iterationsets.begin();
      while(it!=iterationsets.end())
        {
        iterations.push_back((double)*it);
        it++;
        }
      sort(iterations.begin(), iterations.end());
      info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &iterations[0],
          iterations.size());
      double timeRange[2];
      timeRange[0]=iterations[0];
      timeRange[1]=iterations[iterations.size()-1];
      info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), &timeRange[0],
          2);
      }
    else
      {
      info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      }
    }
  else
    {
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }
}

vtkIdType vtkMedReader::GetFrequencyIndex(double freq)
{
  return this->AvailableTimes->LookupValue(freq);
}

int vtkMedReader::RequestDataObject(vtkInformation* request,
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation *info = outputVector->GetInformationObject(0);
  if (vtkMultiBlockDataSet::SafeDownCast(
      info->Get(vtkDataObject::DATA_OBJECT())))
    {
    // The output is already created
    return 1;
    }
  else
    {
    vtkMultiBlockDataSet* output=vtkMultiBlockDataSet::New();
    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
        output->GetExtentType());
    }
  return 1;
}

void vtkMedReader::ClearSelections()
{
  this->PointFields->Initialize();
  this->CellFields->Initialize();
  this->QuadratureFields->Initialize();
  this->ElnoFields->Initialize();

  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fieldfileit = this->Internal->MedFiles.begin();
  while(fieldfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = fieldfileit->second;
    fieldfileit++;

    for(int index=0; index < file->GetNumberOfField(); index++)
      {
      vtkMedField* field = file->GetField(index);
#ifdef DBGOUT
      std::cout<<index<<" "<<vtkMedUtilities::SimplifyName(
                     field->GetName())<<" "<<field->GetName()<<" "<<field->GetFieldType()<<std::endl;
#endif
      switch(field->GetFieldType())
        {
        case vtkMedField::PointField :
        this->PointFields->AddKey(vtkMedUtilities::SimplifyName(
              field->GetName()).c_str());
        break;
        case vtkMedField::CellField :
        this->CellFields->AddKey(vtkMedUtilities::SimplifyName(
              field->GetName()).c_str());
        break;
        case vtkMedField::QuadratureField :
        this->QuadratureFields->AddKey(vtkMedUtilities::SimplifyName(
              field->GetName()).c_str());
        break;
        case vtkMedField::ElnoField :
        this->ElnoFields->AddKey(vtkMedUtilities::SimplifyName(
              field->GetName()).c_str());
        break;
        }
      }

    this->Internal->Families->Initialize();
    this->Groups->Initialize();
    for(int meshIndex=0; meshIndex < file->GetNumberOfMesh(); meshIndex++)
      {
      vtkMedMesh* mesh = file->GetMesh(meshIndex);
      for(int famIndex=0; famIndex<mesh->GetNumberOfPointFamily(); famIndex++)
        {
        vtkMedFamily* fam=mesh->GetPointFamily(famIndex);

        int ng=fam->GetNumberOfGroup();
        for(int gindex=0; gindex<ng; gindex++)
          {
          vtkMedGroup* group=fam->GetGroup(gindex);
          string gname=vtkMedUtilities::GroupKey(mesh->GetName(),
              fam->GetPointOrCell(), group->GetName());

          this->Groups->AddKey(gname.c_str());
          this->Groups->SetKeyStatus(gname.c_str(), 0);
          }
        }
      for(int famIndex=0; famIndex<mesh->GetNumberOfCellFamily(); famIndex++)
        {
        vtkMedFamily* fam=mesh->GetCellFamily(famIndex);

        int ng=fam->GetNumberOfGroup();
        for(int gindex=0; gindex<ng; gindex++)
          {
          vtkMedGroup* group=fam->GetGroup(gindex);
          string gname=vtkMedUtilities::GroupKey(mesh->GetName(),
              fam->GetPointOrCell(), group->GetName());

          this->Groups->AddKey(gname.c_str());
          this->Groups->SetKeyStatus(gname.c_str(), 1);
          }
        }
      }
    this->Internal->GroupSelectionMTime.Modified();

    for(int meshIndex=0; meshIndex< file->GetNumberOfMesh(); meshIndex++)
      {
      if(file->GetMesh(meshIndex)->GetNumberOfGridStep() == 0)
        continue;

      vtkMedGrid* grid=file->GetMesh(meshIndex)->GetGridStep(0);

      for(int entityIndex=0; entityIndex<grid->GetNumberOfEntityArray();
        entityIndex++)
        {
        vtkMedEntityArray* array=grid->GetEntityArray(entityIndex);
        string name=vtkMedUtilities::EntityKey(array->GetEntity());
        this->Entities->AddKey(name.c_str());
        }
      }
    }
  this->Modified();
}

void vtkMedReader::SelectFamiliesFromGroups()
{
  this->Internal->Families->Initialize();

  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      meshfileit = this->Internal->MedFiles.begin();
  while(meshfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = meshfileit->second;
    meshfileit++;

    for(int meshIndex=0; meshIndex < file->GetNumberOfMesh(); meshIndex++)
      {
      vtkMedMesh* mesh = file->GetMesh(meshIndex);
      for(int famIndex=0; famIndex<mesh->GetNumberOfFamily(); famIndex++)
        {
        vtkMedFamily* fam=mesh->GetFamily(famIndex);
        string name=vtkMedUtilities::FamilyKey(mesh->GetName(),
            fam->GetPointOrCell(), fam->GetName());

        this->Internal->Families->SetKeyStatus(name.c_str(), 0);

        for(int gindex=0; gindex<fam->GetNumberOfGroup(); gindex++)
          {
          vtkMedGroup* group=fam->GetGroup(gindex);
          string gname=vtkMedUtilities::GroupKey(mesh->GetName(),
              fam->GetPointOrCell(), group->GetName());
          int state=this->Groups->GetKeyStatus(gname.c_str());

          if(state)
            {
            this->SetFamilyStatus(name.c_str(), 1);
            }
          }
        }
      }
  }

  this->Internal->FamilySelectionMTime.Modified();
}

int vtkMedReader::GetNumberOfPointFieldArrays()
{
  return this->PointFields->GetNumberOfKey();
}

const char*
vtkMedReader::GetPointFieldArrayName(int index)
{
  return this->PointFields->GetKey(index);
}

int vtkMedReader::GetPointFieldArrayStatus(const char* name)
{
  return this->PointFields->GetKeyStatus(name);
}

void vtkMedReader::SetPointFieldArrayStatus(const char* name, int status)
{
  if(this->PointFields->KeyExists(name)&&this->PointFields->GetKeyStatus(
      name)==status)
    return;

  this->PointFields->SetKeyStatus(name, status);

  this->Modified();
}

int vtkMedReader::GetNumberOfCellFieldArrays()
{
  return this->CellFields->GetNumberOfKey();
}

const char*
vtkMedReader::GetCellFieldArrayName(int index)
{
  return this->CellFields->GetKey(index);
}

int vtkMedReader::GetCellFieldArrayStatus(const char* name)
{
  return this->CellFields->GetKeyStatus(name);
}

void vtkMedReader::SetCellFieldArrayStatus(const char* name, int status)
{
  if(this->CellFields->KeyExists(name)&&this->CellFields->GetKeyStatus(
      name)==status)
    return;

  this->CellFields->SetKeyStatus(name, status);

  this->Modified();
}

int vtkMedReader::GetNumberOfQuadratureFieldArrays()
{
  return this->QuadratureFields->GetNumberOfKey();
}

const char* vtkMedReader::GetQuadratureFieldArrayName(int index)
{
  return this->QuadratureFields->GetKey(index);
}

int vtkMedReader::GetQuadratureFieldArrayStatus(const char* name)
{
  return this->QuadratureFields->GetKeyStatus(name);
}

void vtkMedReader::SetQuadratureFieldArrayStatus(const char* name, int status)
{
  if(this->QuadratureFields->KeyExists(name)
      &&this->QuadratureFields->GetKeyStatus(name)==status)
    return;

  this->QuadratureFields->SetKeyStatus(name, status);

  this->Modified();
}

int vtkMedReader::GetNumberOfElnoFieldArrays()
{
  return this->ElnoFields->GetNumberOfKey();
}

const char* vtkMedReader::GetElnoFieldArrayName(int index)
{
  return this->ElnoFields->GetKey(index);
}

int vtkMedReader::GetElnoFieldArrayStatus(const char* name)
{
  return this->ElnoFields->GetKeyStatus(name);
}

void vtkMedReader::SetElnoFieldArrayStatus(const char* name, int status)
{
  if(this->ElnoFields->KeyExists(name)
      &&this->ElnoFields->GetKeyStatus(name)==status)
    return;

  this->ElnoFields->SetKeyStatus(name, status);

  this->Modified();
}

void vtkMedReader::SetEntityStatus(const char* name, int status)
{
  if(this->Entities->KeyExists(name)&&this->Entities->GetKeyStatus(name)
      ==status)
    return;

  this->Entities->SetKeyStatus(name, status);

  this->Modified();
}

void vtkMedReader::SetFamilyStatus(const char* name, int status)
{
  if(this->Internal->Families->KeyExists(name)
      &&this->Internal->Families->GetKeyStatus(name)==status)
    return;

  this->Internal->Families->SetKeyStatus(name, status);
}

void vtkMedReader::SetGroupStatus(const char* name, int status)
{

  if(this->Groups->KeyExists(name)&&this->Groups->GetKeyStatus(name)
      ==status)
    return;

  this->Groups->SetKeyStatus(name, status);

  this->Modified();

  this->Internal->GroupSelectionMTime.Modified();
}

int vtkMedReader::GetGroupStatus(const char* key)
{
  return this->Groups->GetKeyStatus(key);
}

void vtkMedReader::AddQuadratureSchemeDefinition(vtkInformation* info,
    vtkMedLocalization* loc)
{
  if(info==NULL||loc==NULL)
    return;

  vtkInformationQuadratureSchemeDefinitionVectorKey *key=
      vtkQuadratureSchemeDefinition::DICTIONARY();

  vtkQuadratureSchemeDefinition* def=vtkQuadratureSchemeDefinition::New();
  int cellType=vtkMedUtilities::GetVTKCellType(loc->GetGeometryType());
  // Control to avoid crahs when loading a file with structural elements.
  // This should be removed in version 7.1.0 of SALOME.
  // See mantis issue 21990
  if(loc->GetGeometryType() >= MED_STRUCT_GEO_INTERNAL)    
    {
    vtkErrorMacro("You are loading a file containing structural elements BUT they are still not supported");
    return;
    }
  if(loc->GetWeights()->GetVoidPointer(0) ==  NULL)
    return;
  def->Initialize(cellType, vtkMedUtilities::GetNumberOfPoint(
      loc->GetGeometryType()), loc->GetNumberOfQuadraturePoint(),
      (double*)loc->GetShapeFunction()->GetVoidPointer(0),
      (double*)loc->GetWeights()->GetVoidPointer(0));
  key->Set(info, def, cellType);
  def->Delete();

}

void vtkMedReader::LoadConnectivity(vtkMedEntityArray* array)
{
  vtkMedGrid* grid = array->GetParentGrid();
  array->LoadConnectivity();
  if (array->GetConnectivity()==MED_NODAL||vtkMedUtilities::GetDimension(
      array->GetEntity().GeometryType)<2
      || grid->GetParentMesh()->GetMeshType() == MED_STRUCTURED_MESH)
    return;

  vtkMedEntity subentity;
  subentity.EntityType = vtkMedUtilities::GetSubType(array->GetEntity().EntityType);

  vtkMedUnstructuredGrid* ugrid = vtkMedUnstructuredGrid::SafeDownCast(grid);
  if(ugrid == NULL)
    return;

  for(int index=0; index<vtkMedUtilities::GetNumberOfSubEntity(
      array->GetEntity().GeometryType); index++)
    {
    subentity.GeometryType = vtkMedUtilities::GetSubGeometry(
        array->GetEntity().GeometryType, index);
    vtkMedEntityArray* subarray=ugrid->GetEntityArray(subentity);

    if(subarray==NULL)
      {
      vtkErrorMacro("DESC connectivity used, but sub types do not exist in file.");
      return;
      }
    subarray->LoadConnectivity();
    }
}

vtkDataSet* vtkMedReader::CreateUnstructuredGridForPointSupport(
    vtkMedFamilyOnEntityOnProfile* foep)
{
  vtkUnstructuredGrid* vtkgrid = vtkUnstructuredGrid::New();
  foep->ComputeIntersection(NULL);
  vtkMedFamilyOnEntity* foe = foep->GetFamilyOnEntity();
  vtkMedGrid* medgrid=foe->GetParentGrid();
  vtkMedUnstructuredGrid* medugrid=vtkMedUnstructuredGrid::SafeDownCast(
      medgrid);
  vtkMedCurvilinearGrid* medcgrid=vtkMedCurvilinearGrid::SafeDownCast(
      medgrid);

  medgrid->LoadCoordinates();

  vtkIdType npts=medgrid->GetNumberOfPoints();

  bool shallowCopy= (medugrid != NULL || medcgrid!=NULL);
  if(medgrid->GetParentMesh()->GetSpaceDimension()!=3)
    {
    shallowCopy=false;
    }
  else
    {
    shallowCopy = foep->CanShallowCopyPointField(NULL);
    }

  vtkDataArray* coords = NULL;

  if(medugrid != NULL)
    coords = medugrid->GetCoordinates();
  if(medcgrid != NULL)
    coords = medcgrid->GetCoordinates();


  vtkIdType numberOfPoints;
  vtkPoints* points=vtkPoints::New(coords->GetDataType());
  vtkgrid->SetPoints(points);
  points->Delete();

  vtkIdTypeArray* pointGlobalIds=vtkIdTypeArray::New();
  pointGlobalIds->SetName("MED_POINT_ID");
  pointGlobalIds->SetNumberOfComponents(1);
  vtkgrid->GetPointData()->SetGlobalIds(pointGlobalIds);
  pointGlobalIds->Delete();

  if (shallowCopy)
    {
    vtkgrid->GetPoints()->SetData(coords);
    numberOfPoints=npts;

    pointGlobalIds->SetNumberOfTuples(numberOfPoints);
    vtkIdType* ptr=pointGlobalIds->GetPointer(0);
    for(int pid=0; pid<numberOfPoints; pid++)
      ptr[pid]=pid+1;
    }
  if(!shallowCopy)
    {
    vtkIdType currentIndex=0;

    for(vtkIdType index=0; index<medgrid->GetNumberOfPoints(); index++)
      {
      if (!foep->KeepPoint(index))
        {
        continue;
        }

      double coord[3]={0.0, 0.0, 0.0};
      double * tuple=medgrid->GetCoordTuple(index);
      for(int dim=0; dim<medgrid->GetParentMesh()->GetSpaceDimension()&&dim<3; dim++)
        {
        coord[dim]=tuple[dim];
        }
      vtkgrid->GetPoints()->InsertPoint(currentIndex, coord);
      pointGlobalIds->InsertNextValue(index+1);
      currentIndex++;
      }
    vtkgrid->GetPoints()->Squeeze();
    pointGlobalIds->Squeeze();
    numberOfPoints=currentIndex;
    }

  // now create the VTK_VERTEX cells
  for(vtkIdType id=0; id<numberOfPoints; id++)
    {
    vtkgrid->InsertNextCell(VTK_VERTEX, 1, &id);
    }
  vtkgrid->Squeeze();

  // in this particular case, the global ids of the cells is the same as the global ids of the points.
  vtkgrid->GetCellData()->SetGlobalIds(vtkgrid->GetPointData()->GetGlobalIds());

  return vtkgrid;
}

vtkMedGrid* vtkMedReader::FindGridStep(vtkMedMesh* mesh)
{
  if(this->Internal->RealAnimationMode == vtkMedReader::PhysicalTime)
    {
    vtkMedComputeStep cs;
    cs.TimeOrFrequency = this->Internal->UpdateTimeStep;
    return mesh->FindGridStep(cs, vtkMedReader::PhysicalTime);
    }
  else if(this->Internal->RealAnimationMode == vtkMedReader::Modes)
    {
    vtkMedComputeStep cs;
    cs.IterationIt = MED_NO_IT;
    cs.TimeIt = MED_NO_DT;
    cs.TimeOrFrequency = MED_NO_DT;
    return mesh->FindGridStep(cs, vtkMedReader::Modes);
    }
  else // Iterations
    {
    vtkMedComputeStep cs;
    // the time is set by choosing its index in the global
    // array giving the available times : this->TimeIndexForIterations
    cs.TimeOrFrequency = (med_int)this->AvailableTimes->GetValue(
        (vtkIdType)this->TimeIndexForIterations);
    // the iteration is asked by the pipeline
    cs.IterationIt = (med_int)this->Internal->UpdateTimeStep;
    return mesh->FindGridStep(cs, vtkMedReader::Iteration);
    }
  return NULL;
}

void vtkMedReader::CreateMedSupports()
{
  this->Internal->UsedSupports.clear();

  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      meshfileit = this->Internal->MedFiles.begin();
  while(meshfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = meshfileit->second;
    meshfileit++;

    for(int meshIndex=0; meshIndex<file->GetNumberOfMesh(); meshIndex++)
      {
      vtkMedMesh* mesh = file->GetMesh(meshIndex);
      vtkMedGrid* grid = this->FindGridStep(mesh);
      if(grid == NULL)
        continue;

      for(int entityIndex=0; entityIndex<grid->GetNumberOfEntityArray();
        entityIndex++)
        {
        vtkMedEntityArray* array=grid->GetEntityArray(entityIndex);
        if(this->GetEntityStatus(array->GetEntity())==0)
          {
          continue;
          }

        file->GetMedDriver()->LoadFamilyIds(array);
        for(int foeIndex=0; foeIndex<array->GetNumberOfFamilyOnEntity();
          foeIndex++)
          {
          vtkMedFamilyOnEntity* foe=array->GetFamilyOnEntity(foeIndex);
          vtkMedFamily* family=foe->GetFamily();
          if(this->GetFamilyStatus(mesh, family)==0)
            continue;

          // now, I look over all non-point fields to see which profiles
          // have to be used on points.
          bool selectedSupport = false;

          std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
              fieldfileit = this->Internal->MedFiles.begin();
          while(fieldfileit != this->Internal->MedFiles.end())
            {
            vtkMedFile* fieldfile = fieldfileit->second;
            fieldfileit++;

            for(int fieldId=0; fieldId<fieldfile->GetNumberOfField(); fieldId++)
              {
              vtkMedField* field=fieldfile->GetField(fieldId);

              if (!this->IsFieldSelected(field))
                continue;

              vtkMedListOfFieldSteps steps;

              this->GatherFieldSteps(field, steps);

              vtkMedListOfFieldSteps::iterator it=steps.begin();
              while(it!=steps.end())
                {
                vtkMedFieldStep *step = *it;
                step->LoadInformation();
                it++;

                for(int eid=0; eid<step->GetNumberOfFieldOverEntity(); eid++)
                  {
                  vtkMedFieldOverEntity* fieldOverEntity =
                      step->GetFieldOverEntity(eid);

                  for(int pid = 0; pid < fieldOverEntity->GetNumberOfFieldOnProfile(); pid++)
                    {
                    vtkMedFieldOnProfile* fop =
                        fieldOverEntity->GetFieldOnProfile(pid);
                    vtkMedProfile* prof = fop->GetProfile();
                    vtkMedFamilyOnEntityOnProfile* foep =
                        foe->GetFamilyOnEntityOnProfile(prof);
                    if(foep != NULL)
                      {
                      this->Internal->UsedSupports.insert(foep);
                      selectedSupport = true;
                      }
                    }
                  }
                }
              }
            }
          // If no field use this family on entity, I nevertheless create the
          // support, with an empty profile.
          if(!selectedSupport)
            {
            vtkMedFamilyOnEntityOnProfile* foep =
                foe->GetFamilyOnEntityOnProfile((vtkMedProfile*)NULL);
            if(foep == NULL)
              {
              foep = vtkMedFamilyOnEntityOnProfile::New();
              foep->SetFamilyOnEntity(foe);
              foep->SetProfile(NULL);
              foe->AddFamilyOnEntityOnProfile(foep);
              foep->Delete();
              }
            this->Internal->UsedSupports.insert(foep);
            }
          }
        }
      }
  }
}

bool vtkMedReader::BuildVTKSupport(
    vtkMedFamilyOnEntityOnProfile* foep,
    int doBuildSupport)
{

  vtkMedFamilyOnEntity* foe = foep->GetFamilyOnEntity();

  int numProc = 1;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller != NULL)
    {
    numProc = controller->GetNumberOfProcesses();
    }

  if ((foep->GetValid() == 0) && numProc == 1)
    {
    return false;
    }

  vtkMedGrid* grid=foe->GetParentGrid();

  vtkMedEntityArray* array=foe->GetEntityArray();
  vtkMedMesh* mesh=grid->GetParentMesh();
  vtkSmartPointer<vtkStringArray> path = vtkSmartPointer<vtkStringArray>::New();
  string meshName=vtkMedUtilities::SimplifyName(mesh->GetName());
  path->InsertNextValue(meshName);
  std::string finalName;

  if(foe->GetPointOrCell()==vtkMedUtilities::OnPoint)
    {
    path->InsertNextValue(vtkMedUtilities::OnPointName);
    finalName=vtkMedUtilities::SimplifyName(foe->GetFamily()->GetName());
    }
  else
    {
    path->InsertNextValue(vtkMedUtilities::OnCellName);
    path->InsertNextValue(vtkMedUtilities::SimplifyName(foe->GetFamily()->GetName()));
    finalName=vtkMedUtilities::EntityKey(array->GetEntity());
    }

  if(foep->GetProfile() != NULL)
    {
    path->InsertNextValue(finalName);
    finalName = foep->GetProfile()->GetName();
    }

  ostringstream progressBarTxt;
  for(int depth=0; depth<path->GetNumberOfValues(); depth++)
    {
    progressBarTxt<<path->GetValue(depth)<<" ";
    }
  progressBarTxt<<finalName;
  SetProgressText(progressBarTxt.str().c_str());

  vtkDataSet* cachedDataSet = NULL;
  if(this->Internal->DataSetCache.find(foep)!=this->Internal->DataSetCache.end())
    {
    cachedDataSet = this->Internal->DataSetCache[foep];
    }
  else
    {
    vtkDataSet* dataset = NULL;
    if(doBuildSupport)
      {
      if(foe->GetPointOrCell()==vtkMedUtilities::OnPoint)
        {
        dataset = this->CreateUnstructuredGridForPointSupport(foep);
        }
      else
        {
        dataset = foep->GetFamilyOnEntity()->GetParentGrid()->
                  CreateVTKDataSet(foep);
        }
      }

    if(dataset == NULL)
      {
      return false;
      }

    this->Internal->DataSetCache[foep]=dataset;
    cachedDataSet = dataset;
    if(dataset != NULL)
      dataset->Delete();
  }

  vtkMultiBlockDataSet* root=vtkMedUtilities::GetParent(this->GetOutput(), path);
  int nb=root->GetNumberOfBlocks();

  if(cachedDataSet != NULL)
    {
    vtkDataSet* realDataSet=cachedDataSet->NewInstance();
    root->SetBlock(nb, realDataSet);
    realDataSet->Delete();

    root->GetMetaData(nb)->Set(vtkCompositeDataSet::NAME(), finalName.c_str());
    realDataSet->ShallowCopy(cachedDataSet);

    this->Internal->DataSetCache[foep]=cachedDataSet;
    this->Internal->CurrentDataSet[foep]=realDataSet;

    path->InsertNextValue(finalName);
    path->SetName("BLOCK_NAME");
    realDataSet->GetFieldData()->AddArray(path);
    realDataSet->GetInformation()->Remove(vtkMedUtilities::BLOCK_NAME());
    for(int depth=0; depth<path->GetNumberOfValues(); depth++)
      {
      realDataSet->GetInformation()->Set(vtkMedUtilities::BLOCK_NAME(),
                                         path->GetValue(depth), depth);
      }
    }

  return true;
}

void vtkMedReader::MapFieldOnSupport(vtkMedFieldOnProfile* fop,
                                     vtkMedFamilyOnEntityOnProfile* foep,
                                     int doCreateField)
{
  bool cached = false;

  if(this->Internal->FieldCache.find(foep)
      !=this->Internal->FieldCache.end())
    {
    map<vtkMedFieldOnProfile*, VTKField>& fieldCache =
        this->Internal->FieldCache[foep];
    if(fieldCache.find(fop)!=fieldCache.end())
      {
      cached=true;
      }
    }

  if(!cached)
    {
    this->CreateVTKFieldOnSupport(fop, foep, doCreateField);
    }
  this->SetVTKFieldOnSupport(fop, foep);
}

void vtkMedReader::MapFieldsOnSupport(vtkMedFamilyOnEntityOnProfile* foep,
                                      int doCreateField)
{
  // now loop over all fields to map it to the created grids
  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fieldfileit = this->Internal->MedFiles.begin();
  while(fieldfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = fieldfileit->second;
    fieldfileit++;

    for(int fieldId=0; fieldId<file->GetNumberOfField(); fieldId++)
      {
      vtkMedField* field=file->GetField(fieldId);

#ifdef DBGOUT
      std::cout<<"field: name="<<field->GetName()<<" ncmpt="<<field->GetNumberOfComponent()<<std::endl;
#endif
    
      if(strcmp(foep->GetFamilyOnEntity()->
                GetParentGrid()->GetParentMesh()->GetName(),
                field->GetMeshName()) != 0)
        continue;

      if(strcmp(foep->GetFamilyOnEntity()->
                GetParentGrid()->GetParentMesh()->GetName(),
                field->GetMeshName()) != 0)
        continue;

      if (!this->IsFieldSelected(field))
        continue;
      
      vtkMedListOfFieldSteps steps;

      this->GatherFieldSteps(field, steps);
      
      vtkMedListOfFieldSteps::iterator it=steps.begin();
      while(it!=steps.end())
        {
        vtkMedFieldStep *step = *it;
        step->LoadInformation();
        it++;
      
        for(int eid=0; eid<step->GetNumberOfFieldOverEntity(); eid++)
          {
          vtkMedFieldOverEntity* fieldOverEntity = step->GetFieldOverEntity(eid);
          for(int pid = 0; pid < fieldOverEntity->GetNumberOfFieldOnProfile(); pid++)
            {
            vtkMedFieldOnProfile* fop = fieldOverEntity->GetFieldOnProfile(pid);
            if(foep->CanMapField(fop))
              {
              this->MapFieldOnSupport(fop, foep, doCreateField);
              }
            }
          }
        }
      }
    }
}

void vtkMedReader::GatherFieldSteps(vtkMedField* field,
                                    vtkMedListOfFieldSteps& steps)
{
  if(this->Internal->RealAnimationMode == vtkMedReader::PhysicalTime)
    {
    vtkMedComputeStep cs;
    cs.TimeOrFrequency = this->Internal->UpdateTimeStep;
    vtkMedFieldStep* fs =
        field->FindFieldStep(cs, vtkMedReader::PhysicalTime);
    if(fs != NULL)
      steps.push_back(fs);
    }
  else if(this->Internal->RealAnimationMode == vtkMedReader::Iteration)
    {
    vtkMedComputeStep cs;
    cs.IterationIt = (med_int)this->Internal->UpdateTimeStep;
    cs.TimeOrFrequency = (med_int)this->AvailableTimes->GetValue(
        (vtkIdType)this->TimeIndexForIterations);
    vtkMedFieldStep* fs =
        field->FindFieldStep(cs, vtkMedReader::Iteration);
    if(fs != NULL)
      steps.push_back(fs);
    }
  else // modes
    {
    for(int modeid = 0; modeid < this->Frequencies->GetNumberOfKey(); modeid++)
      {
      if(this->Frequencies->GetKeyStatus(
          this->Frequencies->GetKey(modeid)) == 0)
        {
        continue;
        }

      vtkMedComputeStep cs;
      cs.TimeOrFrequency = this->AvailableTimes->GetValue(modeid);
      vtkMedFieldStep* fs =
          field->FindFieldStep(cs, vtkMedReader::PhysicalTime);
      if(fs != NULL)
        steps.push_back(fs);
      }
    }
}

void vtkMedReader::SetVTKFieldOnSupport(vtkMedFieldOnProfile* fop,
    vtkMedFamilyOnEntityOnProfile* foep)
{
  vtkMedFamilyOnEntity* foe = foep->GetFamilyOnEntity();
  vtkMedFieldOverEntity* fieldOverEntity = fop->GetParentFieldOverEntity();
  vtkMedFieldStep* step = fieldOverEntity->GetParentStep();
  vtkMedField* field = step->GetParentField();
  
  const vtkMedComputeStep& cs = step->GetComputeStep();

  vtkDataSet* ds=this->Internal->CurrentDataSet[foep];
  if (ds == NULL)
    {
  // ds == NULL could arrive is some cases when working in parallel
  vtkWarningMacro( "--- vtkMedReader::SetVTKFieldOnSupport: ds is NULL !!");
  return;
    }

  VTKField& vtkfield=this->Internal->FieldCache[foep][fop];

  std::string name=vtkMedUtilities::SimplifyName(field->GetName());
  std::string vectorname = name+"_Vector";
  if(this->AnimationMode==Modes)
    {
    double freq = cs.TimeOrFrequency;
    int index = this->GetFrequencyIndex(freq);
    name += string(" ") + vtkMedUtilities::GetModeKey(index, freq,
      this->AvailableTimes->GetNumberOfTuples()-1);
    vectorname += string(" ") + vtkMedUtilities::GetModeKey(index, freq,
      this->AvailableTimes->GetNumberOfTuples()-1);
    }

  vtkfield.DataArray->SetName(name.c_str());
  vtkfield.DataArray->Squeeze();
  if(vtkfield.Vectors != NULL)
    {
    vtkfield.Vectors->SetName(vectorname.c_str());
    vtkfield.Vectors->Squeeze();
    }
  if(vtkfield.QuadratureIndexArray!=NULL)
    {
    vtkfield.QuadratureIndexArray->Squeeze();
    }

  if(foe->GetPointOrCell()==vtkMedUtilities::OnCell)
    {
    if(field->GetFieldType()==vtkMedField::PointField)
      {
      if(vtkfield.DataArray->GetNumberOfTuples()!=ds->GetNumberOfPoints())
        {
    	  vtkDebugMacro("the data array " << vtkfield.DataArray->GetName()
                      << " do not have the good number of tuples");
        return;
        }
      ds->GetPointData()->AddArray(vtkfield.DataArray);
      if(vtkfield.Vectors != NULL && this->GenerateVectors)
        {
        ds->GetPointData()->AddArray(vtkfield.Vectors);
        ds->GetPointData()->SetActiveVectors(vtkfield.Vectors->GetName());
        }
      switch (vtkfield.DataArray->GetNumberOfComponents())
        {
        case 1:
        ds->GetPointData()->SetActiveScalars(vtkfield.DataArray->GetName());
        break;
        case 3:
        ds->GetPointData()->SetActiveVectors(vtkfield.DataArray->GetName());
        break;
        }
      // if the data set is only composed of VTK_VERTEX cells,
      // and no field called with the same name exist on cells,
      // map this field to cells too
      if(foe->GetVertexOnly()==1 && ds->GetCellData()->GetArray(
              vtkfield.DataArray->GetName())==NULL)
        {
        ds->GetCellData()->AddArray(vtkfield.DataArray);
        if(vtkfield.Vectors != NULL && this->GenerateVectors)
          {
          ds->GetCellData()->AddArray(vtkfield.Vectors);
          ds->GetCellData()->SetActiveVectors(vtkfield.Vectors->GetName());
          }
        switch (vtkfield.DataArray->GetNumberOfComponents())
          {
          case 1:
          ds->GetCellData()->SetActiveScalars(vtkfield.DataArray->GetName());
          break;
          case 3:
          ds->GetCellData()->SetActiveVectors(vtkfield.DataArray->GetName());
          break;
          }
        }
      }
    if(field->GetFieldType()==vtkMedField::CellField)
      {
      if((this->Internal->NumberOfPieces == 1) && vtkfield.DataArray->GetNumberOfTuples()!=ds->GetNumberOfCells()  )
        {
        vtkDebugMacro("the data array " << vtkfield.DataArray->GetName()
                      << " do not have the good number of tuples"
                      << " ncell=" << ds->GetNumberOfCells()
                      << " ntuple=" << vtkfield.DataArray->GetNumberOfTuples());
        return;
        }
      // In case we are in parallel and our process does not contain the data
      if(ds->GetNumberOfCells()==0)
        {
        return;
        }
      ds->GetCellData()->AddArray(vtkfield.DataArray);

      if(vtkfield.Vectors != NULL && this->GenerateVectors)
        {
        ds->GetCellData()->AddArray(vtkfield.Vectors);
        ds->GetCellData()->SetActiveVectors(vtkfield.Vectors->GetName());
        }
      switch (vtkfield.DataArray->GetNumberOfComponents())
        {
        case 1:
        ds->GetCellData()->SetActiveScalars(vtkfield.DataArray->GetName());
        break;
        case 3:
        ds->GetCellData()->SetActiveVectors(vtkfield.DataArray->GetName());
        break;
        }
      // if the data set is only composed of VTK_VERTEX cells,
      // and no field called with the same name exist on points,
      // map this field to points too
      if(foe->GetVertexOnly()==1 && ds->GetPointData()->GetArray(
              vtkfield.DataArray->GetName())==NULL)
        {
        ds->GetPointData()->AddArray(vtkfield.DataArray);
        if(vtkfield.Vectors != NULL && this->GenerateVectors)
          {
          ds->GetPointData()->AddArray(vtkfield.Vectors);
          ds->GetPointData()->SetActiveVectors(vtkfield.Vectors->GetName());
          }
        switch (vtkfield.DataArray->GetNumberOfComponents())
          {
          case 1:
          ds->GetPointData()->SetActiveScalars(vtkfield.DataArray->GetName());
          break;
          case 3:
          ds->GetPointData()->SetActiveVectors(vtkfield.DataArray->GetName());
          break;
          }
        }
      }
    if(field->GetFieldType()==vtkMedField::QuadratureField ||
       field->GetFieldType()==vtkMedField::ElnoField )
      {
      vtkIdType ncells=ds->GetNumberOfCells();
      vtkIdType nid=vtkfield.QuadratureIndexArray->GetNumberOfTuples();
      vtkIdType nda=vtkfield.DataArray->GetNumberOfTuples();
      if((nid!=ncells) && (this->Internal->NumberOfPieces == 1))
        {
        vtkDebugMacro(
            "There should be as many quadrature index values as there are cells");
        return;
        }
      else
        {
      if (ncells == 0)
      {
        vtkfield.DataArray->SetNumberOfTuples( 0 );
        vtkfield.DataArray->Squeeze();
      }
      if (nid>ncells)  // PROBABLY NOT NECESSARY
      {
        vtkfield.QuadratureIndexArray->SetNumberOfTuples(ncells);
        int nquad=fop->GetNumberOfIntegrationPoint();
        vtkfield.DataArray->SetNumberOfTuples( nquad * ds->GetNumberOfCells() );
        vtkfield.DataArray->Squeeze();
      }
        ds->GetFieldData()->AddArray(vtkfield.DataArray);
        ds->GetCellData()->AddArray(vtkfield.QuadratureIndexArray);

        nid=vtkfield.QuadratureIndexArray->GetNumberOfTuples();
        nda=vtkfield.DataArray->GetNumberOfTuples();

        if(vtkfield.Vectors != NULL && this->GenerateVectors)
          {
          ds->GetFieldData()->AddArray(vtkfield.Vectors);
          }
        }
      }
    }//support OnCell
  else
    {//support OnPoint
    if(vtkfield.DataArray->GetNumberOfTuples()!=ds->GetNumberOfPoints())
      {
      vtkDebugMacro("the data array " << vtkfield.DataArray->GetName() << " do not have the good number of tuples");
      return;
      }
    ds->GetPointData()->AddArray(vtkfield.DataArray);
    if(vtkfield.Vectors != NULL && this->GenerateVectors)
      {
      ds->GetPointData()->AddArray(vtkfield.Vectors);
      ds->GetPointData()->SetActiveVectors(vtkfield.Vectors->GetName());
      }
    switch (vtkfield.DataArray->GetNumberOfComponents())
      {
      case 1:
      ds->GetPointData()->SetActiveScalars(vtkfield.DataArray->GetName());
      break;
      case 3:
      ds->GetPointData()->SetActiveVectors(vtkfield.DataArray->GetName());
      break;
      }
    // all the VTK_VERTEX created cells have the same order than the points,
    // I can safely map the point array to the cells in this particular case.
    ds->GetCellData()->AddArray(vtkfield.DataArray);
    if(vtkfield.Vectors != NULL && this->GenerateVectors)
      {
      ds->GetCellData()->AddArray(vtkfield.Vectors);
      ds->GetCellData()->SetActiveVectors(vtkfield.Vectors->GetName());
      }
    switch (vtkfield.DataArray->GetNumberOfComponents())
      {
      case 1:
      ds->GetCellData()->SetActiveScalars(vtkfield.DataArray->GetName());
      break;
      case 3:
      ds->GetCellData()->SetActiveVectors(vtkfield.DataArray->GetName());
      break;
      }
    }
}

void vtkMedReader::InitializeQuadratureOffsets(vtkMedFieldOnProfile* fop,
    vtkMedFamilyOnEntityOnProfile* foep)
{
  vtkMedFamilyOnEntity* foe = foep->GetFamilyOnEntity();

  vtkMedFieldOverEntity* fieldOverEntity = fop->GetParentFieldOverEntity();
  vtkMedFieldStep* step = fieldOverEntity->GetParentStep();
  vtkMedField *field= step->GetParentField();

  // then I compute the quadrature key if needed, and try to find the quadrature offsets.
  if(this->Internal->QuadratureOffsetCache.find(foep)
      ==this->Internal->QuadratureOffsetCache.end())
    this->Internal->QuadratureOffsetCache[foep]=map<LocalizationKey,
        vtkSmartPointer<vtkIdTypeArray> > ();

  map<LocalizationKey, vtkSmartPointer<vtkIdTypeArray> >& quadOffsets=
      this->Internal->QuadratureOffsetCache[foep];

  LocalizationKey quadkey=this->GetLocalizationKey(fop);

  if(quadOffsets.find(quadkey)!=quadOffsets.end())
    {// the quadrature offset array has already been created
    return;
    }

  vtkIdTypeArray* qoffsets=vtkIdTypeArray::New();
  quadOffsets[quadkey]=qoffsets;
  qoffsets->Delete();

  ostringstream sstr;
  if(field->GetFieldType() == vtkMedField::ElnoField)
    {
    qoffsets->GetInformation()->Set(vtkMedUtilities::ELNO(), 1);
    sstr<<"ELNO";
    }
  else if(field->GetFieldType() == vtkMedField::QuadratureField)
    {
    qoffsets->GetInformation()->Set(vtkMedUtilities::ELGA(), 1);
    sstr<<"ELGA";
    }
  else
    {
    sstr<<"QuadraturePointOffset";
    }

  qoffsets->SetName(sstr.str().c_str());

  vtkSmartPointer<vtkMedLocalization> loc=
      this->GetLocalization(fop->GetLocalizationName());

  if(loc == NULL)
    {
    if(fop->GetNumberOfIntegrationPoint()==1)
      {// cell-centered fields can be seen as quadrature fields with 1
      // quadrature point at the center
      vtkMedLocalization* center=vtkMedLocalization::New();
      loc=center;
      center->Delete();
      center->BuildCenter(fieldOverEntity->GetEntity().GeometryType);
      }
    else if(loc == NULL && field->GetFieldType() == vtkMedField::ElnoField)
      {// ELNO fields have no vtkMedLocalization,
      // I need to create a dummy one
      vtkMedLocalization* elnodef=vtkMedLocalization::New();
      loc=elnodef;
      elnodef->Delete();
      elnodef->BuildELNO(fieldOverEntity->GetEntity().GeometryType);
      }
    else
      {
      vtkErrorMacro("Cannot find localization of quadrature field "
                    << field->GetName());
      }
    }
  this->AddQuadratureSchemeDefinition(qoffsets->GetInformation(), loc);
}

void vtkMedReader::CreateVTKFieldOnSupport(vtkMedFieldOnProfile* fop,
    vtkMedFamilyOnEntityOnProfile* foep, int doCreateField)
{
  vtkMedFamilyOnEntity* foe = foep->GetFamilyOnEntity();
  vtkMedFieldOverEntity* fieldOverEntity = fop->GetParentFieldOverEntity();
  vtkMedFieldStep* step = fieldOverEntity->GetParentStep();
  vtkMedField* field = step->GetParentField();

  if(vtkMedUnstructuredGrid::SafeDownCast(
      foep->GetFamilyOnEntity()->GetParentGrid()) != NULL)
    {
    fop->Load(MED_COMPACT_STMODE);
    }
  else
    {
    fop->Load(MED_GLOBAL_STMODE);
    }

  vtkMedIntArray* profids=NULL;

  vtkMedProfile* profile=fop->GetProfile();

  if(profile)
    {
    profile->Load();
    profids=profile->GetIds();
    }//has profile

  VTKField& vtkfield=this->Internal->FieldCache[foep][fop];

  bool shallowok = true;

  // for structured grid, the values are loaded globally, and cells which are
  // not on the profile or not on the family are blanked out.
  // shallow copy is therefore always possible
  if(vtkMedUnstructuredGrid::SafeDownCast(
      foep->GetFamilyOnEntity()->GetParentGrid()) != NULL)
    {
    shallowok = foep->CanShallowCopy(fop);
    }

  if(shallowok)
    {
    vtkfield.DataArray = fop->GetData();
    }
  else
    {
    vtkDataArray* data=vtkMedUtilities::NewArray(field->GetDataType());
    vtkfield.DataArray=data;
    data->Delete();
    vtkfield.DataArray->SetNumberOfComponents(field->GetNumberOfComponent());
    }

  for(int comp=0; comp<field->GetNumberOfComponent(); comp++)
    {
    vtkfield.DataArray->SetComponentName(comp, vtkMedUtilities::SimplifyName(
        field->GetComponentName()->GetValue(comp)).c_str());
    }

  bool createOffsets=false;
  if(field->GetFieldType()==vtkMedField::QuadratureField ||
     field->GetFieldType()==vtkMedField::ElnoField )
    {
    this->InitializeQuadratureOffsets(fop, foep);

    LocalizationKey quadKey = this->GetLocalizationKey(fop);
    vtkfield.QuadratureIndexArray
        =this->Internal->QuadratureOffsetCache[foep][quadKey];
    vtkDataSet* ds = this->Internal->CurrentDataSet[foep];

    vtkfield.DataArray->GetInformation()->Set(
        vtkQuadratureSchemeDefinition::QUADRATURE_OFFSET_ARRAY_NAME(),
        vtkfield.QuadratureIndexArray->GetName());
    vtkfield.QuadratureIndexArray->GetInformation()->Set(
        vtkAbstractArray::GUI_HIDE(), 1);

    if(vtkfield.QuadratureIndexArray->GetNumberOfTuples()
        !=ds->GetNumberOfCells())
      {
      vtkfield.QuadratureIndexArray->SetNumberOfTuples(0);
      createOffsets=true;
      }
    }

  if(!doCreateField)
    return;

  if(shallowok)
    {
    // build the quadrature offset array if needed
    if(createOffsets)
      {
      vtkIdType noffsets;
      int nquad=fop->GetNumberOfIntegrationPoint();
      noffsets=fop->GetData()->GetNumberOfTuples()/nquad;

      vtkDataSet* ds=this->Internal->CurrentDataSet[foep];
      if(noffsets!=ds->GetNumberOfCells())
        {
        vtkErrorMacro(
            "number of quadrature offsets (" << noffsets << ") must "
            << "match the number of cells (" << ds->GetNumberOfCells() << ")!");
        }

      vtkfield.QuadratureIndexArray->SetNumberOfTuples(noffsets);
      for(vtkIdType id=0; id<noffsets; id++)
        {
        vtkfield.QuadratureIndexArray->SetValue(id, nquad*id);
        }
      }

    }
  else if(foe->GetPointOrCell() == vtkMedUtilities::OnCell
     && field->GetFieldType() != vtkMedField::PointField)
    {
    // Cell-centered field on cell support
    int nquad = 1;
    if (field->GetFieldType()==vtkMedField::QuadratureField ||
        field->GetFieldType()==vtkMedField::ElnoField)
      {
      nquad=fop->GetNumberOfIntegrationPoint();
      }
    vtkMedIntArray* profids=NULL;
    if (profile)
      {
      profids=profile->GetIds();
      }
    vtkIdType maxIndex=fop->GetData()->GetNumberOfTuples();
    maxIndex/=nquad;
    vtkIdType quadIndex = 0;

    for (vtkIdType id = 0; id<maxIndex; id++)
      {
      vtkIdType realIndex = (profids!=NULL ? profids->GetValue(id)-1 : id);
      if (!foep->KeepCell(realIndex))
        continue;

      if (field->GetFieldType()==vtkMedField::QuadratureField ||
          field->GetFieldType()==vtkMedField::ElnoField)
        {
        for (int q = 0; q<nquad; q++)
          {
          vtkfield.DataArray->InsertNextTuple(nquad*realIndex+q,
              fop->GetData());
          }
        if (createOffsets)
          {
          vtkfield.QuadratureIndexArray->InsertNextValue(quadIndex);
          quadIndex+=nquad;
          }
        }
      else
        {
        vtkfield.DataArray->InsertNextTuple(id, fop->GetData());
        }
      }//copy all tuples
    vtkfield.DataArray->Squeeze();
    vtkDataSet* ds = this->Internal->CurrentDataSet[foep];
    }
  else if(foe->GetPointOrCell() == vtkMedUtilities::OnCell
     && field->GetFieldType() == vtkMedField::PointField)
    {// point field on cell support
    vtkMedIntArray* profids=NULL;

    vtkMedProfile* profile=fop->GetProfile();

    if(profile)
      {
      profile->Load();

      profids=profile->GetIds();
      }//has profile

    vtkIdType maxId=fop->GetData()->GetNumberOfTuples();

    for(vtkIdType id=0; id<maxId; id++)
      {
      // if I have a profile, then I should insert the value at the position given in the profile.
      vtkIdType destIndex;
      if(profids!=NULL)
        {
        destIndex=profids->GetValue(id)-1; // -1 because med indices start at 1
        }
      else
        {
        destIndex=id;
        }

      if(!foep->KeepPoint(destIndex))
        continue;
      // if I use the med2VTKIndex, then the index to insert
      // this value is given by the map.
      destIndex = foep->GetVTKPointIndex(destIndex);
      vtkfield.DataArray->InsertTuple(destIndex, id, fop->GetData());
      }
    vtkfield.DataArray->Squeeze();
    }// point field on cell support
  else if(foe->GetPointOrCell() == vtkMedUtilities::OnPoint &&
     field->GetFieldType() == vtkMedField::PointField)
    {//support OnPoint

    vtkIdType maxId = fop->GetData()->GetNumberOfTuples();

    for(vtkIdType id=0; id<maxId; id++)
      {
      vtkIdType realIndex=id;
      if(profids!=NULL)
        {
        realIndex=profids->GetValue(id)-1; // -1 because med indices start at 1
        }

      if(!foep->KeepPoint(realIndex))
        continue;

      vtkfield.DataArray->InsertNextTuple(fop->GetData()->GetTuple(id));
      }
    vtkfield.DataArray->Squeeze();
    }// support on point

  // now generate the vector field if asked for
  if(this->GenerateVectors)
    {
    int ncomp = vtkfield.DataArray->GetNumberOfComponents();
    if(ncomp > 1 && ncomp != 3)
      {
      vtkDataArray* vectors = vtkfield.DataArray->NewInstance();
      vectors->SetNumberOfComponents(3);
      vectors->SetNumberOfTuples(vtkfield.DataArray->GetNumberOfTuples());
      vtkfield.Vectors = vectors;
      vectors->Delete();

      vectors->CopyInformation(vtkfield.DataArray->GetInformation());

      if(ncomp < 3)
        vectors->SetComponentName(2, "Z");
      else
        vectors->SetComponentName(2, vtkfield.DataArray->GetComponentName(2));

      vectors->SetComponentName(1, vtkfield.DataArray->GetComponentName(1));
      vectors->SetComponentName(0, vtkfield.DataArray->GetComponentName(0));

      int tuplesize = (ncomp > 3? ncomp: 3);
      double* tuple = new double[tuplesize];
      for(int tid=0; tid<tuplesize; tid++)
        {
        tuple[tid] = 0.0;
        }

      for(vtkIdType id=0; id < vtkfield.DataArray->GetNumberOfTuples(); id++)
        {
        vtkfield.DataArray->GetTuple(id, tuple);
        vectors->SetTuple(id, tuple);
        }
      }
    }
}

int vtkMedReader::HasMeshAnyCellSelectedFamily(vtkMedMesh* mesh)
{
  int nfam = mesh->GetNumberOfCellFamily();
  for (int famid = 0; famid<nfam; famid++)
    {
    vtkMedFamily* fam = mesh->GetFamily(famid);
    if (fam->GetPointOrCell()!=vtkMedUtilities::OnCell||!this->GetFamilyStatus(
        mesh, fam))
      continue;
    return true;
    }
  return false;
}

int vtkMedReader::HasMeshAnyPointSelectedFamily(vtkMedMesh* mesh)
{
  int nfam = mesh->GetNumberOfCellFamily();
  for (int famid = 0; famid<nfam; famid++)
    {
    vtkMedFamily* fam = mesh->GetFamily(famid);
    if (fam->GetPointOrCell()!=vtkMedUtilities::OnPoint
        ||!this->GetFamilyStatus(mesh, fam))
      continue;
    return true;
    }
  return false;
}

void vtkMedReader::BuildSIL(vtkMutableDirectedGraph* sil)
{
    if(sil==NULL)
        return;

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

    // Add a global entry to encode names for the cell types
    vtkIdType globalEntityRoot=sil->AddChild(rootId, childEdge);
    names.push_back("Entity");

    // Add the cell types subtree
    vtkIdType entityTypesRoot=sil->AddChild(rootId, childEdge);
    names.push_back("EntityTree");

    // this is the map that keep added cell types
    map<vtkMedEntity, vtkIdType> entityMap;
    map<med_entity_type, vtkIdType> typeMap;

    std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
        meshfileit = this->Internal->MedFiles.begin();
    while(meshfileit != this->Internal->MedFiles.end())
    {
        vtkMedFile* file = meshfileit->second;
        meshfileit++;

        int numMeshes=file->GetNumberOfMesh();
        for(int meshIndex=0; meshIndex<numMeshes; meshIndex++)
        {
            vtkMedMesh* mesh = file->GetMesh(meshIndex);
            vtkMedGrid* grid = this->FindGridStep(mesh);

            if(grid == NULL)
                continue;

            // add all entities
            for(int entityIndex=0; entityIndex<grid->GetNumberOfEntityArray(); entityIndex++)
            {
                vtkMedEntityArray* array=grid->GetEntityArray(entityIndex);
                vtkMedEntity entity = array->GetEntity();

                if(entityMap.find(entity)==entityMap.end())
                {
                    vtkIdType entityGlobalId=sil->AddChild(globalEntityRoot, childEdge);
                    names.push_back(vtkMedUtilities::EntityKey(entity));

                    vtkIdType typeId;
                    if(typeMap.find(entity.EntityType)==typeMap.end())
                    {
                        typeId=sil->AddChild(entityTypesRoot, childEdge);
                        names.push_back(vtkMedUtilities::EntityName(entity.EntityType));
                        typeMap[entity.EntityType]=typeId;
                    }
                    else
                    {
                        typeId=typeMap[entity.EntityType];
                    }
                    vtkIdType entityId=sil->AddChild(typeId, childEdge);
                    names.push_back(entity.GeometryName);

                    sil->AddEdge(entityId, entityGlobalId, crossEdge);

                    entityMap[entity]=entityId;
                }
            }

            vtkIdType meshGroup = sil->AddChild(groupsRoot, childEdge);
            names.push_back(vtkMedUtilities::SimplifyName(mesh->GetName()));

            // add the two OnPoint and OnCell entries, for groups and for families
            vtkIdType meshCellGroups = sil->AddChild(meshGroup, childEdge);
            names.push_back(vtkMedUtilities::OnCellName);

            vtkIdType meshPointGroups = sil->AddChild(meshGroup, childEdge);
            names.push_back(vtkMedUtilities::OnPointName);

            // this maps will keep all added groups on nodes/cells of this mesh
            map<string, vtkIdType> nodeGroupMap;
            map<string, vtkIdType> cellGroupMap;

            // add all families
            for(int famIndex=0; famIndex<mesh->GetNumberOfFamily(); famIndex++)
            {
                vtkMedFamily* family=mesh->GetFamily(famIndex);

                vtkIdType globalFamilyId = sil->AddChild(globalFamilyRoot, childEdge);
                names.push_back(vtkMedUtilities::FamilyKey(mesh->GetName(),
                                                           family->GetPointOrCell(),
                                                           family->GetName()));

                // family with Id 0 is both on cell and on points, so add it to both
                map<string, vtkIdType> & groupMap=(family->GetPointOrCell()
                                                            ==vtkMedUtilities::OnPoint? nodeGroupMap: cellGroupMap);

                vtkIdType groupRootId =
                    (family->GetPointOrCell()==vtkMedUtilities::OnPoint?
                         meshPointGroups : meshCellGroups);

                // add all the groups of this family
                for(vtkIdType groupIndex=0; groupIndex<family->GetNumberOfGroup();
                     groupIndex++)
                {
                    vtkMedGroup* group=family->GetGroup(groupIndex);

                    vtkIdType familyGroupId = sil->AddChild(globalFamilyId, childEdge);
                    names.push_back(vtkMedUtilities::FamilyKey(
                        mesh->GetName(), family->GetPointOrCell(),
                        family->GetName()));

                    vtkIdType groupGlobalId;
                    if(groupMap.find(group->GetName())==groupMap.end())
                    {
                        vtkIdType groupLocalId;
                        groupLocalId=sil->AddChild(groupRootId, childEdge);
                        names.push_back(vtkMedUtilities::SimplifyName(group->GetName()));

                        groupGlobalId=sil->AddChild(globalGroupRoot, childEdge);
                        names.push_back(vtkMedUtilities::GroupKey(
                            mesh->GetName(), family->GetPointOrCell(),
                            group->GetName()));
                        groupMap[group->GetName()]=groupGlobalId;

                        sil->AddEdge(groupLocalId, groupGlobalId, crossEdge);
                    }
                    vtkIdType groupId = groupMap[group->GetName()];
                    sil->AddEdge(familyGroupId, groupId, childEdge);

                }//groupIndex
            }//famIndex
        }//meshIndex
    }// file iterator

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

void vtkMedReader::ClearMedSupports()
{
  this->Internal->DataSetCache.clear();
  //this->Internal->Med2VTKPointIndex.clear();

  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      meshfileit = this->Internal->MedFiles.begin();
  while(meshfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = meshfileit->second;
    meshfileit++;
    int numMeshes=file->GetNumberOfMesh();
    for(int meshIndex=0; meshIndex<numMeshes; meshIndex++)
      {
      vtkMedMesh* mesh=file->GetMesh(meshIndex);
      mesh->ClearMedSupports();
      }

    int numProf = file->GetNumberOfProfile();
    for (int prof = 0; prof<numProf; prof++)
      {
      vtkMedProfile* profile = file->GetProfile(prof);
      if (profile->GetIds()!=NULL)
        profile->GetIds()->Initialize();
      }
    }
}

void vtkMedReader::ClearMedFields()
{
  this->Internal->FieldCache.clear();
  this->Internal->QuadOffsetKey.clear();
  this->Internal->QuadratureOffsetCache.clear();

  std::map<std::string, vtkSmartPointer<vtkMedFile> >::iterator
      fieldfileit = this->Internal->MedFiles.begin();
  while(fieldfileit != this->Internal->MedFiles.end())
    {
    vtkMedFile* file = fieldfileit->second;
    fieldfileit++;

    int numFields=file->GetNumberOfField();
    for(int ff=0; ff<numFields; ff++)
      {
      vtkMedField* field=file->GetField(ff);
      int nstep=field->GetNumberOfFieldStep();
      for(int sid=0; sid<nstep; sid++)
        {
        vtkMedFieldStep* step = field->GetFieldStep(sid);
        for(int id=0; id<step->GetNumberOfFieldOverEntity(); id++)
          {
          vtkMedFieldOverEntity * fieldOverEntity=step->GetFieldOverEntity(id);
          for(int pid = 0; pid < fieldOverEntity->GetNumberOfFieldOnProfile(); pid++)
            {
            vtkMedFieldOnProfile* fop = fieldOverEntity->GetFieldOnProfile(pid);
            if(fop->GetData() != NULL)
              fop->SetData(NULL);
            }
          }
        }
      }
    }
}

void vtkMedReader::ClearCaches(int when)
{
  switch(when)
  {
    case Initialize:
      this->Internal->CurrentDataSet.clear();
      this->Internal->DataSetCache.clear();
      this->Internal->FieldCache.clear();
      this->Internal->UsedSupports.clear();
      this->Internal->QuadratureOffsetCache.clear();
      this->Internal->QuadOffsetKey.clear();
      //this->Internal->Med2VTKPointIndex.clear();
      break;
    case StartRequest:
      this->Internal->CurrentDataSet.clear();
      this->Internal->UsedSupports.clear();
      if(this->CacheStrategy==CacheNothing)
        {
        this->ClearMedSupports();
        this->ClearMedFields();
        }
      else if(this->CacheStrategy==CacheGeometry)
        {
        this->ClearMedFields();
        }
      break;
    case AfterCreateMedSupports:
      // TODO : clear the unused supports and associated cached datasets and fields
      break;
    case EndBuildVTKSupports:
      break;
    case EndRequest:
      if(this->CacheStrategy==CacheNothing)
        {
        this->ClearMedSupports();
        this->ClearMedFields();
        }
      else if(this->CacheStrategy==CacheGeometry && this->AnimationMode != Modes)
        {
        this->ClearMedFields();
        }
      break;
  }
}

void vtkMedReader::PrintSelf(ostream& os, vtkIndent indent)
{
  PRINT_STRING(os, indent, FileName);
  PRINT_IVAR(os, indent, AnimationMode);
  PRINT_IVAR(os, indent, TimeIndexForIterations);
  PRINT_OBJECT(os, indent, PointFields);
  PRINT_OBJECT(os, indent, CellFields);
  PRINT_OBJECT(os, indent, QuadratureFields);
  PRINT_OBJECT(os, indent, ElnoFields);
  PRINT_OBJECT(os, indent, Groups);
  PRINT_OBJECT(os, indent, Entities);
  PRINT_IVAR(os, indent, CacheStrategy);
  this->Superclass::PrintSelf(os, indent);
}
