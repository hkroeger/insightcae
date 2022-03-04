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



class FrameMesh
: public insight::cad::PostprocAction
{
protected:
    boost::filesystem::path outpath_;
    ScalarPtr L_;
    EdgesDesc edges_;
//    FrameMeshGroupDefinitions v_e_groups_;

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
