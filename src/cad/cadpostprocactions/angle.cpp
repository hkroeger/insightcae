
#include "base/cppextensions.h"
#include "angle.h"
#include "cadfeature.h"
#include "base/units.h"

#include "vtkActor.h"
#include "vtkArcSource.h"
#include "vtkLineSource.h"
#include "vtkPolyLineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkStringArray.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkTextProperty.h"

namespace insight {
namespace cad {




defineType(Angle);




size_t Angle::calcHash() const
{
  ParameterListHash h;
  h+=p1_->value();
  h+=p2_->value();
  h+=pCtr_->value();
  return h.getHash();
}




Angle::Angle(
    insight::cad::VectorPtr p1,
    insight::cad::VectorPtr p2,
    insight::cad::VectorPtr pCtr
    )
: p1_(p1), p2_(p2), pCtr_(pCtr)
{}




double Angle::calculate(
        arma::mat p1,
        arma::mat p2,
        arma::mat pCtr
        )
{
    arma::mat d1 = p1 - pCtr;
    arma::mat d2 = p2 - pCtr;

    if ( (arma::norm(d1, 2)<SMALL)
        || (arma::norm(d2, 2)<SMALL) ) // might appear during sketch resolution
    {
        return 100.*M_PI;
    }

    arma::mat n=arma::cross(d1, d2);
    if (arma::norm(n,2)<SMALL)
    {
        return 0.;
    }
    else
    {
        arma::mat ex=normalized(d1);
        arma::mat ey=normalized(arma::cross(n,ex));
        return atan2(arma::dot(d2, ey), arma::dot(d2, ex));
    }
}




void Angle::build()
{
    angle_ = calculate(
                p1_->value(),
                p2_->value(),
                pCtr_->value() );
}




void Angle::write(ostream&) const
{}




double Angle::dimLineRadius() const
{
    checkForBuildDuringAccess();
    return std::max(
        arma::norm(p1_->value() - pCtr_->value(), 2),
        arma::norm(p2_->value() - pCtr_->value(), 2)
        );
}




double Angle::relativeArrowSize() const
{
    return 0.025;
}




VectorPtr Angle::centerPoint() const
{
    return pCtr_;
}




void Angle::operator=(const Angle &other)
{
    p1_=other.p1_;
    p2_=other.p2_;
    pCtr_=other.pCtr_;
    angle_=other.angle_;
    PostprocAction::operator=(other);
}



std::vector<vtkSmartPointer<vtkProp> > Angle::createVTKRepr() const
{
    insight::CurrentExceptionContext ex("creating angle dimension lines");

    auto angleDim = this;
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

    arma::mat n = arma::cross(r1, r2);
    if (arma::norm(n, 2)<SMALL)
        n=vec3Z(1);
    else
        n=normalized(n);

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
        act->GetProperty()->SetLineWidth(0.5);

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
    labels->SetValue(0, str(boost::format("%g°") % (angleDim->angle_/SI::deg) ).c_str());
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


} // namespace cad
} // namespace insight
