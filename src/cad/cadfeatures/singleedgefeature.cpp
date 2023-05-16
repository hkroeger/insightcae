#include "singleedgefeature.h"

namespace insight {
namespace cad {



SingleEdgeFeature::SingleEdgeFeature()
{}

SingleEdgeFeature::SingleEdgeFeature(TopoDS_Edge e)
    : Feature(e)
{}

SingleEdgeFeature::SingleEdgeFeature(FeatureSetPtr creashapes)
    : Feature(creashapes)
{}

VectorPtr SingleEdgeFeature::start() const
{
    checkForBuildDuringAccess();
    return matconst(refpoints_.at("p0"));
}

VectorPtr SingleEdgeFeature::end() const
{
    checkForBuildDuringAccess();
    return matconst(refpoints_.at("p1"));
}

void SingleEdgeFeature::setShape(const TopoDS_Shape &shape)
{
    Feature::setShape(shape);

    auto e=TopoDS::Edge(shape);

    BRepAdaptor_Curve crv(e);

    gp_Pnt p;
    gp_Vec v;
    crv.D1(crv.FirstParameter(), p, v);
    refpoints_["p0"]=vec3(p);
    refvectors_["et0"]=vec3(v.Normalized());
    crv.D1(crv.LastParameter(), p, v);
    refpoints_["p1"]=vec3(p);
    refvectors_["et1"]=vec3(v.Normalized());
}

bool SingleEdgeFeature::isSingleEdge() const
{
    return true;
}


bool SingleEdgeFeature::isSingleOpenWire() const
{
  return true;
}

} // namespace cad
} // namespace insight
