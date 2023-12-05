
#include "base/units.h"
#include "cadparameter.h"
#include "postprocactionvisualizer.h"
#include "cadpostprocactions/angle.h"

#include "vtkActor.h"
#include "vtkArcSource.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkPointData.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyLineSource.h"
#include "vtkProperty.h"
#include "vtkLineSource.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkActor2D.h"

using namespace boost;

namespace insight {
namespace cad {




PostProcActionVisualizers::VTKActorList Angle_createVTKRepr(PostprocActionPtr ppa)
{
    auto angleDim = std::dynamic_pointer_cast<Angle>(ppa);
    insight::assertion( bool(angleDim), "internal error: expected distance object") ;

    arma::mat p1=angleDim->p1_->value();
    arma::mat p2=angleDim->p2_->value();
    arma::mat pCtr=angleDim->pCtr_->value();

    double rDimLine = std::max(
        insight::LSMALL, angleDim->dimLineRadius() );

    double arrSize = rDimLine *
        angleDim->angle_ * angleDim->relativeArrowSize();

    arma::mat r1=p1-pCtr;
    arma::mat r2=p2-pCtr;
    arma::mat er1 = normalized(r1);
    arma::mat er2 = normalized(r2);

    arma::mat n = normalized(arma::cross(r1, r2));

    auto arc = vtkSmartPointer<vtkArcSource>::New();
    arc->SetCenter( pCtr.memptr() );
    arma::mat ap1=pCtr+rDimLine*er1;
    arc->SetPoint1( ap1.memptr() );
    arma::mat ap2=pCtr+rDimLine*er2;
    arc->SetPoint2( ap2.memptr() );
    arc->SetResolution(32);

    // tangents pointing inwards
    arma::mat et1=-normalized(arma::cross(er1, n));
    arma::mat et2=normalized(arma::cross(er2, n));

    auto addP = [&](vtkPolyLineSource& pp, int i, const arma::mat& p)
    {
        pp.SetPoint(i, p(0), p(1), p(2) );
    };

    auto ah1 = vtkSmartPointer<vtkPolyLineSource>::New();
    ah1->SetNumberOfPoints(3);
    ah1->ClosedOn();
    addP(*ah1, 0, ap1);
    addP(*ah1, 1, ap1+arrSize*(et1+er1/5./2.));
    addP(*ah1, 2, ap1+arrSize*(et1-er1/5./2.));

    auto ah2 = vtkSmartPointer<vtkPolyLineSource>::New();
    ah2->SetNumberOfPoints(3);
    ah2->ClosedOn();
    addP(*ah2, 0, ap2);
    addP(*ah2, 1, ap2+arrSize*(et2+er2/5./2.));
    addP(*ah2, 2, ap2+arrSize*(et2-er2/5./2.));

    auto dl1 = vtkSmartPointer<vtkLineSource>::New();
    dl1->SetPoint1(p1.memptr());
    dl1->SetPoint2(arma::mat(ap1+er1*arrSize*2).memptr());

    auto dl2 = vtkSmartPointer<vtkLineSource>::New();
    dl2->SetPoint1(p2.memptr());
    dl2->SetPoint2(arma::mat(ap2+er2*arrSize*2).memptr());

    auto creaM = [](vtkAlgorithm* algo) {
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(algo->GetOutputPort());
        auto act = vtkSmartPointer<vtkActor>::New();
        act->SetMapper( mapper );
        act->GetProperty()->SetColor(0.5, 0.5, 0.5);
        return act;
    };



    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(1);
    auto labels = vtkSmartPointer<vtkStringArray>::New();
    labels->SetName("labels");
    labels->SetNumberOfValues(1);
    auto sizes = vtkSmartPointer<vtkIntArray>::New();
    sizes->SetName("sizes");
    sizes->SetNumberOfValues(1);
    points->SetPoint(0, arma::mat(pCtr+rDimLine*normalized(er1+er2)).memptr() );
    labels->SetValue(0, str(format("%g°") % (angleDim->angle_/SI::deg) ).c_str());
    sizes->SetValue(0, 6);
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


    return { lblActor, creaM(arc), creaM(ah1), creaM(ah2), creaM(dl1), creaM(dl2) };
}


addStandaloneFunctionToStaticFunctionTable(
    PostProcActionVisualizers,
    Angle,
    createVTKReprByTypeName,
    Angle_createVTKRepr
    );
addStandaloneFunctionToStaticFunctionTable(
    PostProcActionVisualizers,
    AngleConstraint,
    createVTKReprByTypeName,
    Angle_createVTKRepr
    );


} // namespace cad
} // namespace insight
