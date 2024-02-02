
#include "postprocactionvisualizer.h"
#include "base/boost_include.h"
#include "cadpostprocactions/solidproperties.h"

#include "occguitools.h"

using namespace boost;

namespace insight {
namespace cad {


Handle_AIS_InteractiveObject SolidProperties_createAISRepr(PostprocActionPtr ppa)
{
  auto h = std::dynamic_pointer_cast<SolidProperties>(ppa);
  insight::assertion( bool(h), "internal error: expected SolidProperties object");

  TopoDS_Edge cG = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(h->cog_),gp_Dir(1,0,0)), 1));
  Handle_AIS_InteractiveObject aisG( new AIS_Shape(cG) );
//  context->Load(aisG);

//   Handle_AIS_InteractiveObject aisGLabel(createArrow(cG, str(format("CoG: m = %g, A = %g") % mass_ % area_)));
  Handle_AIS_InteractiveObject aisGLabel(new InteractiveText
    (
      str(format("CoG: m = %g, A = %g") % h->mass_ % h->area_), h->cog_ //,
//       double angle = 0, double slant = 0,
//       int color_id = 1, int font_id = 1,
//       double scale = 0.2
    ));
//  context->Load(aisGLabel);

  double
   Lx=h->bb_pmax_(0)-h->bb_pmin_(0),
   Ly=h->bb_pmax_(1)-h->bb_pmin_(1),
   Lz=h->bb_pmax_(2)-h->bb_pmin_(2);

  gp_Pnt p11(h->bb_pmin_(0), h->bb_pmax_(1), h->bb_pmax_(2));
  gp_Pnt p12(h->bb_pmax_(0), h->bb_pmax_(1), h->bb_pmax_(2));
  Handle_AIS_InteractiveObject aisDimX(createLengthDimension(
    BRepBuilderAPI_MakeVertex(p11),
    BRepBuilderAPI_MakeVertex(p12),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(h->bb_pmax_), gp_Vec(0,0,1)))),
    Lx,
    ""
//     str(format("Lx = %g (%g to %g)")%Lx%bb_pmin_(0)%bb_pmax_(0)).c_str()
  ));
//  context->Load(aisDimX);
  Handle_AIS_InteractiveObject aisDimXLabel(new InteractiveText
    (
      str(format("Lx = %g (%g to %g)")%Lx%h->bb_pmin_(0)%h->bb_pmax_(0)).c_str(),
      //vec3(0.5*(p11.XYZ()+p12.XYZ()))
     vec3(p11)
    ));
//  context->Load(aisDimXLabel);

  gp_Pnt p21(h->bb_pmax_(0), h->bb_pmin_(1), h->bb_pmax_(2));
  gp_Pnt p22(h->bb_pmax_(0), h->bb_pmax_(1), h->bb_pmax_(2));
  Handle_AIS_InteractiveObject aisDimY(createLengthDimension(
    BRepBuilderAPI_MakeVertex(p21),
    BRepBuilderAPI_MakeVertex(p22),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(h->bb_pmax_), gp_Vec(0,0,-1)))),
    Ly,
    "" //str(format("Ly = %g (%g to %g)")%Ly%bb_pmin_(1)%bb_pmax_(1)).c_str()
  ));
//  context->Load(aisDimY);
  Handle_AIS_InteractiveObject aisDimYLabel(new InteractiveText
    (
      str(format("Ly = %g (%g to %g)")%Ly%h->bb_pmin_(1)%h->bb_pmax_(1)).c_str(),
      //vec3(0.5*(p11.XYZ()+p12.XYZ()))
     vec3(p21)
    ));
//  context->Load(aisDimYLabel);

  gp_Pnt p31(h->bb_pmax_(0), h->bb_pmax_(1), h->bb_pmin_(2));
  gp_Pnt p32(h->bb_pmax_(0), h->bb_pmax_(1), h->bb_pmax_(2));
  Handle_AIS_InteractiveObject aisDimZ(createLengthDimension(
    BRepBuilderAPI_MakeVertex(p31),
    BRepBuilderAPI_MakeVertex(p32),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(h->bb_pmax_), gp_Vec(-1,1,0)))),
    Lz,
    "" //str(format("Lz = %g (%g to %g)")%Lz%bb_pmin_(2)%bb_pmax_(2)).c_str()
  ));
//  context->Load(aisDimZ);
  Handle_AIS_InteractiveObject aisDimZLabel(new InteractiveText
    (
      str(format("Lz = %g (%g to %g)")%Lz%h->bb_pmin_(2)%h->bb_pmax_(2)).c_str(),
      //vec3(0.5*(p11.XYZ()+p12.XYZ()))
     vec3(p31)
    ));
//  context->Load(aisDimZLabel);

  Handle_AIS_MultipleConnectedInteractive ais(new AIS_MultipleConnectedInteractive());

//  context->Load(ais);

  ais->Connect(aisG);
  ais->Connect(aisGLabel);
  ais->Connect(aisDimX);
  ais->Connect(aisDimXLabel);
  ais->Connect(aisDimY);
  ais->Connect(aisDimYLabel);
  ais->Connect(aisDimZ);
  ais->Connect(aisDimZLabel);

  return ais;
}


addStandaloneFunctionToStaticFunctionTable(
    PostProcActionVisualizers,
    SolidProperties,
    createAISReprByTypeName,
    SolidProperties_createAISRepr
    );

} // namespace cad
} // namespace insight
