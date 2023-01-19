
#include "same.h"

namespace insight {
namespace cad {


template<>
bool same<Vertex>::checkMatch(FeatureID i) const
{
    auto e1 = model_->vertex(i);
    for (int j: f_->data())
    {
      auto e2 = f_->model()->vertex(j);
      if (e1.IsEqual(e2)) return true;
    }

    return false;
}


template<>
bool same<Edge>::checkMatch(FeatureID i) const
{
    auto e1 = model_->edge(i);
    for (int j: f_->data())
    {
      auto e2 = f_->model()->edge(j);
      if (e1.IsEqual(e2)) return true;
    }

    return false;
}


template<>
bool same<Face>::checkMatch(FeatureID i) const
{
    auto e1 = model_->face(i);
    for (int j: f_->data())
    {
      auto e2 = f_->model()->face(j);
      if (e1.IsEqual(e2)) return true;
    }

    return false;
}


template<>
bool same<Solid>::checkMatch(FeatureID i) const
{
    auto e1 = model_->subsolid(i);
    for (int j: f_->data())
    {
      auto e2 = f_->model()->subsolid(j);
      if (e1.IsEqual(e2)) return true;
    }

    return false;
}


} // namespace cad
} // namespace insight
