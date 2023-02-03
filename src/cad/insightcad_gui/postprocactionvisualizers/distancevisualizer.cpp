
#include "postprocactionvisualizer.h"
#include "base/boost_include.h"
#include "base/vtkrendering.h"
#include "cadpostprocactions/pointdistance.h"
#include "base/spatialtransformation.h"

#include "occguitools.h"

#include "vtkTextProperty.h"
#include "vtkStringArray.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPolyDataMapper.h"
#include "vtkArrowSource.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkGlyph3D.h"
#include "vtkDoubleArray.h"

using namespace boost;

namespace insight {
namespace cad {


Handle_AIS_InteractiveObject Distance_createAISRepr(PostprocActionPtr ppa)
{
  auto h = std::dynamic_pointer_cast<Distance>(ppa);
  insight::assertion( bool(h), "internal error: expected distance object") ;

  arma::mat p1=h->p1_->value();
  arma::mat p2=h->p2_->value();
  double L=arma::norm(p2-p1,2);

  Handle_AIS_InteractiveObject aisDim(createLengthDimension(
    BRepBuilderAPI_MakeVertex(to_Pnt(p1)),
    BRepBuilderAPI_MakeVertex(to_Pnt(p2)),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(p1), gp_Vec(0,0,1)))),
    L,
    ""
  ));
  Handle_AIS_InteractiveObject p1Label(new InteractiveText
    (
      str(format("[%g %g %g]") % p1(0)%p1(1)%p1(2) ).c_str(),
      p1
    ));
  Handle_AIS_InteractiveObject p2Label(new InteractiveText
    (
      str(format("[%g %g %g]") % p2(0)%p2(1)%p2(2) ).c_str(),
      p2
    ));


  Handle_AIS_MultipleConnectedInteractive ais(new AIS_MultipleConnectedInteractive());

  ais->Connect(aisDim);
  ais->Connect(p1Label);
  ais->Connect(p2Label);

  return ais;
}


addStandaloneFunctionToStaticFunctionTable(
    PostProcActionVisualizers,
    Distance,
    createAISReprByTypeName,
    Distance_createAISRepr
    );





PostProcActionVisualizers::VTKActorList Distance_createVTKRepr(PostprocActionPtr ppa)
{
    auto h = std::dynamic_pointer_cast<Distance>(ppa);
    insight::assertion( bool(h), "internal error: expected distance object") ;

    arma::mat p1=h->p1_->value();
    arma::mat p2=h->p2_->value();
    arma::mat pmid = 0.5*(p1+p2);
    double L=arma::norm(p2-p1,2);

    auto a0 = vtkSmartPointer<vtkArrowSource>::New();
    //auto tf= vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    auto gl = vtkSmartPointer<vtkGlyph3D>::New();
    auto gd = vtkSmartPointer<vtkPolyData>::New();
    auto gdpts = vtkSmartPointer<vtkPoints>::New();
    gdpts->SetNumberOfPoints(1);
    gdpts->SetPoint(0, pmid.memptr());
    auto gddirs = vtkSmartPointer<vtkDoubleArray>::New();
    gddirs->SetNumberOfTuples(1);
    gddirs->SetNumberOfComponents(3);
    arma::mat d1(p1-pmid);
    gddirs->SetTypedTuple(0, d1.memptr());
    gddirs->SetName("dir");
{

    a0->SetShaftResolution(4);
    a0->SetTipResolution(4);
//    a0->SetTipRadius(0.025);
//    a0->SetTipLength(0.1);
//    a0->SetShaftRadius(0.0075);
    a0->Update();

    gd->SetPoints(gdpts);
    gd->GetPointData()->AddArray(gddirs);

    gl->SetInputData(gd);
    gl->SetSourceConnection(a0->GetOutputPort());
    gl->SetScaleModeToScaleByScalar();
    gl->SetScaleFactor(1);
    gl->SetInputArrayToProcess(
          0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "dir");

//    tf->SetInputConnection(a0->GetOutputPort());
//    auto t= vtkSmartPointer<vtkTransform>::New();
//    t->Scale(1e3, 1e3, 1e3);
//    tf->SetTransform(t);
    //tf->SetTransform( st1.appended(st2).toVTKTransform() );

}
    /*
    auto arr = createArrows(
                { {pmid, p1},
                  {pmid, p2} },
                false );
    insight::dbg()<<arr->GetNumberOfCells()<<" "<<arr->GetNumberOfPoints()<<std::endl;
    for (int i=0; i<arr->GetNumberOfPoints(); ++i)
    {
        double p[3];
        arr->GetPoint(i, p);
        insight::dbg()<<p[0]<<" "<<p[1]<<" "<<p[2]<<std::endl;
    }*/
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    //mapper->SetInputData(arr);
    mapper->SetInputConnection(gl->GetOutputPort());
    auto arrAct = vtkSmartPointer<vtkActor>::New();
    arrAct->SetMapper( mapper );
    arrAct->GetProperty()->SetColor(0.5,0.5,0.5);

    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(3);
    auto labels = vtkSmartPointer<vtkStringArray>::New();
    labels->SetName("labels");
    labels->SetNumberOfValues(3);
    auto sizes = vtkSmartPointer<vtkIntArray>::New();
    sizes->SetName("sizes");
    sizes->SetNumberOfValues(3);

    points->SetPoint(0, p1.memptr());
    labels->SetValue(0, str(format("[%g %g %g]") % p1(0)%p1(1)%p1(2) ).c_str());
    sizes->SetValue(0, 4);

    points->SetPoint(1, pmid.memptr());
    labels->SetValue(1, str(format("L=%g") % L ).c_str());
    sizes->SetValue(1, 6);

    points->SetPoint(2, p2.memptr());
    labels->SetValue(2, str(format("[%g %g %g]") % p2(0)%p2(1)%p2(2) ).c_str());
    sizes->SetValue(2, 4);

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

    return { arrAct/*, lblActor*/ };
}


addStandaloneFunctionToStaticFunctionTable(
    PostProcActionVisualizers,
    Distance,
    createVTKReprByTypeName,
    Distance_createVTKRepr
    );

} // namespace cad
} // namespace insight
