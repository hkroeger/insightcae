#include "projectededge.h"

namespace insight {
namespace bmd {



ProjectedEdge::ProjectedEdge(const Point& c0, const Point& c1, const std::string& geometryLabel)
    : Edge(c0, c1),
      geometryLabel_(geometryLabel)
{}

std::vector<OFDictData::data> ProjectedEdge::bmdEntry(const PointMap& allPoints, int OFversion) const
{
    return {
        "project",
        allPoints.find(c0_)->second, allPoints.find(c1_)->second,
        "("+geometryLabel_+")"
    };
}

Edge* ProjectedEdge::transformed(const arma::mat& tm, const arma::mat trans) const
{
    return new ProjectedEdge(tm*c0_+trans, tm*c1_+trans, geometryLabel_);
}

Edge* ProjectedEdge::clone() const
{
    return new ProjectedEdge(c0_, c1_, geometryLabel_);
}


} // namespace bmd
} // namespace insight
