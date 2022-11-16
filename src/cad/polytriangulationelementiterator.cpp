#include "polytriangulationelementiterator.h"

namespace insight {
namespace OCC {

bool operator== (const PolyTriangulationElementIterator& a, const PolyTriangulationElementIterator& b)
{
    return a.pt_ == b.pt_ && a.i_==b.i_;
};

bool operator!= (const PolyTriangulationElementIterator& a, const PolyTriangulationElementIterator& b)
{
    return !operator==(a, b);
};


PolyTriangulationElementIterator::PolyTriangulationElementIterator()
    : i_(0)
{}

PolyTriangulationElementIterator::PolyTriangulationElementIterator(Handle_Poly_Triangulation pt, Standard_Integer i)
    : pt_(pt), i_(i)
{}

const PolyTriangulationElementIterator::value_type& PolyTriangulationElementIterator::operator*() const
{
#if (OCC_VERSION_MAJOR>6)
    return pt_->Triangle(i_);
#else
    return pt_->Triangles().Value(i_);
#endif
}

const PolyTriangulationElementIterator::value_type* PolyTriangulationElementIterator::operator->() const
{
#if (OCC_VERSION_MAJOR>6)
    return &pt_->Triangle(i_);
#else
    return &pt_->Triangles().Value(i_);
#endif
}

PolyTriangulationElementIterator PolyTriangulationElementIterator::operator++(int)
{
    PolyTriangulationElementIterator tmp = *this;
    ++(*this);
    return tmp;
}

PolyTriangulationElementIterator& PolyTriangulationElementIterator::operator++()
{
    if ( !pt_.IsNull() && (i_<pt_->NbTriangles()) )
    {
        i_ += 1;
    }
    else
    {
        i_=0;
        pt_.Nullify();
    }
    return *this;
}




} // namespace OCC
} // namespace insight
