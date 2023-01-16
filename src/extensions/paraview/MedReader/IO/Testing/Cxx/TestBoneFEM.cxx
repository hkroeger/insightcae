
#include "boost/filesystem.hpp"
#include "vtkMedReader.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSmartPointer.h"
#include "vtkInformation.h"
#include "vtkDoubleArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkExtractBlock.h"

#include <iostream>
#include <set>
#include <vector>

#include <regex>

class MultiBlockDataSetExtractor
{
  vtkMultiBlockDataSet* mbds_;
  std::map<vtkDataObject*,int> flatIndices_;


public:
  MultiBlockDataSetExtractor(vtkMultiBlockDataSet* mbds);

  std::set<int> flatIndices(const std::vector<std::string>& groupNamePatterns) const;

  static std::set<vtkDataObject*> findObjectsBelowGroup(const std::string& name_pattern, vtkDataObject* input);
};

MultiBlockDataSetExtractor::MultiBlockDataSetExtractor(vtkMultiBlockDataSet* mbds)
  : mbds_(mbds)
{

  vtkSmartPointer<vtkDataObjectTreeIterator> iter = mbds_->NewTreeIterator();
  iter->VisitOnlyLeavesOff();
  iter->SkipEmptyNodesOff();

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto o=iter->GetCurrentDataObject();
    auto j=iter->GetCurrentFlatIndex();
    std::cout<<j<<" "<<iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME())<<std::endl;
    flatIndices_[o]=j;
  }
}




std::set<int> MultiBlockDataSetExtractor::flatIndices(const std::vector<std::string>& groupNamePatterns) const
{
  auto i = groupNamePatterns.begin();

  auto gi = findObjectsBelowGroup(*i, mbds_); // top level

  if (i!=groupNamePatterns.end())
  {
    for ( ++i; i!=groupNamePatterns.end() ; ++i) // get one level down
    {
      std::cout<<"level = "<<*i<<std::endl;
      std::set<vtkDataObject*> ngi;
      for (auto j: gi)
      {
        auto r=findObjectsBelowGroup(*i, j);
        ngi.insert(r.begin(), r.end());
      }
      gi=ngi;
    }
  }

  std::set<int> res;
  for(auto j: gi) // get all leaf objects below found groups
  {
    vtkSmartPointer<vtkDataObjectTreeIterator> iter =
        vtkMultiBlockDataSet::SafeDownCast(j)->NewTreeIterator();
    iter->VisitOnlyLeavesOn();
    std::cout<<"final index = ";
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      int fi=flatIndices_.at(iter->GetCurrentDataObject());
      std::cout<<fi<<" ";
      res.insert(fi);
    }
    std::cout<<std::endl;
  }
  return res;
}

std::set<vtkDataObject*> MultiBlockDataSetExtractor::findObjectsBelowGroup(const std::string& name_pattern, vtkDataObject* input)
{

  std::set<vtkDataObject*> res;

  vtkMultiBlockDataSet *mbds=vtkMultiBlockDataSet::SafeDownCast(input);

  std::regex pattern(name_pattern);
  vtkSmartPointer<vtkDataObjectTreeIterator> iter = mbds->NewTreeIterator();
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    std::string name(iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME()));

//    iter->Print(std::cout);
//    iter->GetCurrentMetaData()->Print(std::cout);

    auto j=iter->GetCurrentDataObject();
    if (std::regex_match(name, pattern))
    {
      std::cout<<name<<" matches "<<name_pattern<<" ("<<j<<")"<<std::endl;
      res.insert(j);
    }
    else
    {
      std::cout<<name<<" not matching "<<name_pattern<<" ("<<j<<")"<<std::endl;
    }
  }

  return res;
}

void printPointFields(std::ostream& os, vtkDataSet *ds)
{
    auto pd=ds->GetPointData();
    for (int i=0; i<pd->GetNumberOfArrays(); ++i)
    {
        os<<"ARRAY : "<<pd->GetArrayName(i)<<std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc!=2)
    {
        std::cerr<<"expected a single filename as command line argument!"<<std::endl;
        exit(1);
    }
    boost::filesystem::path medfile(argv[1]);

    auto reader = vtkSmartPointer<vtkMedReader>::New();
    reader->SetFileName(medfile.string().c_str());
//    reader->SetPointFieldArrayStatus("result_DEPL", 1);
//    reader->SetEntityStatus("CELL_TYPE/MED_CELL/HE8", 1);
//    reader->SetEntityStatus("CELL_TYPE/MED_CELL/PE6", 1);
//    reader->SetEntityStatus("CELL_TYPE/MED_CELL/PO1", 1);
//    reader->SetEntityStatus("CELL_TYPE/MED_CELL/QU4", 1);
//    reader->SetEntityStatus("CELL_TYPE/MED_CELL/QU8", 1);
//    reader->SetEntityStatus("CELL_TYPE/MED_CELL/TR3", 1);
//    reader->SetEntityStatus("CELL_TYPE/MED_CELL/TR6", 1);
//    reader->SetEntityStatus("CELL_TYPE/MED_NODE/", 1);
    reader->SetGenerateVectors(1);

    reader->UpdateInformation();
    vtkSmartPointer<vtkDoubleArray> times=reader->GetAvailableTimes();
    for (int i=0; i<times->GetNumberOfValues(); ++i)
    {
        std::cout<<"i="<<i<<", t="<<times->GetValue(i)<<std::endl;
    }
    reader->GetOutputInformation(0)->Set(
       vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), times->GetValue(times->GetNumberOfValues()-1) );
    reader->Update();

    reader->SetTimeIndexForIterations(3);
    reader->Update();
    std::cout<<"t idx = "<<reader->GetTimeIndexForIterations()<<std::endl;

//    std::cout<<"reader"<<std::endl;
//    reader->PrintSelf(std::cout, vtkIndent());
//    reader->GetOutput()->PrintSelf(std::cout, vtkIndent());


    auto cads=reader->GetOutput();
    auto voli =
        MultiBlockDataSetExtractor(
                vtkMultiBlockDataSet::SafeDownCast(cads)).flatIndices(
          {"mesh", "OnCell", "volume"}
          );
    auto vole = vtkSmartPointer<vtkExtractBlock>::New();
    vole->SetInputData(cads);
    for (auto i: voli) { vole->AddIndex(i); }

    auto vol = vtkSmartPointer<vtkCompositeDataGeometryFilter>::New();
    vol->SetInputConnection(vole->GetOutputPort());
    vol->Update();
    printPointFields(std::cout, vol->GetOutput());

    auto voldata=vol->GetOutput();
    auto sieq = voldata->GetPointData()->GetArray("result_SIEQ_NOEU");
    double range[2];
    sieq->GetRange(range, 0);
    std::cout<<"range = "<<range[0]<<" ... "<<range[1]<<std::endl;

    return 0;
}
