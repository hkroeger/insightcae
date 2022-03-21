#include "ispartofface.h"
#include "cadfeature.h"

#include "occtools.h"
#include "geotest.h"

using namespace std;
using namespace boost;

namespace insight {
namespace cad {




template<> isPartOfFace<Edge>::isPartOfFace(FeaturePtr m)
: f_(TopoDS::Face(m->shape()))
{}




template<>
bool isPartOfFace<Edge>::checkMatch(FeatureID feature) const
{
  bool match=false;

  TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));
  match |= isPartOf(f_, e1);

  return match;
}





template<> isPartOfFace<Face>::isPartOfFace(FeaturePtr m)
: f_(TopoDS::Face(m->shape()))
{}




template<>
bool isPartOfFace<Face>::checkMatch(FeatureID feature) const
{
  bool match=false;

//   for (int f: f_)
  {
    TopoDS_Face f1=TopoDS::Face(model_->face(feature));
    match |= isPartOf(f_, f1);
  }

  return match;
}




} // namespace cad
} // namespace insight
