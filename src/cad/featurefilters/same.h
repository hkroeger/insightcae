#ifndef INSIGHT_CAD_SAMEENTITY_H
#define INSIGHT_CAD_SAMEENTITY_H

#include "cadfeature.h"
#include "feature.h"
#include "base/exception.h"


namespace insight {
namespace cad {

template<EntityType T>
class same
: public Filter
{
protected:
    FeatureSetPtr f_;

public:
    same(FeaturePtr m)
        : f_(m->allOf(T))
    {}

    same(FeatureSet f)
    : f_(std::make_shared<FeatureSet>(f))
    {}

    bool checkMatch(FeatureID i) const
    {
        throw insight::Exception("not overriden");
        return false;
    }

    FilterPtr clone() const
    {
        return std::make_shared<same>(*f_);
    }

};

template<> bool same<Vertex>::checkMatch(FeatureID feature) const;
template<> bool same<Edge>::checkMatch(FeatureID feature) const;
template<> bool same<Face>::checkMatch(FeatureID feature) const;
template<> bool same<Solid>::checkMatch(FeatureID feature) const;

typedef same<Vertex> sameVertex;
typedef same<Edge> sameEdge;
typedef same<Face> sameFace;
typedef same<Solid> sameSolid;

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SAMEENTITY_H
