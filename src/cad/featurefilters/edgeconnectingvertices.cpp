#include "edgeconnectingvertices.h"

namespace insight {
namespace cad {

EdgeConnectingVertices::EdgeConnectingVertices(FeatureID v0, FeatureID v1)
    : v0_(v0), v1_(v1)
{}


EdgeConnectingVertices::EdgeConnectingVertices(const FeatureSet& v0, const FeatureSet& v1)
{
    insight::assertion(
        v0.shape()==Vertex,
        "Expected vertex set for first vertex! Got type %d.", v0.shape() );
    insight::assertion(
        v1.shape()==Vertex,
        "Expected vertex set for second vertex! Got type %d.", v1.shape() );

    insight::assertion(
        v0.size()==1,
        "Expected vertex set of size 1 for first vertex! Got size %d.", v0.size() );
    insight::assertion(
        v1.size()==1,
        "Expected vertex set of size 1 for second vertex! Got size %d.", v1.size() );

    v0_=*v0.data().begin();
    v1_=*v1.data().begin();
}

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
