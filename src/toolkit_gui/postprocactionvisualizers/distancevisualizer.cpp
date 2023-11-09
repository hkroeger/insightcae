
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
#include "vtkLineSource.h"
#include "vtkTextActor.h"
#include "vtkCaptionActor2D.h"
#include "vtkProperty2D.h"

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
  auto dist = std::dynamic_pointer_cast<Distance>(ppa);
  insight::assertion( bool(dist), "internal error: expected distance object") ;

  arma::mat p1 = dist->p1_->value();
  arma::mat p2 = dist->p2_->value();
  double L=arma::norm(p2-p1, 2);
  arma::mat dir=normalized(p2-p1);

  arma::mat ofs = dist->dimLineOffset();
  arma::mat pmid = 0.5*(p1+p2);

  double relArrSize = 2.*dist->relativeArrowSize();

  vtkNew<vtkPolyData> a0;
    {
        // Create a vtkPoints object and store the points in it
        vtkNew<vtkPoints> points;
        points->InsertNextPoint(0,0,0);
        points->InsertNextPoint(1.-relArrSize, 0, 0);
        points->InsertNextPoint(1.-relArrSize, relArrSize/5., 0);
        points->InsertNextPoint(1,0,0);
        points->InsertNextPoint(1.-relArrSize, -relArrSize/5., 0);
        points->InsertNextPoint(1.-relArrSize, 0, 0);

        vtkNew<vtkPolyLine> polyLine;
        polyLine->GetPointIds()->SetNumberOfIds(points->GetNumberOfPoints());
        for (unsigned int i = 0; i < 6; i++)
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

    auto hl1 = vtkSmartPointer<vtkLineSource>::New();
    hl1->SetPoint1(p1.memptr());
    hl1->SetPoint2(arma::mat(p1+ofs).memptr());
    auto mapper1 = vtkSmartPointer<vtkPolyDataMapper>::New();
    //mapper->SetInputData(arr);
    mapper1->SetInputConnection(hl1->GetOutputPort());
    mapper1->Update();
    auto arrAct1 = vtkSmartPointer<vtkActor>::New();
    arrAct1->SetMapper( mapper1 );
    arrAct1->GetProperty()->SetColor(0.5, 0.5, 0.5);

    auto hl2 = vtkSmartPointer<vtkLineSource>::New();
    hl2->SetPoint1(p2.memptr());
    hl2->SetPoint2(arma::mat(p2+ofs).memptr());
    auto mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
    //mapper->SetInputData(arr);
    mapper2->SetInputConnection(hl2->GetOutputPort());
    mapper2->Update();
    auto arrAct2 = vtkSmartPointer<vtkActor>::New();
    arrAct2->SetMapper( mapper2 );
    arrAct2->GetProperty()->SetColor(0.5, 0.5, 0.5);


    auto lblActor = vtkSmartPointer<vtkActor2D>::New();

    if (displayCoords)
    {
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
        points->SetPoint(il, arma::mat(pmid+ofs).memptr());
        labels->SetValue(il, str(format("L=%g") % L ).c_str());
        sizes->SetValue(il, 6);
        il++;

        if (displayCoords)
        {
            points->SetPoint(il, arma::mat(p1+ofs).memptr());
            labels->SetValue(il, str(format("[%g %g %g]") % p1(0)%p1(1)%p1(2) ).c_str());
            sizes->SetValue(il, 4);
            il++;

            points->SetPoint(il, arma::mat(p2+ofs).memptr());
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

        lblActor->SetMapper(lblMap);
    }
    else
    {
        auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
        caption->BorderOff();
        caption->SetCaption(str(format("%g") % L ).c_str());
        caption->SetAttachmentPoint( arma::mat(pmid+ofs).memptr() );
        caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
        caption->GetCaptionTextProperty()->SetColor(0,0,0);
        caption->GetCaptionTextProperty()->SetJustificationToCentered();
        caption->GetCaptionTextProperty()->SetFontSize(10);
        caption->GetCaptionTextProperty()->FrameOff();
        caption->GetCaptionTextProperty()->ShadowOff();
        caption->GetCaptionTextProperty()->BoldOff();

        lblActor=caption;
    }


    return { lblActor, arrAct, arrAct1, arrAct2 };
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
