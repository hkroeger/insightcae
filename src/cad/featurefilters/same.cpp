
#include "same.h"

namespace insight {
namespace cad {




template<>
const TopoDS_Vertex& same<Vertex,TopoDS_Vertex>::other(FeatureID j) const
{
    return f_->model()->vertex(j);
}


template<>
bool same<Vertex, TopoDS_Vertex>::checkMatch(FeatureID i) const
{
    auto e1 = model_->vertex(i);
    for (auto j = unmatched_.begin(); j!=unmatched_.end(); ++j)
    {
        if (e1.IsEqual(*j))
        {
            unmatched_.erase(j);
            return true;
        }
    }

    return false;
}




template<>
const TopoDS_Edge& same<Edge,TopoDS_Edge>::other(FeatureID j) const
{
    return f_->model()->edge(j);
}


template<>
bool same<Edge, TopoDS_Edge>::checkMatch(FeatureID i) const
{
    auto &e1 = model_->edge(i);
    for (auto j = unmatched_.begin(); j!=unmatched_.end(); ++j)
    {
        if (e1.IsEqual(*j))
        {
            unmatched_.erase(j);
            return true;
        }
    }

    return false;
}




template<>
const TopoDS_Face& same<Face,TopoDS_Face>::other(FeatureID j) const
{
    return f_->model()->face(j);
}


template<>
bool same<Face,TopoDS_Face>::checkMatch(FeatureID i) const
{
    auto &e1 = model_->face(i);
    for (auto j = unmatched_.begin(); j!=unmatched_.end(); ++j)
    {
      if (e1.IsEqual(*j))
      {
          unmatched_.erase(j);
          return true;
      }
    }

    return false;
}




template<>
const TopoDS_Solid& same<Solid,TopoDS_Solid>::other(FeatureID j) const
{
    return f_->model()->subsolid(j);
}


template<>
bool same<Solid, TopoDS_Solid>::checkMatch(FeatureID i) const
{
    auto &e1 = model_->subsolid(i);
    for (auto j = unmatched_.begin(); j!=unmatched_.end(); ++j)
    {
        if (e1.IsEqual(*j))
        {
            unmatched_.erase(j);
            return true;
        }
    }

    return false;
}




} // namespace cad
} // namespace insight
