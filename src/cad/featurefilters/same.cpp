
#include "same.h"

namespace insight {
namespace cad {


template<>
bool same<Vertex>::checkMatch(FeatureID i) const
{
    bool match=false;

    for (int j: f_->data())
    {
      auto e1 = model_->vertex(i);
      auto e2 = f_->model()->vertex(j);
      match |= e1.IsEqual(e2);
    }

    return match;
}


template<>
bool same<Edge>::checkMatch(FeatureID i) const
{
    bool match=false;

    for (int j: f_->data())
    {
      auto e1 = model_->edge(i);
      auto e2 = f_->model()->edge(j);
      match |= e1.IsEqual(e2);
    }

    return match;
}


template<>
bool same<Face>::checkMatch(FeatureID i) const
{
    bool match=false;

    for (int j: f_->data())
    {
      auto e1 = model_->face(i);
      auto e2 = f_->model()->face(j);
      match |= e1.IsEqual(e2);
    }

    return match;
}


template<>
bool same<Solid>::checkMatch(FeatureID i) const
{
    bool match=false;

    for (int j: f_->data())
    {
      auto e1 = model_->subsolid(i);
      auto e2 = f_->model()->subsolid(j);
      match |= e1.IsEqual(e2);
    }

    return match;
}


} // namespace cad
} // namespace insight
