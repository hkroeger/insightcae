
#include "base/exception.h"
#include "base/intervals.h"
#include "base/boost_include.h"

namespace insight {




std::ostream &operator<<(std::ostream& os, const Interval& i)
{
    if (i.toBeIgnored())
    {
        os << "( completely ignored, "<<i.a_<<"..."<<i.b_<<" )";
    }
    else if (i.isClipped())
    {
        os << "clipped to " << i.a_ << "..." << i.clipRightOf_
           << " ( " << i.a_ << "..." << i.b_ << " )";
    }
    else
    {
        os << i.a_<<"..."<<i.b_<<" (not clipped)";
    }
    return os;
}

std::ostream &operator<<(std::ostream& os, IntervalPtr ip)
{
    os << *ip;
    return os;
}


Interval::Interval(double start, double end)
    : a_(start), b_(end)
{
    insight::assertion(
                b_>a_,
                str(boost::format(
                        "invalid interval definition: begin (%g) is after end(%g)!"
                        ) % a_ % b_) );
    resetClip();
}




Interval::~Interval()
{}




void Interval::clipRightOf(double x)
{
    clipRightOf_=x;
}




bool Interval::toBeIgnored() const
{
    return clipRightOf_<a_;
}




bool Interval::isClipped() const
{
    return clipRightOf_<b_;
}




void Interval::resetClip()
{
    clipRightOf_=b_;
}




std::ostream& operator<<(std::ostream& os, const OverlappingIntervals& oi)
{
    for (const auto&i : oi.intervals_)
    {
        os << i.first << ":\t " << i.second << std::endl;
    }
    return os;
}




OverlappingIntervals::OverlappingIntervals(const IntervalList &intervals)
    : intervals_(intervals)
{}

void OverlappingIntervals::insert(double priority, IntervalPtr ip)
{
    intervals_[priority]=ip;
}




void OverlappingIntervals::clipIntervals()
{
    for (
         auto i=intervals_.rbegin();
         i!=intervals_.rend();
         ++i )
    {
        if (!i->second->toBeIgnored())
        {
            double clipAt=i->second->A();
            auto j=i;
            for (
                 ++j;
                 j!=intervals_.rend();
                 ++j )
            {
                j->second->clipRightOf(clipAt);
            }
        }
    }
}



} // namespace insight
