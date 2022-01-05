#ifndef INSIGHT_CAD_ISPARTOFFACE_H
#define INSIGHT_CAD_ISPARTOFFACE_H

#include "cadfeature.h"
#include "feature.h"
#include "base/exception.h"


namespace insight {
namespace cad {

template<EntityType T>
class isPartOfFace
    : public Filter
{
protected:
    TopoDS_Face f_;

public:
    isPartOfFace(const TopoDS_Face& f)
    : f_(f)
    {}

    isPartOfFace(FeaturePtr m)
      : f_(TopoDS::Face(m->shape()))
    {
        throw insight::Exception("isPartOfFace filter: not implemented!");
    }

    isPartOfFace(FeatureSet f)
        : f_(TopoDS::Face(f.model()->shape()))
    {}

    bool checkMatch(FeatureID) const
    {
        throw insight::Exception("isPartOfFace filter: not implemented!");
    }

    FilterPtr clone() const
    {
        return FilterPtr(new isPartOfFace(f_));
    }

};

template<> isPartOfFace<Edge>::isPartOfFace(FeaturePtr m);
template<> bool isPartOfFace<Edge>::checkMatch(FeatureID feature) const;
template<> isPartOfFace<Face>::isPartOfFace(FeaturePtr m);
template<> bool isPartOfFace<Face>::checkMatch(FeatureID feature) const;

typedef isPartOfFace<Edge> isPartOfFaceEdge;
typedef isPartOfFace<Face> isPartOfFaceFace;

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_ISPARTOFFACE_H
