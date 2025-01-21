#include "vtkrendering.h"

#include <algorithm>
#include <iterator>
#include <vtkTransformPolyDataFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkTransform.h>

#include "boost/range/adaptor/indexed.hpp"
#include "vtkSmartPointer.h"
#include "vtkOpenFOAMReader.h"
#include "vtkArrowSource.h"
#include "vtkGlyphSource2D.h"
#include "vtkCutter.h"
#include "vtkPlane.h"
#include "vtkX3DExporter.h"
#include "vtkRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkDataSetMapper.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"
#include "vtkProperty.h"
#include "vtkAutoInit.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkTextProperty.h"
#include "vtkLineSource.h"
#include "vtkRendererCollection.h"
#include "vtkActor2DCollection.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkCleanPolyData.h"
#include "vtkLogLookupTable.h"
// #include "vtkLogger.h"

#include "vtkPointData.h"
#include "vtkExtractBlock.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkAlgorithmOutput.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkAxesActor.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkLightKit.h"
#include "vtkMultiProcessController.h"
#include "vtkExecutive.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformationVector.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkLabelPlacementMapper.h"

#include "vtkAppendFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkFieldData.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiProcessStream.h"
#include "vtkConnectivityFilter.h"
#include "vtkThreshold.h"
#include "vtkAlgorithm.h"
#include "vtkCellData.h"

#include "vtkDataSetSurfaceFilter.h"
#include "vtkCellSizeFilter.h"
#include "vtkPolyDataNormals.h"
#include "vtkCellCenters.h"

#include "base/exception.h"
#include "base/spatialtransformation.h"
#include "base/translations.h"

#include "base/resultset.h"
#include "base/resultelements/attributeresulttable.h"

void vtkRenderingOpenGL2_AutoInit_Construct();
void vtkRenderingFreeType_AutoInit_Construct();
void vtkInteractionStyle_AutoInit_Construct();



inline bool vtkSkipAttributeType(int attr)
{
    return (attr == vtkDataObject::POINT_THEN_CELL);
}

inline void vtkShallowCopy(vtkDataObject* output, vtkDataObject* input)
{
    vtkCompositeDataSet* cdout = vtkCompositeDataSet::SafeDownCast(output);
    if (cdout == NULL)
    {
        output->ShallowCopy(input);
        return;
    }

    // We can't use vtkCompositeDataSet::ShallowCopy() since that simply passes
    // the leaf datasets without actually shallowcopying them. That doesn't work
    // in our case since we will be modifying the datasets in the output.
    vtkCompositeDataSet* cdin = vtkCompositeDataSet::SafeDownCast(input);
    cdout->CopyStructure(cdin);
    vtkSmartPointer<vtkCompositeDataIterator> initer;
    initer.TakeReference(cdin->NewIterator());
    for (initer->InitTraversal(); !initer->IsDoneWithTraversal(); initer->GoToNextItem())
    {
        vtkDataObject* in = initer->GetCurrentDataObject();
        vtkDataObject* clone = in->NewInstance();
        clone->ShallowCopy(in);
        cdout->SetDataSet(initer, clone);
        clone->FastDelete();
    }
}

vtkStandardNewMacro(vtkCleanArrays);
//vtkCxxSetObjectMacro(vtkCleanArrays, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkCleanArrays::vtkCleanArrays()
{
    this->FillPartialArrays = false;
//    this->Controller = 0;
//    this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkCleanArrays::~vtkCleanArrays()
{
//    this->SetController(0);
}

//****************************************************************************
class vtkCleanArrays::vtkArrayData
{
public:
    std::string Name;
    int NumberOfComponents;
    int Type;
    vtkArrayData()
    {
        this->NumberOfComponents = 0;
        this->Type = 0;
    }
    vtkArrayData(const vtkArrayData& other)
    {
        this->Name = other.Name;
        this->NumberOfComponents = other.NumberOfComponents;
        this->Type = other.Type;
    }

    bool operator<(const vtkArrayData& b) const
    {
        if (this->Name != b.Name)
        {
            return this->Name < b.Name;
        }
        if (this->NumberOfComponents != b.NumberOfComponents)
        {
            return this->NumberOfComponents < b.NumberOfComponents;
        }
        return this->Type < b.Type;
    }

    void Set(vtkAbstractArray* array)
    {
        this->Name = array->GetName();
        this->NumberOfComponents = array->GetNumberOfComponents();
        this->Type = array->GetDataType();
    }

    vtkAbstractArray* NewArray(vtkIdType numTuples) const
    {
        vtkAbstractArray* array = vtkAbstractArray::CreateArray(this->Type);
        if (array)
        {
            array->SetName(this->Name.c_str());
            array->SetNumberOfComponents(this->NumberOfComponents);
            array->SetNumberOfTuples(numTuples);
            vtkDataArray* data_array = vtkDataArray::SafeDownCast(array);
            for (int cc = 0; data_array && cc < this->NumberOfComponents; cc++)
            {
                data_array->FillComponent(cc, 0.0);
            }
        }
        return array;
    }
};

class vtkCleanArrays::vtkArraySet : public std::set<vtkCleanArrays::vtkArrayData>
{
    int Valid;

public:
    vtkArraySet()
        : Valid(0)
    {
    }
    bool IsValid() const { return this->Valid != 0; }
    void MarkValid() { this->Valid = 1; }

    void Intersection(const vtkArraySet& other)
    {
        if (this->Valid && other.Valid)
        {
            vtkCleanArrays::vtkArraySet setC;
            std::set_intersection(
                this->begin(), this->end(), other.begin(), other.end(), std::inserter(setC, setC.begin()));
            setC.MarkValid();
            this->swap(setC);
        }
        else if (other.Valid)
        {
            *this = other;
        }
    }
    void Union(const vtkArraySet& other)
    {
        if (this->Valid && other.Valid)
        {
            vtkCleanArrays::vtkArraySet setC;
            std::set_union(
                this->begin(), this->end(), other.begin(), other.end(), std::inserter(setC, setC.begin()));
            setC.MarkValid();
            this->swap(setC);
        }
        else if (other.Valid)
        {
            *this = other;
        }
    }

    // Fill up \c this with arrays from \c dsa
    void Initialize(vtkFieldData* dsa)
    {
        this->Valid = true;
        int numArrays = dsa->GetNumberOfArrays();
        if (dsa->GetNumberOfTuples() == 0)
        {
            numArrays = 0;
        }
        for (int cc = 0; cc < numArrays; cc++)
        {
            vtkAbstractArray* array = dsa->GetAbstractArray(cc);
            if (array && array->GetName())
            {
                vtkCleanArrays::vtkArrayData mda;
                mda.Set(array);
                this->insert(mda);
            }
        }
    }

    // Remove arrays from \c dsa not present in \c this.
    void UpdateFieldData(vtkFieldData* dsa) const
    {
        if (this->Valid == 0)
        {
            return;
        }
        vtkArraySet myself = (*this);
        int numArrays = dsa->GetNumberOfArrays();
        for (int cc = numArrays - 1; cc >= 0; cc--)
        {
            vtkAbstractArray* array = dsa->GetAbstractArray(cc);
            if (array && array->GetName())
            {
                vtkCleanArrays::vtkArrayData mda;
                mda.Set(array);
                if (myself.find(mda) == myself.end())
                {
                    // cout << "Removing: " << array->GetName() << endl;
                    dsa->RemoveArray(array->GetName());
                }
                else
                {
                    myself.erase(mda);
                }
            }
        }
        // Now fill any missing arrays.
        for (iterator iter = myself.begin(); iter != myself.end(); ++iter)
        {
            vtkAbstractArray* array = iter->NewArray(dsa->GetNumberOfTuples());
            if (array)
            {
                dsa->AddArray(array);
                array->Delete();
            }
        }
    }

    void Save(vtkMultiProcessStream& stream)
    {
        stream.Reset();
        stream << this->Valid;
        stream << static_cast<unsigned int>(this->size());
        vtkCleanArrays::vtkArraySet::iterator iter;
        for (iter = this->begin(); iter != this->end(); ++iter)
        {
            stream << iter->Name << iter->NumberOfComponents << iter->Type;
        }
    }

    void Load(vtkMultiProcessStream& stream)
    {
        this->clear();
        unsigned int numvalues;
        stream >> this->Valid;
        stream >> numvalues;
        for (unsigned int cc = 0; cc < numvalues; cc++)
        {
            vtkCleanArrays::vtkArrayData mda;
            stream >> mda.Name >> mda.NumberOfComponents >> mda.Type;
            this->insert(mda);
        }
    }
    void Print()
    {
        vtkCleanArrays::vtkArraySet::iterator iter;
        cout << "Valid: " << this->Valid << endl;
        for (iter = this->begin(); iter != this->end(); ++iter)
        {
            cout << iter->Name << ", " << iter->NumberOfComponents << ", " << iter->Type << endl;
        }
        cout << "-----------------------------------" << endl << endl;
    }
};

//----------------------------------------------------------------------------
static void IntersectStreams(vtkMultiProcessStream& A, vtkMultiProcessStream& B)
{
    vtkCleanArrays::vtkArraySet setA;
    vtkCleanArrays::vtkArraySet setB;
    setA.Load(A);
    setB.Load(B);
    setA.Intersection(setB);
    B.Reset();
    setA.Save(B);
}

//----------------------------------------------------------------------------
static void UnionStreams(vtkMultiProcessStream& A, vtkMultiProcessStream& B)
{
    vtkCleanArrays::vtkArraySet setA;
    vtkCleanArrays::vtkArraySet setB;
    setA.Load(A);
    setB.Load(B);
    setA.Union(setB);
    B.Reset();
    setA.Save(B);
}

//----------------------------------------------------------------------------
int vtkCleanArrays::RequestData(
    vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
    vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
    vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
    vtkShallowCopy(outputDO, inputDO);
    vtkCompositeDataSet* outputCD = vtkCompositeDataSet::SafeDownCast(outputDO);

//    vtkMultiProcessController* controller = this->Controller;
//    if ((!controller || controller->GetNumberOfProcesses() <= 1) && outputCD == NULL)
//    {
//        // Nothing to do since not running in parallel or on composite datasets.
//        return 1;
//    }

    // Build the array sets for all attribute types across all blocks (if any).
    vtkCleanArrays::vtkArraySet arraySets[vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES];
    if (outputCD)
    {
        vtkSmartPointer<vtkCompositeDataIterator> iter;
        iter.TakeReference(outputCD->NewIterator());
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
            vtkDataObject* dobj = iter->GetCurrentDataObject();
            for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
            {
                if (vtkSkipAttributeType(attr))
                {
                    continue;
                }
                if (dobj->GetNumberOfElements(attr) > 0)
                {
                    vtkCleanArrays::vtkArraySet myset;
                    myset.Initialize(dobj->GetAttributesAsFieldData(attr));
                    if (this->FillPartialArrays)
                    {
                        arraySets[attr].Union(myset);
                    }
                    else
                    {
                        arraySets[attr].Intersection(myset);
                    }
                }
            }
        }
    }
    else
    {
        for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
        {
            if (vtkSkipAttributeType(attr))
            {
                continue;
            }
            if (outputDO->GetNumberOfElements(attr) > 0)
            {
                arraySets[attr].Initialize(outputDO->GetAttributesAsFieldData(attr));
            }
        }
    }

//    if (controller && controller->GetNumberOfProcesses() > 1)
//    {
//        for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
//        {
//            if (vtkSkipAttributeType(attr))
//            {
//                continue;
//            }
//            vtkMultiProcessStream mstream;
//            arraySets[attr].Save(mstream);
//            vtkMultiProcessControllerHelper::ReduceToAll(controller, mstream,
//                                                         this->FillPartialArrays ? ::UnionStreams : ::IntersectStreams, 1278392 + attr);
//            arraySets[attr].Load(mstream);
//        }
//    }

    if (outputCD)
    {
        vtkSmartPointer<vtkCompositeDataIterator> iter;
        iter.TakeReference(outputCD->NewIterator());
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
            vtkDataObject* dobj = iter->GetCurrentDataObject();
            for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
            {
                if (vtkSkipAttributeType(attr))
                {
                    continue;
                }
                arraySets[attr].UpdateFieldData(dobj->GetAttributesAsFieldData(attr));
            }
        }
    }
    else
    {
        for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
        {
            if (vtkSkipAttributeType(attr))
            {
                continue;
            }
            arraySets[attr].UpdateFieldData(outputDO->GetAttributesAsFieldData(attr));
        }
    }
    return 1;
}

//----------------------------------------------------------------------------
void vtkCleanArrays::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "FillPartialArrays: " << this->FillPartialArrays << endl;
    os << indent << "Controller: " << this->Controller << endl;
}


vtkStandardNewMacro(vtkCompositeDataToUnstructuredGridFilter);
//----------------------------------------------------------------------------
vtkCompositeDataToUnstructuredGridFilter::vtkCompositeDataToUnstructuredGridFilter()
{
    this->SubTreeCompositeIndex = 0;
    this->MergePoints = true;
    this->Tolerance = 0.;
}

//----------------------------------------------------------------------------
vtkCompositeDataToUnstructuredGridFilter::~vtkCompositeDataToUnstructuredGridFilter()
{
}

int vtkCompositeDataToUnstructuredGridFilter::RequestData(vtkInformation* vtkNotUsed(request),
                                                          vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
    vtkCompositeDataSet* cd = vtkCompositeDataSet::GetData(inputVector[0], 0);
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::GetData(inputVector[0], 0);
    vtkDataSet* ds = vtkDataSet::GetData(inputVector[0], 0);
    vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector, 0);

    if (ug)
    {
        output->ShallowCopy(ug);
        return 1;
    }

    vtkNew<vtkAppendFilter> appender;
    appender->SetMergePoints(this->MergePoints ? 1 : 0);
    if (this->MergePoints)
    {
#if (VTK_MAJOR_VERSION>8 || (VTK_MAJOR_VERSION==8 && VTK_MINOR_VERSION>=90) )
        appender->SetTolerance(this->Tolerance);
#endif
    }
    if (ds)
    {
        this->AddDataSet(ds, appender);
    }
    else if (cd)
    {
        if (this->SubTreeCompositeIndex == 0)
        {
            this->ExecuteSubTree(cd, appender);
        }
        vtkDataObjectTreeIterator* iter = vtkDataObjectTreeIterator::SafeDownCast(cd->NewIterator());
        if (!iter)
        {
            vtkErrorMacro("Composite data is not a tree");
            return 0;
        }
        iter->VisitOnlyLeavesOff();
        for (iter->InitTraversal();
             !iter->IsDoneWithTraversal() && iter->GetCurrentFlatIndex() <= this->SubTreeCompositeIndex;
             iter->GoToNextItem())
        {
            if (iter->GetCurrentFlatIndex() == this->SubTreeCompositeIndex)
            {
                vtkDataObject* curDO = iter->GetCurrentDataObject();
                vtkCompositeDataSet* curCD = vtkCompositeDataSet::SafeDownCast(curDO);
                vtkUnstructuredGrid* curUG = vtkUnstructuredGrid::SafeDownCast(curDO);
                vtkDataSet* curDS = vtkUnstructuredGrid::SafeDownCast(curDO);
                if (curUG)
                {
                    output->ShallowCopy(curUG);
                    // NOTE: Not using the appender at all.
                }
                else if (curDS && curCD->GetNumberOfPoints() > 0)
                {
                    this->AddDataSet(curDS, appender);
                }
                else if (curCD)
                {
                    this->ExecuteSubTree(curCD, appender);
                }
                break;
            }
        }
        iter->Delete();
    }

    if (appender->GetNumberOfInputConnections(0) > 0)
    {
        appender->Update();
        output->ShallowCopy(appender->GetOutput());
        // this will override field data the vtkAppendFilter passed from the first
        // block. It seems like a reasonable approach, if global field data is
        // present.
        if (ds)
        {
            output->GetFieldData()->PassData(ds->GetFieldData());
        }
        else if (cd)
        {
            output->GetFieldData()->PassData(cd->GetFieldData());
        }
    }

    this->RemovePartialArrays(output);
    return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::ExecuteSubTree(
    vtkCompositeDataSet* curCD, vtkAppendFilter* appender)
{
    vtkCompositeDataIterator* iter2 = curCD->NewIterator();
    for (iter2->InitTraversal(); !iter2->IsDoneWithTraversal(); iter2->GoToNextItem())
    {
        vtkDataSet* curDS = vtkDataSet::SafeDownCast(iter2->GetCurrentDataObject());
        if (curDS)
        {
            appender->AddInputData(curDS);
        }
    }
    iter2->Delete();
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::AddDataSet(vtkDataSet* ds, vtkAppendFilter* appender)
{
    vtkDataSet* clone = ds->NewInstance();
    clone->ShallowCopy(ds);
    appender->AddInputData(clone);
    clone->Delete();
}

//----------------------------------------------------------------------------
int vtkCompositeDataToUnstructuredGridFilter::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info)
{
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::RemovePartialArrays(vtkUnstructuredGrid* data)
{
    vtkUnstructuredGrid* clone = vtkUnstructuredGrid::New();
    clone->ShallowCopy(data);
    auto cleaner = vtkCleanArrays::New();
    cleaner->SetInputData(clone);
    cleaner->Update();
    data->ShallowCopy(cleaner->GetOutput());
    cleaner->Delete();
    clone->Delete();
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "SubTreeCompositeIndex: " << this->SubTreeCompositeIndex << endl;
    os << indent << "MergePoints: " << this->MergePoints << endl;
    os << indent << "Tolerance: " << this->Tolerance << endl;
}

using namespace std;
using namespace boost;

namespace insight
{



vtkSmartPointer<vtkPolyData> createArrows(
    std::vector<std::pair<arma::mat, arma::mat> > from_to,
    bool glyph2d
)
{
  auto af = vtkSmartPointer<vtkAppendPolyData>::New();

  for (const auto& ft: from_to)
  {
    const arma::mat& from = ft.first;
    const arma::mat& to = ft.second;

    if (glyph2d)
    {
        auto l = vtkSmartPointer<vtkLineSource>::New();
        l->SetPoint1( toArray(from) );
        l->SetPoint2( toArray(to) );
        l->Update();

        af->AddInputData(l->GetOutput());
    }
    else
    {
        double r=arma::norm(from-to, 2);

        insight::CoordinateSystem cs(from, to-from);

        arma::mat R=arma::zeros(3,3);
        R.col(0)=cs.ex;
        R.col(1)=cs.ey;
        R.col(2)=cs.ez;
        insight::SpatialTransformation st1, st2;
        st1.setScale(r);
        st2.setRotationMatrix(R);

        auto a0 = vtkSmartPointer<vtkArrowSource>::New();
        a0->SetTipRadius(0.025);
        a0->SetTipLength(0.1);
        a0->SetShaftRadius(0.0075);

        auto tf= vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        tf->SetInputConnection(a0->GetOutputPort());
        tf->SetTransform( st1.appended(st2).toVTKTransform() );
        tf->Update();

        af->AddInputData(tf->GetOutput());
    }
  }
  auto cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
  cleanFilter->SetInputConnection(af->GetOutputPort());
  cleanFilter->Update();

  return cleanFilter->GetOutput();
}




const std::vector<double> colorMapData_SD = {
    0.0,    0.0,    0.333333,    1,
    0.2003664868,    0.576471,    1,    0.988235,
    0.4172793444,    0.0,    0.870588,    0.0862745,
    0.6102942075,    0.976471,    0.94902,    0.0980392,
    0.8180138451,    1.0,    0.0,    0.0,
    1.0,    1.0,    0.0,    1.0
    };


const std::vector<double> colorMapData_NICEdge = {
    -1, 0.19120799999999999, 0.19120799999999999, 0.19120799999999999,
    -0.87451000000000001, 0.239484, 0.0054503499999999996, 0.61482099999999995,

    -0.74902000000000002,
    0.22059300000000001,
    0.061745899999999999,
    0.86354699999999995,

    -0.623529,
    0.17509,
    0.27898800000000001,
    0.97794000000000003,

    -0.49803900000000001,
    0.14352599999999999,
    0.57606900000000005,
    0.99855300000000002,

    -0.37254900000000002,
    0.16645599999999999,
    0.87188299999999996,
    0.96594000000000002,

    -0.247059,
    0.37620199999999998,
    0.99355499999999997,
    0.98183299999999996,

    -0.121569,
    0.68199600000000005,
    0.99129699999999998,
    0.99923899999999999,

    0.0039215700000000001,
    0.95417200000000002,
    0.95273399999999997,
    0.94374000000000002,

    0.129412,
    0.99973500000000004,
    0.99300999999999995,
    0.66289600000000004,

    0.25490200000000002,
    0.97939900000000002,
    0.99146599999999996,
    0.35797299999999999,

    0.38039200000000001,
    0.96877100000000005,
    0.85496700000000003,
    0.162659,

    0.50588200000000005,
    0.99924500000000005,
    0.556697,
    0.14432300000000001,

    0.63137299999999996,
    0.97395900000000002,
    0.26223000000000002,
    0.17794599999999999,

    0.75686299999999995,
    0.85235799999999995,
    0.052670700000000001,
    0.22297400000000001,

    0.88235300000000005,
    0.593889,
    0.00912724,
    0.23885500000000001,

    1,
    0.19120799999999999,
    0.19120799999999999,
    0.19120799999999999
};


const std::vector<double> colorMapData_BlueGreenOrange = {
    0.0,
    0.83137300000000003,
    0.90980399999999995,
    0.98039200000000004,

    0.012500000000000001,
    0.74902000000000002,
    0.86274499999999998,
    0.96078399999999997,

    0.025000000000000001,
    0.69411800000000001,
    0.82745100000000005,
    0.94117600000000001,

    0.050000000000000003,
    0.56862699999999999,
    0.76078400000000002,
    0.92156899999999997,

    0.074999999999999997,
    0.45097999999999999,
    0.70588200000000001,
    0.90196100000000001,

    0.10000000000000001,
    0.34509800000000002,
    0.64313699999999996,
    0.85882400000000003,

    0.125,
    0.247059,
    0.57254899999999997,
    0.819608,

    0.14999999999999999,
    0.180392,
    0.52156899999999995,
    0.78039199999999997,

    0.16,
    0.14902000000000001,
    0.49019600000000002,
    0.74902000000000002,

    0.17999999999999999,
    0.129412,
    0.44705899999999998,
    0.70980399999999999,

    0.20000000000000001,
    0.101961,
    0.42745100000000003,
    0.69019600000000003,

    0.20999999999999999,
    0.094117999999999993,
    0.403922,
    0.65882399999999997,

    0.22,
    0.090195999999999998,
    0.39215699999999998,
    0.63921600000000001,

    0.23000000000000001,
    0.082352999999999996,
    0.36862699999999998,
    0.61960800000000005,

    0.23999999999999999,
    0.070587999999999998,
    0.352941,
    0.59999999999999998,

    0.25,
    0.066667000000000004,
    0.32941199999999998,
    0.56862699999999999,

    0.26000000000000001,
    0.074510000000000007,
    0.31372499999999998,
    0.54117599999999999,

    0.27000000000000002,
    0.086275000000000004,
    0.30588199999999999,
    0.50980400000000003,

    0.28000000000000003,
    0.094117999999999993,
    0.286275,
    0.478431,

    0.28999999999999998,
    0.101961,
    0.27843099999999998,
    0.45097999999999999,

    0.29999999999999999,
    0.109804,
    0.26666699999999999,
    0.41176499999999999,

    0.31,
    0.11372500000000001,
    0.258824,
    0.38039200000000001,

    0.32000000000000001,
    0.11372500000000001,
    0.25097999999999998,
    0.34902,

    0.33000000000000002,
    0.109804,
    0.26666699999999999,
    0.32156899999999999,

    0.34000000000000002,
    0.105882,
    0.30196099999999998,
    0.26274500000000001,

    0.34999999999999998,
    0.094117999999999993,
    0.30980400000000002,
    0.24313699999999999,

    0.35999999999999999,
    0.082352999999999996,
    0.32156899999999999,
    0.22745099999999999,

    0.37,
    0.074510000000000007,
    0.34117599999999998,
    0.219608,

    0.38,
    0.070587999999999998,
    0.36078399999999999,
    0.21176500000000001,

    0.39000000000000001,
    0.066667000000000004,
    0.38039200000000001,
    0.21568599999999999,

    0.40000000000000002,
    0.062744999999999995,
    0.40000000000000002,
    0.17647099999999999,

    0.42499999999999999,
    0.074510000000000007,
    0.41960799999999998,
    0.145098,

    0.45000000000000001,
    0.086275000000000004,
    0.439216,
    0.117647,

    0.47499999999999998,
    0.121569,
    0.47058800000000001,
    0.117647,

    0.5,
    0.18431400000000001,
    0.50196099999999999,
    0.14902000000000001,

    0.52500000000000002,
    0.25490200000000002,
    0.54117599999999999,
    0.18823500000000001,

    0.55000000000000004,
    0.32549,
    0.58039200000000002,
    0.231373,

    0.57499999999999996,
    0.403922,
    0.61960800000000005,
    0.27843099999999998,

    0.59999999999999998,
    0.50196099999999999,
    0.67058799999999996,
    0.33333299999999999,

    0.63,
    0.59215700000000004,
    0.72941199999999995,
    0.40000000000000002,

    0.65000000000000002,
    0.74117599999999995,
    0.78823500000000002,
    0.49019600000000002,

    0.67000000000000004,
    0.85882400000000003,
    0.85882400000000003,
    0.60392199999999996,

    0.69999999999999996,
    0.92156899999999997,
    0.83529399999999998,
    0.58039200000000002,

    0.75,
    0.90196100000000001,
    0.72941199999999995,
    0.494118,

    0.80000000000000004,
    0.85882400000000003,
    0.584314,
    0.388235,

    0.84999999999999998,
    0.80000000000000004,
    0.439216,
    0.32156899999999999,

    0.90000000000000002,
    0.67843100000000001,
    0.298039,
    0.20392199999999999,

    0.94999999999999996,
    0.54901999999999995,
    0.168627,
    0.109804,

    0.97499999999999998,
    0.478431,
    0.082352999999999996,
    0.047058999999999997,

    1.0,
    0.45097999999999999,
    0.0078429999999999993,
    0.0
};


const std::vector<double> colorMapData_CoolToWarm = {
    0,
    0,
    0,
    0.34902,
    0.03125,
    0.039216000000000001,
    0.062744999999999995,
    0.38039200000000001,
    0.0625,
    0.062744999999999995,
    0.117647,
    0.41176499999999999,
    0.09375,
    0.090195999999999998,
    0.18431400000000001,
    0.45097999999999999,
    0.125,
    0.12548999999999999,
    0.26274500000000001,
    0.50196099999999999,
    0.15625,
    0.16078400000000001,
    0.33725500000000003,
    0.54117599999999999,
    0.1875,
    0.20000000000000001,
    0.39607799999999999,
    0.56862699999999999,
    0.21875,
    0.23921600000000001,
    0.45490199999999997,
    0.59999999999999998,
    0.25,
    0.286275,
    0.52156899999999995,
    0.65098,
    0.28125,
    0.33725500000000003,
    0.59215700000000004,
    0.70196099999999995,
    0.3125,
    0.388235,
    0.65490199999999998,
    0.74902000000000002,
    0.34375,
    0.466667,
    0.73725499999999999,
    0.819608,
    0.375,
    0.57254899999999997,
    0.819608,
    0.87843099999999996,
    0.40625,
    0.65490199999999998,
    0.86666699999999997,
    0.90980399999999995,
    0.4375,
    0.75294099999999997,
    0.91764699999999999,
    0.94117600000000001,
    0.46875,
    0.82352899999999996,
    0.95686300000000002,
    0.96862700000000002,
    0.5,
    0.98823499999999997,
    0.96078399999999997,
    0.90196100000000001,
    0.5,
    0.94117600000000001,
    0.98431400000000002,
    0.98823499999999997,
    0.52000000000000002,
    0.98823499999999997,
    0.94509799999999999,
    0.85097999999999996,
    0.54000000000000004,
    0.98039200000000004,
    0.89803900000000003,
    0.78431399999999996,
    0.5625,
    0.96862700000000002,
    0.83529399999999998,
    0.69803899999999997,
    0.59375,
    0.94901999999999997,
    0.73333300000000001,
    0.58823499999999995,
    0.625,
    0.92941200000000002,
    0.65098,
    0.50980400000000003,
    0.65625,
    0.90980399999999995,
    0.56470600000000004,
    0.43529400000000001,
    0.6875,
    0.87843099999999996,
    0.45882400000000001,
    0.352941,
    0.71875,
    0.83921599999999996,
    0.388235,
    0.286275,
    0.75,
    0.76078400000000002,
    0.29411799999999999,
    0.21176500000000001,
    0.78125,
    0.70196099999999995,
    0.21176500000000001,
    0.168627,
    0.8125,
    0.65098,
    0.156863,
    0.129412,
    0.84375,
    0.59999999999999998,
    0.094117999999999993,
    0.094117999999999993,
    0.875,
    0.54901999999999995,
    0.066667000000000004,
    0.098039000000000001,
    0.90625,
    0.50196099999999999,
    0.050979999999999998,
    0.12548999999999999,
    0.9375,
    0.45097999999999999,
    0.054901999999999999,
    0.17254900000000001,
    0.96875,
    0.40000000000000002,
    0.054901999999999999,
    0.19215699999999999,
    1,
    0.34902,
    0.070587999999999998,
    0.21176500000000001
};

const std::vector<double> colorMapData_BlackBodyRadiation = {
    0,
    0,
    0,
    0,
    0.40000000000000002,
    0.90196078431399995,
    0,
    0,
    0.80000000000000004,
    0.90196078431399995,
    0.90196078431399995,
    0,
    1,
    1,
    1,
    1
};

vtkSmartPointer<vtkLookupTable> createColorMap(
        const std::vector<double>& cb,
        int nc,
        bool logscale
        )
{
    auto gen = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();

    gen->DiscretizeOn();

    int n=cb.size()/4;
    double x0=cb[0], x1=cb[4*(n-1)];

    gen->SetNumberOfIndexedColors(n);
    for (int i=0; i<n; i++)
    {
        int j=4*i;
        gen->AddRGBPoint(
            (cb[j]-x0)/(x1-x0),
            cb[j+1], cb[j+2], cb[j+3]
            );
    }
    gen->SetNumberOfValues(nc);
    gen->SetUseLogScale(logscale);
    gen->Build();

    vtkSmartPointer<vtkLookupTable> lut;
    if (logscale)
    {
        lut = vtkSmartPointer<vtkLogLookupTable>::New();
        lut->SetScaleToLog10();
    }
    else
    {
        lut = vtkSmartPointer<vtkLookupTable>::New();
    }
    lut->SetNumberOfTableValues(nc);
    for (int i=0; i<nc; i++)
    {
        double rgb[4];
        gen->GetColor( double(i)/double(nc-1), rgb);
        lut->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
    }

    lut->Build();

    return lut;
}



MinMax calcRange(
    FieldSelection fsel,
    const std::vector<vtkDataSet*> datasets,
    const std::vector<vtkAlgorithm*> algorithms
    )
{
    insight::CurrentExceptionContext ex("determining range of field "+fsel.fieldName());

    auto fieldname = boost::fusion::get<0>(fsel);
    auto fs = boost::fusion::get<1>(fsel);
    auto cmpt = boost::fusion::get<2>(fsel);

    MinMax mm(DBL_MAX, -DBL_MAX);

    auto evaluate = [&](vtkDataSet* ds)
    {
//        ds->Print(std::cout);
        double mima[2] = {0,1};
        vtkDataArray *arr;
        switch (fs)
        {
          case OnPoint:
            arr=ds->GetPointData()->GetArray(fieldname.c_str());
            insight::assertion(
                        arr!=nullptr,
                        "could not lookup point field "+fieldname );
            break;
          case OnCell:
            arr=ds->GetCellData()->GetArray(fieldname.c_str()); //->GetRange(mima, cmpt);
            insight::assertion(
                        arr!=nullptr,
                        "could not lookup cell field "+fieldname );
            break;
        }
        arr->GetRange(mima, cmpt); // cmpt==-1 => L2 norm
        mm.first=std::min(mm.first, mima[0]);
        mm.second=std::max(mm.second, mima[1]);
    };

    for (const auto d: datasets)
    {
        evaluate(d);
    }
    for (const auto a: algorithms)
    {
        a->Update();
        if (auto * d = vtkDataSet::SafeDownCast(a->GetOutputDataObject(0)))
            evaluate(d);
        else if (auto * cd = vtkCompositeDataSet::SafeDownCast(a->GetOutputDataObject(0)))
        {
            auto i=cd->NewIterator();
            for (i->InitTraversal(); !i->IsDoneWithTraversal(); i->GoToNextItem())
                evaluate( vtkDataSet::SafeDownCast(cd->GetDataSet(i)) );
        }
//        else
//            throw insight::Exception("invalid input algorithm provided (does not return vtkDataSet)");
    }
    return mm;
}



void
MultiBlockDataSetExtractor::findObjectsBelowNode(
        std::vector<std::string> name_pattern,
        vtkDataObject* input,
        std::set<int>& res) const
{
  insight::CurrentExceptionContext ex("searching for groups with names matching \""+name_pattern.front()+"\"");

  vtkMultiBlockDataSet *mbds=vtkMultiBlockDataSet::SafeDownCast(input);
  insight::assertion(mbds!=nullptr, "valid vtkMultiBlockDataset expected!");

  boost::regex repat(name_pattern.front());

  auto iter = mbds->NewTreeIterator();
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  for ( iter->InitTraversal();
       !iter->IsDoneWithTraversal();
        iter->GoToNextItem() )
  {
    std::string name(
                iter->GetCurrentMetaData()->Get(
                    vtkCompositeDataSet::NAME() ) );

    if (boost::regex_match(name, repat))
    {
        auto sub = vtkMultiBlockDataSet::SafeDownCast(iter->GetCurrentDataObject());

        if (name_pattern.size()>1)
        {
            insight::assertion(
                        sub!=nullptr,
                        "node "+name+" (selected by pattern \""+name_pattern.front()+"\") was not a vtkMultiBlockDataSet!" );
            findObjectsBelowNode(
                        std::vector<std::string>(name_pattern.begin()+1, name_pattern.end()),
                        sub, res);
        }
        else
        {
            if (sub) // add all childs
            {
                auto k = sub->NewTreeIterator();
                k->VisitOnlyLeavesOff();
                k->TraverseSubTreeOn();
                for ( k->InitTraversal();
                     !k->IsDoneWithTraversal();
                      k->GoToNextItem() )
                {
                    res.insert(flatIndices_.at(k->GetCurrentDataObject()));
                }
            }
            else // add leaf itself
            {
                res.insert(flatIndices_.at(iter->GetCurrentDataObject()));
            }
        }
    }
  }
}



MultiBlockDataSetExtractor::MultiBlockDataSetExtractor(vtkMultiBlockDataSet* mbds)
  : mbds_(mbds)
{
  CurrentExceptionContext ex("generating flat index list of vtkMultiBlockDataSet");

  insight::assertion(mbds_!=nullptr, "a non-null pointer to the MultiBlockDataSet is expected!");

  // traverse over all leafs to get flat index
  // GetCurrentFlatIndex() only return the right value,
  // if the loop is over the entire structure
  auto iter = mbds_->NewTreeIterator();
  iter->VisitOnlyLeavesOff();
  iter->SkipEmptyNodesOff();

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto o=iter->GetCurrentDataObject();
    auto j=iter->GetCurrentFlatIndex();
    flatIndices_[o]=j;
  }
}




std::set<int> MultiBlockDataSetExtractor::flatIndices(const std::vector<std::string>& groupNamePatterns) const
{
  insight::CurrentExceptionContext ex("determining block indices for pattern ["+boost::join(groupNamePatterns, ", ")+"]");

  insight::assertion(
              groupNamePatterns.size()>0,
              "specify at least one input element" );

  std::set<int> res;
  findObjectsBelowNode(groupNamePatterns, mbds_, res);

  return res;
}



boost::mutex VTKlock;

VTKOffscreenScene::VTKOffscreenScene()
    : boost::mutex::scoped_lock(VTKlock)
{

  vtkRenderingOpenGL2_AutoInit_Construct();
  vtkRenderingFreeType_AutoInit_Construct();
  vtkInteractionStyle_AutoInit_Construct();

  renderer_ = vtkSmartPointer<vtkRenderer>::New();
  renderWindow_ = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow_->AddRenderer(renderer_);
  renderWindow_->OffScreenRenderingOn();
  renderWindow_->SetSize(1920, 1440);
  renderWindow_->SetMultiSamples(8);
  renderer_->SetBackground(1, 1, 1); // Background color white

  auto renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow_);
  auto axes = vtkSmartPointer<vtkAxesActor>::New();
  auto widget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
  widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
  widget->SetOrientationMarker( axes );
  widget->SetInteractor( renderWindowInteractor );
  widget->SetViewport( 0.0, 0.0, 0.4, 0.4 );
  widget->SetEnabled( 1 );
  widget->InteractiveOn();

  auto light_kit = vtkSmartPointer<vtkLightKit>::New();
  light_kit->SetKeyLightIntensity(1.0);
  light_kit->AddLightsToRenderer(renderer_);
}




VTKOffscreenScene::~VTKOffscreenScene()
{
}




void VTKOffscreenScene::addActor2D(vtkSmartPointer<vtkActor2D> actor)
{
  renderer_->AddActor2D(actor);
}


vtkSmartPointer<vtkScalarBarActor> VTKOffscreenScene::addColorBar(
      const std::string& title,
      vtkSmartPointer<vtkLookupTable> lut,
      double x, double y,
      bool horiz,
      double w, double len,
      double fontmult
      )
{
  auto cb = vtkSmartPointer<vtkScalarBarActor>::New();
  cb->SetLookupTable(lut);
  if (horiz)
  {
    cb->SetOrientationToHorizontal();
    cb->SetHeight(w);
    cb->SetWidth(len);
  }
  else
  {
    cb->SetHeight(len);
    cb->SetWidth(w);
  }
  cb->SetPosition(x, y);
  cb->SetLabelFormat("%g");
  cb->SetNumberOfLabels(5);
  cb->VisibilityOn();
  cb->SetTitle(title.c_str());
  cb->SetBarRatio(0.15);
  cb->SetUnconstrainedFontSize(true);
  cb->GetTitleTextProperty()->SetColor(0,0,0);
  cb->GetTitleTextProperty()->SetFontSize(fontmult*double(cb->GetTitleTextProperty()->GetFontSize()));
  cb->GetLabelTextProperty()->SetColor(0,0,0);
  cb->GetLabelTextProperty()->SetItalic(false);
  cb->GetLabelTextProperty()->SetFontSize(fontmult*double(cb->GetLabelTextProperty()->GetFontSize()));
  cb->GetAnnotationTextProperty()->SetColor(0,0,0);
  cb->GetAnnotationTextProperty()->SetItalic(false);
  cb->GetAnnotationTextProperty()->SetFontSize(fontmult*double(cb->GetAnnotationTextProperty()->GetFontSize()));

  renderer_->AddActor(cb);

  return cb;
}

void VTKOffscreenScene::exportX3D(const boost::filesystem::path& file)
{
  auto x3d=vtkSmartPointer<vtkX3DExporter>::New();
  x3d->SetInput(renderWindow_);
  x3d->SetFileName(file.string().c_str());
  x3d->BinaryOff();
  x3d->Update();
  x3d->Write();
}

void VTKOffscreenScene::exportImage(const boost::filesystem::path& pngfile)
{
  renderWindow_->Render();
  auto windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
  windowToImageFilter->SetInput(renderWindow_);
  windowToImageFilter->Update();
  auto writer = vtkSmartPointer<vtkPNGWriter>::New();
  writer->SetFileName(pngfile.string().c_str());
  writer->SetInputConnection(windowToImageFilter->GetOutputPort());
  writer->Write();
}




vtkCamera* VTKOffscreenScene::activeCamera()
{
  return renderer_->GetActiveCamera();
}




void VTKOffscreenScene::setupActiveCamera(const View &view)
{
  auto camera = activeCamera();
  camera->SetFocalPoint( toArray(view.focalPoint()) );
  camera->SetViewUp( toArray(view.upwardDirection()) );
  camera->SetPosition( toArray(view.cameraLocation()) );
}




void VTKOffscreenScene::setParallelScale(
    boost::variant<
      double, // scale
      std::pair<double,double> // Lh, Lv
    > scaleOrSize
    )
{
  if (const auto* sz = boost::get<std::pair<double,double> >(&scaleOrSize))
  {
      double w = sz->first;
      double h = sz->second;
      double W = renderWindow_->GetSize()[0];
      double H = renderWindow_->GetSize()[1];
      double HbyW=H/W;
      double scale=0.5*std::max(w*HbyW, h);
      //print W, H, w, h, scale
      cout<<"setParallelScale: W, H, w, h, scale = "<<W<<", "<<H<<", "<<w<<", "<<h<<", "<<scale<<endl;
      activeCamera()->SetParallelScale(scale);
  }
  else if (const auto* sc = boost::get<double>(&scaleOrSize))
  {
      activeCamera()->SetParallelScale(*sc);
  }
}

void VTKOffscreenScene::fitAll(double mult)
{
  double bnds[6] = {DBL_MAX, -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX, -DBL_MAX};

  renderWindow_->Render();

  auto aa = renderer_->GetActors();
  aa->InitTraversal();
  for (vtkActor*a = aa->GetNextItem(); a!=0; a = aa->GetNextItem())
  {
    double abnds[6]={0};
    a->GetBounds(abnds);
    for (int i=0;i<3;i++)
    {
      bnds[2*i]=std::min(bnds[2*i], abnds[2*i]);
      bnds[2*i+1]=std::max(bnds[2*i+1], abnds[2*i+1]);
    }
  }

  arma::mat L=vec3(bnds[1]-bnds[0], bnds[3]-bnds[2], bnds[5]-bnds[4]);
  arma::mat ctr=vec3(
      0.5*(bnds[1]+bnds[0]),
      0.5*(bnds[3]+bnds[2]),
      0.5*(bnds[5]+bnds[4])
      );

  arma::mat p, fp, ey;
  p=fp=ey=vec3(0,0,0);
  activeCamera()->GetPosition(p.memptr());
  activeCamera()->GetFocalPoint(fp.memptr());
  activeCamera()->GetViewUp(ey.memptr());
  arma::mat n=p-fp; n/=norm(n,2);
  ey/=norm(ey,2);
  arma::mat ex=-arma::cross(n,ey); ex/=norm(ex,2);
  ey = arma::cross(n, ex);

  arma::mat diff=ctr-p; diff-=dot(diff,n)*n;
  arma::mat np=p+diff, nfp=fp+diff;
  activeCamera()->SetPosition( np.memptr() );
  activeCamera()->SetFocalPoint( nfp.memptr() );

  double w= fabs(arma::dot(vec3(L[0],0,0), ex))+fabs(arma::dot(vec3(0,L[1],0), ex))+fabs(arma::dot(vec3(0,0,L[2]), ex));
  double h= fabs(arma::dot(vec3(L[0],0,0), ey))+fabs(arma::dot(vec3(0,L[1],0), ey))+fabs(arma::dot(vec3(0,0,L[2]), ey));

  setParallelScale(std::pair<double,double>(mult*w, mult*h));
}

void VTKOffscreenScene::clearScene()
{
  auto acts = renderer_->GetActors();
  vtkActor *act;
  for( acts->InitTraversal(); (act = acts->GetNextItem())!=nullptr; )
  {
      renderer_->RemoveActor( act );
  }

  auto acts2 = renderer_->GetActors2D();
  vtkActor2D *act2;
  for( acts2->InitTraversal(); (act2 = acts2->GetNextItem())!=nullptr; )
  {
      renderer_->RemoveActor2D( act2 );
  }

//  renderer_->Clear();
}

void VTKOffscreenScene::removeActor(vtkActor *act)
{
  renderer_->RemoveActor(act);
}

void VTKOffscreenScene::removeActor2D(vtkActor2D *act)
{
  renderer_->RemoveActor2D(act);
}



//class ModifiedPOpenFOAMReader : public vtkPOpenFOAMReader
//{
//  ModifiedPOpenFOAMReader()
//      : vtkPOpenFOAMReader()
//  {}

//public:
//  static ModifiedPOpenFOAMReader* New();

////    int UpdateTimeStep(
////      double time, int piece = -1, int numPieces = 1, int ghostLevels = 0,
////      const int extents[6] = nullptr) override
////    {
////      for (int r=0; r<Readers->GetNumberOfItems(); ++r)
////      {
////          if (reader = vtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetItemAsObject(i)))
////            reader->UpdateTimeStep(time, piece, numPieces, ghostLevels, extents);
////      }
////      return vtkPOpenFOAMReader::UpdateTimeStep(time, piece, numPieces, ghostLevels, extents);
////    }
//};

//vtkStandardNewMacro(ModifiedPOpenFOAMReader);

OpenFOAMCaseScene::OpenFOAMCaseScene(
    const boost::filesystem::path& casepath,
    bool readZones,
    int np)
  : VTKOffscreenScene()
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
    auto controller =vtkSmartPointer<vtkMPIController>::New();
  controller->Initialize(nullptr, nullptr);
  int rank = controller->GetLocalProcessId();
  // vtkLogger::SetThreadName("rank=" + std::to_string(rank));
  vtkMultiProcessController::SetGlobalController(controller);
#endif

  ofcase_ = vtkSmartPointer<vtkOpenFOAMReader>::New();
  ofcase_->SetFileName( casepath.string().c_str() );
  ofcase_->Update();

  //ofcase_->SetSkipZeroTime(false);
//  ofcase_->SetCaseType(
//      np > 1 ?
//          vtkPOpenFOAMReader::DECOMPOSED_CASE :
//          vtkPOpenFOAMReader::RECONSTRUCTED_CASE );

  ofcase_->SetUse64BitLabels(false);
  ofcase_->SetUse64BitFloats(true);

  ofcase_->CreateCellToPointOn();
  ofcase_->AddDimensionsToArrayNamesOff();

  ofcase_->EnableAllPatchArrays();
  ofcase_->EnableAllCellArrays();
  ofcase_->EnableAllPointArrays();
  ofcase_->EnableAllLagrangianArrays();

  ofcase_->CacheMeshOn();
  ofcase_->DecomposePolyhedraOn();
  ofcase_->ListTimeStepsByControlDictOff();
  if (readZones)
  {
      ofcase_->ReadZonesOn();
  }
  else
  {
      ofcase_->ReadZonesOff();
  }

  ofcase_->Update();

  auto execInfo = ofcase_->GetExecutive()->GetOutputInformation(0);
  int nt = execInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
//  times_ = ofcase_->GetTimeValues();
  times_.resize(nt);
  execInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times_[0]);

  cout<<"VTK OpenFOAM Reader: available times = (";
  for (int i=0; i<times_.size(); i++)
  {
    cout<<" "<<times_[i]<<":"<<i<<endl;
  }
  cout<<" )"<<endl;

  setTimeValue( times_.back() );

  for (int i=0; i<ofcase_->GetNumberOfPatchArrays(); i++)
  {
    patches_[ofcase_->GetPatchArrayName(i)]=i;
  }
}

vtkMultiBlockDataSet* OpenFOAMCaseScene::GetOutput()
{
  return ofcase_->GetOutput();
}

const std::vector<double>& OpenFOAMCaseScene::times() const
{
  return times_;
}

double OpenFOAMCaseScene::setTimeValue(double t)
{
  auto ti = std::lower_bound(times_.begin(), times_.end(), t);
  insight::assertion(
      ti!=times_.end(),
      _("no lower bound found for t=%g!"), t );

  auto execInfos = ofcase_->GetExecutive()->GetOutputInformation();
  double tact = *ti;
  std::cerr<<"tact="<<tact<<std::endl;
  ofcase_->UpdateInformation();
  for (int i=0; i<execInfos->GetNumberOfInformationObjects(); ++i)
  {
    std::cout<<"set "<<i<<std::endl;
    execInfos->GetInformationObject(i)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), tact );
  }
  ofcase_->SetTimeValue(tact);
  ofcase_->UpdateTimeStep(tact);

  ofcase_->Modified();
  ofcase_->Update();

  return tact;
}


void OpenFOAMCaseScene::setTimeIndex(vtkIdType timeId)
{
  setTimeValue( times_[timeId] );
}


vtkSmartPointer<vtkUnstructuredGridAlgorithm> OpenFOAMCaseScene::internalMeshFilter() const
{
  auto internal = extractBlocks(
      MultiBlockDataSetExtractor(ofcase_->GetOutput()).flatIndices(
          {"internalMesh"} ));

  auto af = vtkSmartPointer<vtkCompositeDataToUnstructuredGridFilter>::New();
  af->AddInputConnection(internal->GetOutputPort());
  return af;
}



vtkSmartPointer<vtkUnstructuredGrid> OpenFOAMCaseScene::internalMesh() const
{
  auto af = internalMeshFilter();
  af->Update();
  return af->GetOutput();
}


std::vector<std::string> OpenFOAMCaseScene::matchingPatchNames(const std::string& patchNamePattern) const
{
  std::vector<string> patchNames;

  regex pattern(patchNamePattern);
  for (const auto& p: patches_)
  {
    if (regex_match(p.first, pattern))
    {
      patchNames.push_back(p.first);
      cout<<"selecting for "<<patchNamePattern<<": "<<p.first<<" (id "<<p.second<<")"<<endl;
    }
  }

  return patchNames;
}

vtkPolyData* OpenFOAMCaseScene::patch(const std::string& name) const
{
  auto oo=ofcase_->GetOutput();
  for (vtkIdType i=0; i<oo->GetNumberOfBlocks(); i++)
  {
    auto md=oo->GetMetaData(i);
    if ( std::string(md->Get(vtkCompositeDataSet::NAME()))=="Patches" )
    {
      auto pm = vtkMultiBlockDataSet::SafeDownCast(oo->GetBlock(i));
      for (vtkIdType j=0; j<pm->GetNumberOfBlocks(); j++)
      {
        if ( std::string(pm->GetMetaData(j)->Get(vtkCompositeDataSet::NAME()))==name )
        {
          auto pmesh=vtkPolyData::SafeDownCast(pm->GetBlock(j));
          return pmesh;
        }
      }
    }
  }

  return nullptr;
}

vtkSmartPointer<vtkUnstructuredGridAlgorithm> OpenFOAMCaseScene::patchesFilter(
    const std::string& namePattern) const
{
  auto patches = extractBlock(
      MultiBlockDataSetExtractor(ofcase_->GetOutput()).flatIndices(
          { "Patches", namePattern } ));

  auto pf = vtkSmartPointer<vtkCompositeDataToUnstructuredGridFilter>::New();
  pf->AddInputConnection(patches->GetOutputPort());
  return pf;
}


vtkSmartPointer<vtkUnstructuredGrid> OpenFOAMCaseScene::patches(const std::string& namePattern) const
{
  auto pf = patchesFilter(namePattern);
  pf->Update();
  return pf->GetOutput();
}

vtkSmartPointer<vtkOpenFOAMReader> OpenFOAMCaseScene::ofcase() const
{
  return ofcase_;
}

vtkSmartPointer<vtkCompositeDataGeometryFilter> OpenFOAMCaseScene::extractBlock(const std::string& name) const
{
  std::set<int> blkIdx;
  if (patches_.find(name)!=patches_.end())
  {
    int i=patches_.at(name);
    blkIdx.insert(i);
  }
  else
  {
    auto names = matchingPatchNames(name);
    for (const auto& pn: names)
    {
      blkIdx.insert(patches_.at(pn));
    }
    if (blkIdx.size()==0)
      throw insight::Exception(
          str(format("No patch matches expression \"%s\"!")%name)
          );
  }
  return extractBlock(blkIdx);
}

vtkSmartPointer<vtkCompositeDataGeometryFilter> OpenFOAMCaseScene::extractBlock(int blockIdx) const
{
  return extractBlock(std::set<int>({blockIdx}));
}

vtkSmartPointer<vtkMultiBlockDataSetAlgorithm> OpenFOAMCaseScene::extractBlocks(const std::set<int>& blockIdxs) const
{
  insight::assertion(
      blockIdxs.size()>0,
      "no block indices to extract were provided!" );

  auto eb = vtkSmartPointer<vtkExtractBlock>::New();
  eb->SetInputConnection(ofcase_->GetOutputPort());
  for (int i: blockIdxs)
  {
    eb->AddIndex( i );
  }
  return eb;
}

vtkSmartPointer<vtkCompositeDataGeometryFilter> OpenFOAMCaseScene::extractBlock(const std::set<int>& blockIdxs) const
{
  auto eb2 = vtkSmartPointer<vtkCompositeDataGeometryFilter>::New();
  eb2->SetInputConnection(extractBlocks(blockIdxs)->GetOutputPort());
  return eb2;
}

FieldSelection::FieldSelection(string fieldName, FieldSupport fieldSupport, int component)
  : boost::fusion::tuple
    <
     std::string,
     FieldSupport,
     int
    > ( fieldName, fieldSupport, component)
{}

string FieldSelection::fieldName() const
{
  return boost::fusion::at_c<0>(*this);
}


FieldColor::FieldColor(FieldSelection fs,
           vtkSmartPointer<vtkLookupTable> lut,
           RangeSelection rs)
  : boost::fusion::tuple
    <
     FieldSelection,
     vtkSmartPointer<vtkLookupTable>,
     RangeSelection
    >(fs, lut, rs)
{}

vtkSmartPointer<vtkLookupTable> FieldColor::lookupTable() const
{
  return boost::fusion::at_c<1>(*this);
}



void forEachUnconnectedPart(
    OpenFOAMCaseScene& scene,
    const boost::filesystem::path& exePath,
    ResultSection *section,
    vtkAlgorithm* in,
    std::function<void(vtkAlgorithm*, ResultSection*, int )> displayRegion)
{
    auto cf =  vtkSmartPointer<vtkConnectivityFilter>::New();
    cf->SetInputConnection(in->GetOutputPort());
    cf->SetExtractionModeToAllRegions();
    cf->ColorRegionsOn();
    cf->Update();

    int nregions = cf->GetNumberOfExtractedRegions();
    if (nregions==1)
    {
        scene.clearScene();
        displayRegion(in, section, -1 );
    }
    else
    {

        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetNumberOfPoints(nregions);
        auto labels = vtkSmartPointer<vtkStringArray>::New();
        labels->SetName("labels");
        labels->SetNumberOfValues(nregions);
        auto sizes = vtkSmartPointer<vtkIntArray>::New();
        sizes->SetName("sizes");
        sizes->SetNumberOfValues(nregions);

        std::vector<vtkSmartPointer<vtkThreshold> > regions(nregions);
        std::vector<arma::mat> regionCtrs(nregions);

        for (int r=0; r<nregions; ++r)
        {
            auto thr = vtkSmartPointer<vtkThreshold>::New();
            thr->SetInputConnection(cf->GetOutputPort());
            thr->SetInputArrayToProcess(
                0, 0, 0,
                vtkDataObject::FIELD_ASSOCIATION_CELLS,
                "RegionId" );
            thr->ThresholdBetween(r, r);            
            thr->Update();

            regions[r]=thr;


            {
                auto es = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                es->SetInputConnection(thr->GetOutputPort());
                auto ms = vtkSmartPointer<vtkCellSizeFilter>::New();
                ms->ComputeAreaOn();
                ms->SetInputConnection(es->GetOutputPort());
                auto ctrs = vtkSmartPointer<vtkCellCenters>::New();
                ctrs->SetInputConnection(ms->GetOutputPort());
                ctrs->Update();

                auto pp = ctrs->GetOutput()->GetPoints();
                auto Ap = ctrs->GetOutput()->GetPointData()->GetArray("Area");
                arma::mat ctr=vec3Zero();
                double A=0;
                for (int i=0; i<pp->GetNumberOfPoints(); ++i)
                {
                    A += Ap->GetTuple1(i);
                    ctr += vec3FromComponents(pp->GetPoint(i)) * Ap->GetTuple1(i);
                }
                regionCtrs[r] = ctr/A;
            }

            auto pts=thr->GetOutput()->GetPoints();
            double p[3];
            pts->GetPoint(0, p);
            points->SetPoint(r, p);
            labels->SetValue(r, str(format("%d") % r).c_str());
            sizes->SetValue(r, 4);
        }

        auto pointSource = vtkSmartPointer<vtkPolyData>::New();
        pointSource->SetPoints(points);
        pointSource->GetPointData()->AddArray(labels);
        pointSource->GetPointData()->AddArray(sizes);

        auto pts2Lbl = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
        pts2Lbl->SetInputData(pointSource);
        pts2Lbl->SetLabelArrayName("labels");
        pts2Lbl->SetPriorityArrayName("sizes");
        pts2Lbl->GetTextProperty()->SetColor(0,0,0);
        pts2Lbl->Update();

        // Create a mapper and actor for the labels.
        auto lblMap = vtkSmartPointer<vtkLabelPlacementMapper>::New();
        lblMap->SetInputConnection(
            pts2Lbl->GetOutputPort());

        auto lblActor = vtkSmartPointer<vtkActor2D>::New();
        lblActor->SetMapper(lblMap);

        // display overview
        scene.clearScene();

        FieldSelection ri_field("RegionId", FieldSupport::OnCell, -1);
        FieldColor ri_fc(ri_field, createColorMap(), calcRange(ri_field, {}, {cf}));
        scene.addAlgo<vtkDataSetMapper>(in, ri_fc);
        scene.addActor(lblActor);

        scene.fitAll();

        auto img = exePath / ("regions.png");
        scene.exportImage(img);
        section->insert(img.filename().stem().string(),
                         std::unique_ptr<Image>(new Image
                         (
                            exePath, img.filename(),
                            "Region IDs", ""
                            ))).setOrder(0);


        AttributeTableResult::AttributeNames rowLabels(nregions);
        std::map<std::string, AttributeTableResult::AttributeValues> scalarQtys;

        for (int r=0; r<nregions; ++r)
        {
            auto thr = regions[r];

            auto subsec = std::make_shared<ResultSection>(
                str(boost::format("Region %d")%(r+1))
                );

            auto regionPrefix = str(boost::format("region_%d")%(r+1));

            scene.clearScene();
            displayRegion( thr, subsec.get(), r );

            subsec->insert(
                "regionCoG",
                new VectorResult(
                    regionCtrs[r],
                    str(boost::format(_("center of gravity of region %d"))%r), "", "")
                );

            subsec->setOrder(10.*r);
            section->insert(
                regionPrefix,
                subsec );

            rowLabels[r]=regionPrefix;
            for (const auto& q: *subsec)
            {
                if (auto s = std::dynamic_pointer_cast<ScalarResult>(q.second))
                {
                    if (scalarQtys.count(q.first)<1)
                        scalarQtys[q.first].resize(nregions, {"(n.a.)"});

                    scalarQtys[q.first][r] = s->value();
                }
            }
        }

        {
            TabularResult::Table rows;
            for (auto rc: boost::adaptors::index(regionCtrs))
            {
                rows.push_back(
                    {
                        double(rc.index()+1),
                        rc.value()(0), rc.value()(1), rc.value()(2)
                    }
                    );
            }
            section->insert(
                "table_regionCoG",
                new TabularResult({"regionId", "x", "y", "z"},
                                  rows, "table of region CoGs", "", ""));
        }

        for (const auto& sq: boost::adaptors::index(scalarQtys))
        {
            section->insert(
                       "table_"+sq.value().first,
                       new AttributeTableResult(
                           rowLabels, sq.value().second,
                           "table of "+sq.value().first, "", "",
                           SimpleLatex("region"), sq.value().first)
                       ).setOrder(10.*nregions+sq.index());
        }

    }
}

arma::mat average(vtkDataArray *arr)
{
    int nc=arr->GetNumberOfComponents();
    arma::mat avg(nc, 1);
    for (int i=0; i<arr->GetNumberOfTuples(); ++i)
    {
        arma::mat cv(nc, 1);
        arr->GetTuple(i, cv.memptr());
        avg += cv;
    }
    avg/=double(arr->GetNumberOfTuples());
    return avg;
}







}
