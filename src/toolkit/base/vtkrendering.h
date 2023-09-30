#ifndef VTKRENDERING_H
#define VTKRENDERING_H

#include <vector>
#include <string>
#include <map>

#include "base/boost_include.h"
#include "base/linearalgebra.h"

#include <limits>
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkLookupTable.h"
#include "vtkCamera.h"
#include "vtkScalarBarActor.h"
#include "vtkRenderWindow.h"
#include "vtkActor.h"
#include "vtkColorSeries.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkAlgorithm.h"
#include "vtkOpenFOAMReader.h"
//#include "vtkPOpenFOAMReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkCompositeDataGeometryFilter.h"

#include "boost/property_tree/ptree.hpp"


#include "vtkUnstructuredGridAlgorithm.h"

#include "vtkPassInputTypeAlgorithm.h"

class vtkCompositeDataSet;
class vtkAppendFilter;
class vtkMultiProcessController;

class vtkCleanArrays : public vtkPassInputTypeAlgorithm
{
public:
    static vtkCleanArrays* New();
    vtkTypeMacro(vtkCleanArrays, vtkPassInputTypeAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent) override;

//    //@{
//    /**
//   * The user can set the controller used for inter-process communication. By
//   * default set to the global communicator.
//   */
//    void SetController(vtkMultiProcessController* controller);
//    vtkGetObjectMacro(Controller, vtkMultiProcessController);
//    //@}

    //@{
    /**
   * When set to true (false by default), 0 filled array will be added for
   * missing arrays on this process (instead of removing partial arrays).
   */
    vtkSetMacro(FillPartialArrays, bool);
    vtkGetMacro(FillPartialArrays, bool);
    vtkBooleanMacro(FillPartialArrays, bool);
    //@}

protected:
    vtkCleanArrays();
    ~vtkCleanArrays() override;

    int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
                    vtkInformationVector* outputVector) override;

    vtkMultiProcessController* Controller;

    bool FillPartialArrays;

private:
    vtkCleanArrays(const vtkCleanArrays&) = delete;
    void operator=(const vtkCleanArrays&) = delete;

public:
    class vtkArrayData;
    class vtkArraySet;
};

/**
 * @brief The vtkCompositeDataToUnstructuredGridFilter class
 * copied here from PVVTKExtensionsMisc, because PVExtensions
 * are not available in MXE cross compiler (only plain VTK)
 */
class vtkCompositeDataToUnstructuredGridFilter
    : public vtkUnstructuredGridAlgorithm
{
public:
    static vtkCompositeDataToUnstructuredGridFilter* New();
    vtkTypeMacro(vtkCompositeDataToUnstructuredGridFilter, vtkUnstructuredGridAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    //@{
    /**
   * Get/Set the composite index of the subtree to be merged. By default set to
   * 0 i.e. root, hence entire input composite dataset is merged.
   */
    vtkSetMacro(SubTreeCompositeIndex, unsigned int);
    vtkGetMacro(SubTreeCompositeIndex, unsigned int);
    //@}

    //@{
    /**
   * Turn on/off merging of coincidental points.  Frontend to
   * vtkAppendFilter::MergePoints. Default is on.
   */
    vtkSetMacro(MergePoints, bool);
    vtkGetMacro(MergePoints, bool);
    vtkBooleanMacro(MergePoints, bool);
    //@}

    //@{
    /**
   * Get/Set the tolerance to use to find coincident points when `MergePoints`
   * is `true`. Default is 0.0.
   *
   * This is simply passed on to the internal vtkAppendFilter::vtkLocator used to merge points.
   * @sa `vtkLocator::SetTolerance`.
   */
    vtkSetClampMacro(Tolerance, double, 0.0, VTK_DOUBLE_MAX);
    vtkGetMacro(Tolerance, double);
    //@}

protected:
    vtkCompositeDataToUnstructuredGridFilter();
    ~vtkCompositeDataToUnstructuredGridFilter() override;

    /**
   * This is called by the superclass.
   * This is the method you should override.
   */
    int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
                    vtkInformationVector* outputVector) override;

    int FillInputPortInformation(int port, vtkInformation* info) override;

    /**
   * Remove point/cell arrays not present on all processes.
   */
    void RemovePartialArrays(vtkUnstructuredGrid* data);

    void AddDataSet(vtkDataSet* ds, vtkAppendFilter* appender);

    void ExecuteSubTree(vtkCompositeDataSet* cd, vtkAppendFilter* output);
    unsigned int SubTreeCompositeIndex;
    bool MergePoints;
    double Tolerance;

private:
    vtkCompositeDataToUnstructuredGridFilter(
        const vtkCompositeDataToUnstructuredGridFilter&) = delete;
    void operator=(const vtkCompositeDataToUnstructuredGridFilter&) = delete;
};



namespace insight
{


vtkSmartPointer<vtkPolyData> createArrows(
        std::vector<std::pair<arma::mat, arma::mat> > from_to,
        bool glyph2d=true
    );


extern const std::vector<double> colorMapData_SD;


vtkSmartPointer<vtkLookupTable> createColorMap(
        const std::vector<double>& cb = colorMapData_SD,
        int nc=32,
        bool logscale = false
        );



typedef std::pair<double, double> MinMax;

typedef enum { Cell = 0, Point = 1 } FieldSupport;

struct FieldSelection
   : public boost::fusion::tuple
    <
     std::string,
     FieldSupport,
     int
    >
{
  FieldSelection(
      std::string fieldName,
      FieldSupport fieldSupport,
      int component=-1
      );

  std::string fieldName() const;
};

typedef boost::variant<boost::blank,MinMax> RangeSelection;

struct FieldColor : public boost::fusion::tuple
    <
     FieldSelection,
     vtkSmartPointer<vtkLookupTable>,
     RangeSelection
    >
{
  FieldColor(FieldSelection fieldSelection,
             vtkSmartPointer<vtkLookupTable> lookupTable,
             RangeSelection rangeSelection);

  vtkSmartPointer<vtkLookupTable> lookupTable() const;
};

typedef boost::variant
    <
     arma::mat,
     FieldColor
    >
        ColorSpecification;



MinMax calcRange(
    FieldSelection fsel,
    const std::vector<vtkDataSet*> datasets,
    const std::vector<vtkAlgorithm*> algorithms
    );


class MultiBlockDataSetExtractor
{
  vtkMultiBlockDataSet* mbds_;
//  struct Node {
//      vtkDataObject* node;
//      int flatIndex;
//  };
//  boost::property_tree::basic_ptree<std::string,Node> tree_;
  std::map<vtkDataObject*,int> flatIndices_;



public:
  MultiBlockDataSetExtractor(vtkMultiBlockDataSet* mbds);

  std::set<int> flatIndices(const std::vector<std::string>& groupNamePatterns) const;

  void findObjectsBelowNode(
          std::vector<std::string> name_pattern,
          vtkDataObject* input,
          std::set<int>& res) const;
};


enum DatasetRepresentation
{
  Points = VTK_POINTS,
  Wireframe = VTK_WIREFRAME,
  Surface = VTK_SURFACE
};

class VTKOffscreenScene
{

protected:
  vtkSmartPointer<vtkRenderer> renderer_;
  vtkSmartPointer<vtkRenderWindow> renderWindow_;

public:
  VTKOffscreenScene();
  ~VTKOffscreenScene();

  template<class Mapper, class Input>
  vtkActor* addAlgo(
      Input input,
      ColorSpecification colorspec = ColorSpecification(insight::vec3(0,0,0)),
      DatasetRepresentation repr = Surface
      )
  {
//    input->Update();
//    return addData<Mapper>(input->GetOutput(), colorspec, repr);
      auto mapper = vtkSmartPointer<Mapper>::New();
      mapper->SetInputConnection(input->GetOutputPort());

      return addProperties<Mapper>(mapper, colorspec, /*input,*/ repr);
  }

  template<class Mapper, class Input>
  vtkActor* addData(
      Input input,
      ColorSpecification colorspec = ColorSpecification(insight::vec3(0,0,0)),
      DatasetRepresentation repr = Surface
      )
  {
    auto mapper = vtkSmartPointer<Mapper>::New();
    mapper->SetInputData(input);

    return addProperties<Mapper>(mapper, colorspec, /*input,*/ repr);
  }

  void addActor2D(vtkSmartPointer<vtkActor2D> actor);

  template<class Mapper>
  vtkActor* addProperties(
      vtkSmartPointer<Mapper>& mapper,
      ColorSpecification colorspec,
//      vtkDataSet *ds,
      DatasetRepresentation repr = Surface
      )
  {
    auto actor = vtkSmartPointer<vtkActor>::New();

    if (const auto* c = boost::get<arma::mat>(&colorspec))
    {
      mapper->ScalarVisibilityOff();
      actor->GetProperty()->SetColor((*c)(0), (*c)(1), (*c)(2));
    }
    else if (const auto *c = boost::get<FieldColor>(&colorspec))
    {
      auto fsel=boost::fusion::get<0>(*c);
      auto fieldname = boost::fusion::get<0>(fsel);
      auto fs = boost::fusion::get<1>(fsel);
      auto cmpt = boost::fusion::get<2>(fsel);
      auto lut = boost::fusion::get<1>(*c);
      auto rsel = boost::fusion::get<2>(*c);

      mapper->SetInterpolateScalarsBeforeMapping(true);
      mapper->SetLookupTable(lut);
      mapper->ScalarVisibilityOn();
      switch (fs)
      {
        case Point:
          mapper->SetScalarModeToUsePointFieldData();
          break;
        case Cell:
          mapper->SetScalarModeToUseCellFieldData();
          break;
      }

      if (cmpt<0)
      {
        lut->SetVectorModeToMagnitude();
      }
      else
      {
        lut->SetVectorModeToComponent();
        lut->SetVectorComponent(cmpt);
      }
      mapper->SelectColorArray(fieldname.c_str());
      MinMax mm;
      if (const auto *mima = boost::get<MinMax>(&rsel))
      {
          mm=*mima;
      }
      else
      {
          insight::assertion(
               mapper->GetInput()!=nullptr,
              "expected dataset input!");
          mm=calcRange(fsel, { mapper->GetInput() }, {});
      }
      mapper->SetScalarRange(mm.first, mm.second);

    }

    actor->SetMapper(mapper);
    actor->GetProperty()->BackfaceCullingOff();
    actor->GetProperty()->SetRepresentation(repr);

    renderer_->AddActor(actor);

    return actor;
  }

  vtkSmartPointer<vtkScalarBarActor> addColorBar(
      const std::string& title,
      vtkSmartPointer<vtkLookupTable> lut,
      double x=0.9, double y=0.1,
      bool horiz=false,
      double w=0.09, double len=0.8,
      double fontmult=3.
      );


  void exportX3D(const boost::filesystem::path& file);
  void exportImage(const boost::filesystem::path& pngfile);

  vtkCamera* activeCamera();
  void setupActiveCamera(const insight::View& view);


  void setParallelScale(
      boost::variant<
        double, // scale
        std::pair<double,double> // Lh, Lv
      > scaleOrSize
      );

  void fitAll(double mult=1.05);

  void clearScene();

  void removeActor(vtkActor* act);
  void removeActor2D(vtkActor2D* act);
};








class OpenFOAMCaseScene
  : public VTKOffscreenScene
{
  vtkSmartPointer<vtkOpenFOAMReader> ofcase_;
  std::map<std::string,int> patches_;
  std::vector<double> times_;

public:
  OpenFOAMCaseScene(const boost::filesystem::path& casepath, int np=1);

  vtkMultiBlockDataSet* GetOutput();

  const std::vector<double>& times() const;

  /**
   * @brief setTimeValue
   * @param t
   * desired time value
   * @return
   * returns the actually set value. That shall be the closest time step available.
   */
  double setTimeValue(double t);

  void setTimeIndex(vtkIdType timeId);

  vtkSmartPointer<vtkOpenFOAMReader> ofcase() const;
  vtkSmartPointer<vtkUnstructuredGridAlgorithm> internalMeshFilter() const;
  vtkSmartPointer<vtkUnstructuredGrid> internalMesh() const;

  std::vector<std::string> matchingPatchNames(const std::string& patchNamePattern) const;

  vtkPolyData* patch(const std::string& name) const;
  vtkSmartPointer<vtkUnstructuredGridAlgorithm> patchesFilter(const std::string& namePattern) const;
  vtkSmartPointer<vtkUnstructuredGrid> patches(const std::string& namePattern) const;

  vtkSmartPointer<vtkCompositeDataGeometryFilter> extractBlock(int blockIdx) const;
  vtkSmartPointer<vtkCompositeDataGeometryFilter> extractBlock(const std::string& name) const;
  vtkSmartPointer<vtkMultiBlockDataSetAlgorithm> extractBlocks(const std::set<int>& blockIdxs) const;
  vtkSmartPointer<vtkCompositeDataGeometryFilter> extractBlock(const std::set<int>& blockIdxs) const;
};




}

#endif // VTKRENDERING_H
