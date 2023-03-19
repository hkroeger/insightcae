
#include "postprocactionvisualizer.h"
#include "base/boost_include.h"
#include "base/vtkrendering.h"
#include "cadpostprocactions/pointdistance.h"
#include "base/spatialtransformation.h"

#include "occguitools.h"

#include "vtkGlyphSource2D.h"
#include "vtkNamedColors.h"
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
#include "vtkPolyLine.h"

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



PostProcActionVisualizers::VTKActorList Distance_createVTKRepr(PostprocActionPtr ppa, bool displayCoords)
{
    auto h = std::dynamic_pointer_cast<Distance>(ppa);
    insight::assertion( bool(h), "internal error: expected distance object") ;

    arma::mat p1=h->p1_->value();
    arma::mat p2=h->p2_->value();
    arma::mat dir=p2-p1;
    double L=arma::norm(dir,2);
    dir/=L;

    arma::mat n=arma::cross(dir, vec3(0,0,1));
    if (arma::norm(n,2)<SMALL)
        n=arma::cross(dir, vec3(0,1,0));

    // offset a bit
    arma::mat ofs = n*0.025*L;

    arma::mat pmid = 0.5*(p1+p2);


    vtkNew<vtkPolyData> a0;
    {
        // Create a vtkPoints object and store the points in it
        vtkNew<vtkPoints> points;
        points->InsertNextPoint(0,0,0);
        points->InsertNextPoint(0.95,0,0);
        points->InsertNextPoint(0.95, 0.01, 0);
        points->InsertNextPoint(1,0,0);
        points->InsertNextPoint(0.95, -0.01, 0);
        points->InsertNextPoint(0.95,0,0);

        vtkNew<vtkPolyLine> polyLine;
        polyLine->GetPointIds()->SetNumberOfIds(points->GetNumberOfPoints());
        for (unsigned int i = 0; i < points->GetNumberOfPoints(); i++)
        {
            polyLine->GetPointIds()->SetId(i, i);
        }

        // Create a cell array to store the lines in and add the lines to it
        vtkNew<vtkCellArray> cells;
        cells->InsertNextCell(polyLine);

        // Create a polydata to store everything in

        // Add the points to the dataset
        a0->SetPoints(points);

        // Add the lines to the dataset
        a0->SetLines(cells);
    }


    //auto tf= vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    auto gdpts = vtkSmartPointer<vtkPoints>::New();
    gdpts->SetNumberOfPoints(2);

    auto gddirs = vtkSmartPointer<vtkDoubleArray>::New();
    gddirs->SetNumberOfComponents(3);
    gddirs->SetNumberOfTuples(2);
    gddirs->SetName("dir");

    // set data
    gdpts->SetPoint(0, arma::mat(pmid+ofs).memptr());
    gddirs->SetTypedTuple(
                0, arma::mat(p1-pmid).memptr() );

    gdpts->SetPoint(1, arma::mat(pmid+ofs).memptr());
    gddirs->SetTypedTuple(
                1, arma::mat(p2-pmid).memptr() );

    auto gd = vtkSmartPointer<vtkPolyData>::New();
    gd->SetPoints(gdpts);
    gd->GetPointData()->AddArray(gddirs);
    gd->GetPointData()->SetActiveVectors("dir");


    auto gl = vtkSmartPointer<vtkGlyph3D>::New();
    gl->SetInputData(gd); //data
    gl->SetSourceData(a0); //arrow
    gl->SetVectorModeToUseVector();
    gl->SetScaleModeToScaleByVector();
    gl->SetInputArrayToProcess(
          1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "dir");




    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    //mapper->SetInputData(arr);
    mapper->SetInputConnection(gl->GetOutputPort());
    mapper->Update();
    auto arrAct = vtkSmartPointer<vtkActor>::New();
    arrAct->SetMapper( mapper );
    arrAct->GetProperty()->SetColor(0.5, 0.5, 0.5);

    int nl=displayCoords?3:1;
    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(nl);
    auto labels = vtkSmartPointer<vtkStringArray>::New();
    labels->SetName("labels");
    labels->SetNumberOfValues(nl);
    auto sizes = vtkSmartPointer<vtkIntArray>::New();
    sizes->SetName("sizes");
    sizes->SetNumberOfValues(nl);

    int il=0;
    points->SetPoint(il, pmid.memptr());
    labels->SetValue(il, str(format("L=%g") % L ).c_str());
    sizes->SetValue(il, 6);
    il++;

    if (displayCoords)
    {
        points->SetPoint(il, p1.memptr());
        labels->SetValue(il, str(format("[%g %g %g]") % p1(0)%p1(1)%p1(2) ).c_str());
        sizes->SetValue(il, 4);
        il++;

        points->SetPoint(il, p2.memptr());
        labels->SetValue(il, str(format("[%g %g %g]") % p2(0)%p2(1)%p2(2) ).c_str());
        sizes->SetValue(il, 4);
        il++;
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

    return { lblActor, arrAct };
}

PostProcActionVisualizers::VTKActorList Distance_createVTKRepr_post(PostprocActionPtr ppa)
{ return Distance_createVTKRepr(ppa, true); }


addStandaloneFunctionToStaticFunctionTable(
    PostProcActionVisualizers,
    Distance,
    createVTKReprByTypeName,
    Distance_createVTKRepr_post
    );

PostProcActionVisualizers::VTKActorList Distance_createVTKRepr_constr(PostprocActionPtr ppa)
{ return Distance_createVTKRepr(ppa, false); }

addStandaloneFunctionToStaticFunctionTable(
    PostProcActionVisualizers,
    DistanceConstraint,
    createVTKReprByTypeName,
    Distance_createVTKRepr_constr
    );

} // namespace cad
} // namespace insight
