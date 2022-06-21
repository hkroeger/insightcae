#ifndef INSIGHT_INTERVALS_H
#define INSIGHT_INTERVALS_H

#include <map>
#include <memory>



namespace insight {




class Interval;
class OverlappingIntervals;




std::ostream &operator<<(std::ostream& os, const Interval&);




class Interval
{
    friend std::ostream& operator<<(std::ostream& os, const Interval&);

    double a_, b_;
    double clipRightOf_;

public:
    Interval(double start, double end);
    virtual ~Interval();

    inline double A() const { return a_; }
    inline double B() const { return b_; }

    inline double clippedB() const { return clipRightOf_; }

    void clipRightOf(double x);

    bool toBeIgnored() const;
    bool isClipped() const;

    void resetClip();
};

typedef std::shared_ptr<Interval> IntervalPtr;




std::ostream& operator<<(std::ostream& os, const OverlappingIntervals&);

std::ostream &operator<<(std::ostream& os, IntervalPtr);



class OverlappingIntervals
{
    friend std::ostream& operator<<(std::ostream& os, const OverlappingIntervals&);

public:
    typedef std::map<double, IntervalPtr> IntervalList;

private:
    IntervalList intervals_;

public:
    OverlappingIntervals(const IntervalList& intervals = IntervalList());

    void insert(double priority, IntervalPtr);

    void clipIntervals();

    inline const IntervalList& intervals() const { return intervals_; }
};




} // namespace insight

#endif // INSIGHT_INTERVALS_H
