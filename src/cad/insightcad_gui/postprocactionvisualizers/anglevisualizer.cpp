
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
#include "vtkProperty.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkActor2D.h"

using namespace boost;

namespace insight {
namespace cad {




PostProcActionVisualizers::VTKActorList Angle_createVTKRepr(PostprocActionPtr ppa)
{
    auto h = std::dynamic_pointer_cast<Angle>(ppa);
    insight::assertion( bool(h), "internal error: expected distance object") ;

    arma::mat p1=h->p1_->value();
    arma::mat p2=h->p2_->value();
    arma::mat pCtr=h->pCtr_->value();

    arma::mat d1=p1-pCtr;
    arma::mat d2=p2-pCtr;
    double r1=arma::norm(d1,2);
    double r2=arma::norm(d2,2);
    d1/=r1;
    d2/=r2;

    double ra=std::min(r1, r2)*0.1;

    auto arc = vtkSmartPointer<vtkArcSource>::New();
    arc->SetCenter(pCtr.memptr());
    arc->SetPoint1( arma::mat(pCtr+ra*d1).memptr() );
    arc->SetPoint2( arma::mat(pCtr+ra*d2).memptr() );
    arc->SetResolution(6);


    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    //mapper->SetInputData(arr);
    mapper->SetInputConnection(arc->GetOutputPort());
    mapper->Update();
    auto arrAct = vtkSmartPointer<vtkActor>::New();
    arrAct->SetMapper( mapper );
    arrAct->GetProperty()->SetColor(0.5, 0.5, 0.5);

    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(1);
    auto labels = vtkSmartPointer<vtkStringArray>::New();
    labels->SetName("labels");
    labels->SetNumberOfValues(1);
    auto sizes = vtkSmartPointer<vtkIntArray>::New();
    sizes->SetName("sizes");
    sizes->SetNumberOfValues(1);

    points->SetPoint(0, arma::mat(pCtr+0.5*ra*(d1+d2)).memptr() );
    labels->SetValue(0, str(format("%gdeg") % (h->angle_/SI::deg) ).c_str());
    sizes->SetValue(0, 6);

//    points->SetPoint(1, p1.memptr());
//    labels->SetValue(1, str(format("[%g %g %g]") % p1(0)%p1(1)%p1(2) ).c_str());
//    sizes->SetValue(1, 4);

//    points->SetPoint(2, p2.memptr());
//    labels->SetValue(2, str(format("[%g %g %g]") % p2(0)%p2(1)%p2(2) ).c_str());
//    sizes->SetValue(2, 4);

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

//    auto arr = createArrows(
//                { {pmid, p1},
//                  {pmid, p2} },
//                false );
//    insight::dbg()<<arr->GetNumberOfCells()<<" "<<arr->GetNumberOfPoints()<<std::endl;
//    for (int i=0; i<arr->GetNumberOfPoints(); ++i)
//    {
//        double p[3];
//        arr->GetPoint(i, p);
//        insight::dbg()<<p[0]<<" "<<p[1]<<" "<<p[2]<<std::endl;
//    }

    return { lblActor, arrAct };
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
