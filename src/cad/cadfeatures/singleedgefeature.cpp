#include "singleedgefeature.h"

#include "base/exception.h"
#include "base/tools.h"

namespace insight {
namespace cad {






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


ImportedSingleEdgeFeature::ImportedSingleEdgeFeature(TopoDS_Edge e)
    : importSource_(e)
{}

ImportedSingleEdgeFeature::ImportedSingleEdgeFeature(FeatureSetPtr creashapes)
    : importSource_(creashapes)
{}


size_t ImportedSingleEdgeFeature::calcHash() const
{
    ParameterListHash h;
    h+=this->type();

    if (auto *e=boost::get<TopoDS_Edge>(&importSource_))
    {
        h+=TopoDS_Shape(*e);
    }
    else if (auto *fs=boost::get<FeatureSetPtr>(&importSource_))
    {
        h+=**fs;
    }
    else
        throw insight::UnhandledSelection();

    return h.getHash();
}


void ImportedSingleEdgeFeature::build()
{
    ExecTimer t("Import::build() ["+featureSymbolName()+"]");

    if (!cache.contains(hash()))
    {

        if (auto *e=boost::get<TopoDS_Edge>(&importSource_))
        {
            setShape(*e);
        }
        else if (auto *fs=boost::get<FeatureSetPtr>(&importSource_))
        {
            auto& featSet=*fs;

            insight::assertion(
                featSet->shape()==Edge,
                "an edge type feature set is expected!");

            insight::assertion(
                featSet->size()==1,
                "a feature set containing a single edge is expected!");


            setShape(
                featSet->model()->edge(
                    *featSet->data().begin()
                )
            );
        }
        else
            throw insight::UnhandledSelection();


        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<ImportedSingleEdgeFeature>(hash()));
    }
}

} // namespace cad
} // namespace insight
