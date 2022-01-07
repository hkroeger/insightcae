#include "connected.h"
#include "cadfeature.h"

#include "occtools.h"
#include "geotest.h"

#include "TopOpeBRepBuild_Tools.hxx"

using namespace std;
using namespace boost;

namespace insight
{
namespace cad
{




template<> connected<Face>::connected(FeaturePtr m)
: f_(m->allFaces())
{
}


class NeighbourhoodExplorer
{
  const Feature& s_;
  TopTools_IndexedDataMapOfShapeListOfShape edgeFaces_/*, faceEdges_*/;

public:
  NeighbourhoodExplorer(const Feature& s)
    : s_(s)
  {
#if OCC_VERSION_MAJOR<7
    TopExp::MapShapesAndAncestors
#else
    TopExp::MapShapesAndUniqueAncestors
#endif
        (
          s_.shape(),
          TopAbs_EDGE, TopAbs_FACE,
          edgeFaces_
    );
//    TopExp::MapShapesAndUniqueAncestors(
//          s_.shape(),
//          TopAbs_FACE, TopAbs_EDGE,
//          faceEdges_
//    );
  }

  bool addNeighbours(const TopoDS_Shape& f, std::set<FeatureID>& result)
  {
    bool wasAdded=false;

    result.insert(s_.faceID(f)); // add current face

    for (TopExp_Explorer ex(f, TopAbs_EDGE); ex.More(); ex.Next())
    {
      TopoDS_Edge b = TopoDS::Edge(ex.Current());

      TopoDS_Face bn;
      if (TopOpeBRepBuild_Tools::GetAdjacentFace(f, b, edgeFaces_, bn))
      {
        FeatureID fi = s_.faceID(bn);
        if (result.find(fi)==result.end()) // if not yet visited
        {
          addNeighbours(bn, result);
          wasAdded=true;
        }
      }
    }

//    const auto& borders=faceEdges_.FindFromKey(f);
//    for (const auto& b: borders)
//    {
//      const auto& border_adjacents = edgeFaces_.FindFromKey(b);
//      for (const auto& bn: border_adjacents)
//      {
//        FeatureID fi = s_.faceID(bn);
//        if (result.find(fi)==result.end()) // if not yet visited
//        {
//          cout<<"add"<<endl;
//          addNeighbours(bn, result);
//        }
//        else
//          cout<<"not added"<<endl;
//      }
//    }

    return wasAdded;
  }
};


template<>
void connected<Face>::initialize(ConstFeaturePtr m)
{
  Filter::initialize(m);

  if (model_ != f_->model())
    throw insight::Exception("Can only check connections within the same feature!");

  NeighbourhoodExplorer nex(*f_->model());

  for (const auto& sfi: f_->data())
  {
    cout<<"adding neighbours of face #"<<sfi<<endl;
    nex.addNeighbours(f_->model()->face(sfi), selected_feats_);
  }
}

template<>
bool connected<Face>::checkMatch(FeatureID feature) const
{
  return selected_feats_.find(feature) != selected_feats_.end();
}

}
}

