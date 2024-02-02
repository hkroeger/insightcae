#ifndef INSIGHT_CAD_SAMEENTITY_H
#define INSIGHT_CAD_SAMEENTITY_H

#include "cadfeature.h"
#include "feature.h"
#include "base/exception.h"


namespace insight {
namespace cad {

template<EntityType T, class TopoDS_Type>
class same
: public Filter
{
protected:
    FeatureSetPtr f_; // to match

    mutable std::vector<TopoDS_Type> unmatched_;

    inline const TopoDS_Type& other(FeatureID j) const;

public:
    same(FeaturePtr m)
        : f_(m->allOf(T))
    {}

    same(FeatureSet f)
    : f_(std::make_shared<FeatureSet>(f))
    {}

    void initialize(ConstFeaturePtr m) override
    {
        Filter::initialize(m);
        unmatched_.clear();
        std::transform(
            f_->data().begin(), f_->data().end(),
            std::back_inserter(unmatched_),
            [&](FeatureID j)
            {
                return TopoDS_Type{other(j)};
            }
            );
    }

    bool checkMatch(FeatureID i) const override
    {
        throw insight::Exception("not overriden");
        return false;
    }

    FilterPtr clone() const override
    {
        return std::make_shared<same>(*f_);
    }

};

template<> const TopoDS_Vertex& same<Vertex, TopoDS_Vertex>::other(FeatureID feature) const;
template<> bool same<Vertex, TopoDS_Vertex>::checkMatch(FeatureID feature) const;

template<> const TopoDS_Edge& same<Edge, TopoDS_Edge>::other(FeatureID feature) const;
template<> bool same<Edge, TopoDS_Edge>::checkMatch(FeatureID feature) const;

template<> const TopoDS_Face& same<Face, TopoDS_Face>::other(FeatureID feature) const;
template<> bool same<Face, TopoDS_Face>::checkMatch(FeatureID feature) const;

template<> const TopoDS_Solid& same<Solid, TopoDS_Solid>::other(FeatureID feature) const;
template<> bool same<Solid, TopoDS_Solid>::checkMatch(FeatureID feature) const;

typedef same<Vertex, TopoDS_Vertex> sameVertex;
typedef same<Edge, TopoDS_Edge> sameEdge;
typedef same<Face, TopoDS_Face> sameFace;
typedef same<Solid, TopoDS_Solid> sameSolid;

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SAMEENTITY_H
