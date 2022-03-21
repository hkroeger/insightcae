
#include "postprocactionvisualizer.h"
#include "base/boost_include.h"
#include "cadpostprocactions/hydrostatics.h"

#include "occguitools.h"

using namespace boost;

namespace insight {
namespace cad {


Handle_AIS_InteractiveObject Hydrostatics_createAISRepr(PostprocActionPtr ppa)
{
  auto h = std::dynamic_pointer_cast<Hydrostatics>(ppa);
  insight::assertion( bool(h), "internal error: expected Hydrostatics object");

  TopoDS_Edge cG = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(h->G_),gp_Dir(to_Vec(h->elat_))), 1));
  Handle_AIS_Shape aisG = new AIS_Shape(cG);
  TopoDS_Edge cB = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(h->B_),gp_Dir(to_Vec(h->elat_))), 1));
  Handle_AIS_Shape aisB = new AIS_Shape(cB);
  TopoDS_Edge cM = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(h->M_),gp_Dir(to_Vec(h->elat_))), 1));
  Handle_AIS_Shape aisM = new AIS_Shape(cM);

  double d_gm=norm(h->G_-h->M_,2);

  Handle_AIS_InteractiveObject aisDim (createLengthDimension(
    BRepBuilderAPI_MakeVertex(to_Pnt(h->G_)), BRepBuilderAPI_MakeVertex(to_Pnt(h->M_)),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(h->G_), to_Vec(vec3(0,1,0))))),
    d_gm,
    str(format("GM = %g")%d_gm).c_str()
  ));

  Handle_AIS_InteractiveObject aisBLabel (createArrow(
    cB, str(format("B: V_disp = %g") % h->V_)
  ));
  Handle_AIS_InteractiveObject aisGLabel (createArrow(
    cG, str(format("G: m = %g") % h->m_)
  ));

#warning Needs context in older version
  Handle_AIS_MultipleConnectedInteractive ais(new AIS_MultipleConnectedInteractive());

  ais->Connect(aisG);
  ais->Connect(aisB);
  ais->Connect(aisM);
  ais->Connect(aisDim);
  ais->Connect(aisGLabel);
  ais->Connect(aisBLabel);

  return ais;
}


addStandaloneFunctionToStaticFunctionTable(
    PostProcActionVisualizers,
    Hydrostatics,
    createAISReprByTypeName,
    Hydrostatics_createAISRepr
    );

} // namespace cad
} // namespace insight
