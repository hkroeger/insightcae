#ifndef INSIGHT_CAD_EDGECONNECTINGVERTICES_H
#define INSIGHT_CAD_EDGECONNECTINGVERTICES_H


#include "featurefilter.h"


namespace insight {
namespace cad {

class EdgeConnectingVertices
 : public Filter
{
protected:
    FeatureID v0_, v1_;

public:
    EdgeConnectingVertices(FeatureID v0, FeatureID v1);
    bool checkMatch(FeatureID feature) const override;

    FilterPtr clone() const override;
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_EDGECONNECTINGVERTICES_H
