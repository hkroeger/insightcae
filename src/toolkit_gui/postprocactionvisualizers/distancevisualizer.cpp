
#include "cadpostprocactions/pointdistance.h"
#include "postprocactionvisualizer.h"

#include "base/boost_include.h"
#include "base/vtkrendering.h"
#include "base/spatialtransformation.h"
#include "cadparameters.h"

#include "occguitools.h"


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


} // namespace cad
} // namespace insight
