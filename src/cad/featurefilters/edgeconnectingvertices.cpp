#include "edgeconnectingvertices.h"

namespace insight {
namespace cad {

EdgeConnectingVertices::EdgeConnectingVertices(FeatureID v0, FeatureID v1)
    : v0_(v0), v1_(v1)
{}


bool EdgeConnectingVertices::checkMatch(FeatureID feature) const
{
    auto edge = model_->edge(feature);
    auto i0 = model_->vertexID(TopExp::FirstVertex(edge));
    auto i1 = model_->vertexID(TopExp::LastVertex(edge));
    return (
     (v0_==i0 && v1_==i1) ||
     (v0_==i1 && v1_==i0)
    );
}

FilterPtr EdgeConnectingVertices::clone() const
{
    return std::make_shared<EdgeConnectingVertices>(v0_, v1_);
}


} // namespace cad
} // namespace insight
