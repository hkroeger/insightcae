#ifndef INSIGHT_CAD_FRAMEMESH_H
#define INSIGHT_CAD_FRAMEMESH_H

#include "mesh.h"


namespace insight {
namespace cad {




//typedef boost::fusion::vector<
//    insight::cad::GroupsDesc, //vertexGroups,
//    insight::cad::GroupsDesc //edgeGroups
//  > FrameMeshGroupDefinitions;



typedef boost::fusion::vector2<
    FeaturePtr,
    boost::optional<boost::fusion::vector2<
        FeaturePtr,
        ScalarPtr > >
    > EdgeDesc;

typedef std::vector<EdgeDesc> EdgesDesc;



class FindIntersections
{
public:
    typedef std::pair<gp_Pnt, double> IntersectionLocation;
    typedef std::set<IntersectionLocation> IntersectionLocations;

private:
    IntersectionLocations intersectionsOnE1_, intersectionsOnE2_;
public:
    FindIntersections(const TopoDS_Edge& e1, const TopoDS_Edge& e2);

    const IntersectionLocations& intersectionsOnE1()
    { return intersectionsOnE1_; }

    const IntersectionLocations& intersectionsOnE2()
    { return intersectionsOnE2_; }

    static bool isOnEndpoint(const gp_Pnt& p, const TopoDS_Edge& e);

    static void append(const IntersectionLocations& newIts,
                       IntersectionLocations& its);
};




class FrameMesh
: public insight::cad::PostprocAction
{
public:
    typedef std::set<TopoDS_Edge> EdgeSet;

protected:
    boost::filesystem::path outpath_;
    ScalarPtr L_;
    EdgesDesc edges_;
//    FrameMeshGroupDefinitions v_e_groups_;

    EdgeSet modelEdges;
    EdgeSet meshEdges;
    std::map<TopoDS_Edge, std::pair<cad::FeaturePtr,double> > crossSectionModels;

    void splitEdge(
            const TopoDS_Edge& e,
            const FindIntersections::IntersectionLocations& spl
            );

  size_t calcHash() const override;
  void build() override;

public:
  declareType("FrameMesh");
  FrameMesh
  (
    const boost::filesystem::path& outpath,
    ScalarPtr L,
    EdgesDesc edges
//    FeaturePtr model,
//    const FrameMeshGroupDefinitions& v_e_groups
  );

   void write(std::ostream& ) const override;
};



} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_FRAMEMESH_H
