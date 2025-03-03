#ifndef INSIGHT_BLOCKMESHVISUALIZATION_H
#define INSIGHT_BLOCKMESHVISUALIZATION_H

#include "TopoDS_Edge.hxx"
#include "TopoDS_Shape.hxx"
#include "openfoam/blockmesh.h"

namespace insight {

class CADParameterSetModelVisualizer;

namespace bmd
{

typedef std::pair<insight::bmd::Point, insight::bmd::Point> PointPair;

class blockMeshVisualization
        : public std::vector<TopoDS_Shape>
{
    const bmd::blockMesh& bm_;
    std::set<bmd::Point> includedVertices;
    std::set<bmd::PointPair> includedEdges;

    TopoDS_Edge addEdge(const arma::mat& p0, const arma::mat& p1);

public:
    blockMeshVisualization(const bmd::blockMesh& bm);

    void addToVisualizer(
        CADParameterSetModelVisualizer& vz,
        const std::string& prefix,
        const boost::variant<double,gp_Trsf>& s_or_t = 1.0 ) const;
};

}

} // namespace insight


namespace std
{

bool operator<( const insight::bmd::PointPair& p1, const insight::bmd::PointPair& p2);

}


#endif // INSIGHT_BLOCKMESHVISUALIZATION_H
