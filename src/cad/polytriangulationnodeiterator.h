#ifndef INSIGHT_OCC_POLYTRIANGULATIONNODEITERATOR_H
#define INSIGHT_OCC_POLYTRIANGULATIONNODEITERATOR_H

#include "Poly_Triangulation.hxx"
#include "TopLoc_Location.hxx"

namespace insight {
namespace OCC {

class PolyTriangulationNodeIterator;

bool operator== (const PolyTriangulationNodeIterator& a, const PolyTriangulationNodeIterator& b);
bool operator!= (const PolyTriangulationNodeIterator& a, const PolyTriangulationNodeIterator& b);

class PolyTriangulationNodeIterator
{
    // Iterator tags here...
    using difference_type   = std::ptrdiff_t;
    using value_type        = gp_Pnt;
    using pointer           = gp_Pnt*;  // or also value_type*
    using reference         = gp_Pnt&;  // or also value_type&

    Handle_Poly_Triangulation pt_;
    Standard_Integer i_;

public:
    PolyTriangulationNodeIterator();
    PolyTriangulationNodeIterator(Handle_Poly_Triangulation pt, Standard_Integer i=1);

    const value_type& operator*() const;
    const value_type* operator->() const;

    PolyTriangulationNodeIterator operator++(int);
    PolyTriangulationNodeIterator& operator++();

    friend bool operator== (const PolyTriangulationNodeIterator& a, const PolyTriangulationNodeIterator& b);
    friend bool operator!= (const PolyTriangulationNodeIterator& a, const PolyTriangulationNodeIterator& b);
};

} // namespace OCC
} // namespace insight

#endif // INSIGHT_OCC_POLYTRIANGULATIONNODEITERATOR_H
