#ifndef VTKRENDERING_H
#define VTKRENDERING_H

#include <vector>
#include <string>

#include "base/boost_include.h"
#include "base/linearalgebra.h"

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
#include "vtkUnstructuredGrid.h"
#include "vtkCompositeDataGeometryFilter.h"




namespace insight
{


vtkSmartPointer<vtkPolyData> createArrows(
    std::vector<std::pair<arma::mat, arma::mat> > from_to
    );


extern const std::vector<double> colorMapData_SD;


vtkSmartPointer<vtkLookupTable> createColorMap(
        const std::vector<double>& cb = colorMapData_SD,
        int nc=32
        );



typedef std::pair<double, double> MinMax;

typedef enum { Cell, Point } FieldSupport;

typedef boost::fusion::tuple
    <
     std::string,
     FieldSupport,
     int
    >
        FieldSelection;

typedef boost::variant<boost::blank,MinMax> RangeSelection;

typedef boost::fusion::tuple
    <
     FieldSelection,
     vtkSmartPointer<vtkLookupTable>,
     RangeSelection
    >
        FieldColor;

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





class VTKOffscreenScene
{

public:


protected:
  vtkSmartPointer<vtkRenderer> renderer_;
  vtkSmartPointer<vtkRenderWindow> renderWindow_;

public:
  VTKOffscreenScene();

  template<class Mapper, class Input>
  void addAlgo(
      Input input,
      ColorSpecification colorspec
      )
  {
    input->Update();
    addData<Mapper>(input->GetOutput(), colorspec);
  }

  template<class Mapper, class Input>
  void addData(
      Input input,
      ColorSpecification colorspec
      )
  {
    auto mapper = vtkSmartPointer<Mapper>::New();
    mapper->SetInputData(input);

    addProperties<Mapper>(mapper, colorspec, input);
  }

  template<class Mapper>
  void addProperties(
      vtkSmartPointer<Mapper>& mapper,
      ColorSpecification colorspec,
          vtkDataSet *ds
      )
  {
    auto actor = vtkSmartPointer<vtkActor>::New();

    if (const auto* c = boost::get<arma::mat>(&colorspec))
    {
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
          mm=calcRange(fsel, {ds}, {});
      }
      mapper->SetScalarRange(mm.first, mm.second);

    }

    actor->SetMapper(mapper);
    actor->GetProperty()->BackfaceCullingOff();

    renderer_->AddActor(actor);
  }

  vtkSmartPointer<vtkScalarBarActor> addColorBar(
      const std::string& title,
      vtkSmartPointer<vtkLookupTable> lut,
      double x=0.9, double y=0.1, double w=0.075,
      double fontmult=3.
      );


  void exportX3D(const boost::filesystem::path& file);
  void exportImage(const boost::filesystem::path& pngfile);

  vtkCamera* activeCamera();

  void setParallelScale(
      boost::variant<
        double, // scale
        std::pair<double,double> // Lh, Lv
      > scaleOrSize
      );

  void fitAll(double mult=1.05);
};








class OpenFOAMCaseScene
  : public VTKOffscreenScene,
    public vtkSmartPointer<vtkOpenFOAMReader>
{
  vtkSmartPointer<vtkOpenFOAMReader> ofcase_;
  std::map<std::string,int> patches_;

public:
  OpenFOAMCaseScene(const boost::filesystem::path& casepath);

  vtkSmartPointer<vtkOpenFOAMReader> ofcase() const;
  vtkUnstructuredGrid* internalMesh() const;
  vtkPolyData* patch(const std::string& name) const;
  vtkSmartPointer<vtkCompositeDataGeometryFilter> extractBlock(int blockIdx) const;
  vtkSmartPointer<vtkCompositeDataGeometryFilter> extractBlock(const std::string& name) const;
};




}

#endif // VTKRENDERING_H
