#ifndef INSIGHT_OCC_POLYTRIANGLEITERATOR_H
#define INSIGHT_OCC_POLYTRIANGLEITERATOR_H

#include "Poly_Triangulation.hxx"

namespace insight {
namespace OCC {

class PolyTriangulationElementIterator;

bool operator== (const PolyTriangulationElementIterator& a, const PolyTriangulationElementIterator& b);
bool operator!= (const PolyTriangulationElementIterator& a, const PolyTriangulationElementIterator& b);

class PolyTriangulationElementIterator
{
    // Iterator tags here...
    using difference_type   = std::ptrdiff_t;
    using value_type        = Poly_Triangle;
    using pointer           = Poly_Triangle*;  // or also value_type*
    using reference         = Poly_Triangle&;  // or also value_type&

    Handle_Poly_Triangulation pt_;
    Standard_Integer i_;

public:
    PolyTriangulationElementIterator();
    PolyTriangulationElementIterator(Handle_Poly_Triangulation pt, Standard_Integer i=1);

    const value_type& operator*() const;
    const value_type* operator->() const;

    PolyTriangulationElementIterator operator++(int);
    PolyTriangulationElementIterator& operator++();

    friend bool operator== (const PolyTriangulationElementIterator& a, const PolyTriangulationElementIterator& b);
    friend bool operator!= (const PolyTriangulationElementIterator& a, const PolyTriangulationElementIterator& b);
};

} // namespace OCC
} // namespace insight

#endif // INSIGHT_OCC_POLYTRIANGLEITERATOR_H
