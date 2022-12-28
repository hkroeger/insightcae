#include "polytriangulationnodeiterator.h"

namespace insight {
namespace OCC {


bool operator== (const PolyTriangulationNodeIterator& a, const PolyTriangulationNodeIterator& b)
{
    return a.pt_ == b.pt_ && a.i_==b.i_;
};

bool operator!= (const PolyTriangulationNodeIterator& a, const PolyTriangulationNodeIterator& b)
{
    return !operator==(a, b);
};


PolyTriangulationNodeIterator::PolyTriangulationNodeIterator()
    : i_(0)
{}

PolyTriangulationNodeIterator::PolyTriangulationNodeIterator(
        Handle_Poly_Triangulation pt,
        Standard_Integer i )
    : pt_(pt), i_(i)
{}

const PolyTriangulationNodeIterator::value_type&
PolyTriangulationNodeIterator::operator*() const
{
#if (OCC_VERSION_MAJOR>6)
    return pt_->Node(i_);
#else
    return pt_->Nodes().Value(i_);
#endif
}

const PolyTriangulationNodeIterator::value_type*
PolyTriangulationNodeIterator::operator->() const
{
#if (OCC_VERSION_MAJOR>6)
    return &pt_->Node(i_);
#else
    return &pt_->Nodes().Value(i_);
#endif
}

PolyTriangulationNodeIterator PolyTriangulationNodeIterator::operator++(int)
{
    PolyTriangulationNodeIterator tmp = *this;
    ++(*this);
    return tmp;
}

PolyTriangulationNodeIterator& PolyTriangulationNodeIterator::operator++()
{
    if ( !pt_.IsNull() && (i_ < pt_->NbNodes()) )
    {
        i_ +=  1;
    }
    else
    {
        i_ = 0;
        pt_.Nullify();
    }
    return *this;
}



} // namespace OCC
} // namespace insight
