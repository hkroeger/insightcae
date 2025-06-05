#ifndef INSIGHT_LSDYNAINPUTCARDS_CONTROL_H
#define INSIGHT_LSDYNAINPUTCARDS_CONTROL_H


#include "lsdyna/lsdynainputcard.h"
#include "base/units.h"

namespace insight {
namespace LSDynaInputCards {


class Control_Units
        : public LSDynaInputCard
{
public:
    si::Length lengthUnit_;
    si::Time timeUnit_;
    si::Mass massUnit_;

public:
    Control_Units();

    template<class L, class T, class M>
    Control_Units(L lengthUnit, T timeUnit, M massUnit)
        : lengthUnit_(1*lengthUnit),
          timeUnit_(1*timeUnit),
          massUnit_(1*massUnit)
    {}

    void write(std::ostream& os) const override;
};




class Control_Termination
        : public LSDynaInputCard
{
public:
    typedef boost::variant<boost::blank,double> OptionalValue;

    si::Time endTime_;
    OptionalValue endEnergyError_, endMassError_;

public:
    Control_Termination(
            si::Time endTime,
            OptionalValue endEnergyError = boost::blank(),
            OptionalValue endMassError = boost::blank()
            );

    void write(std::ostream& os) const override;
};




class Control_Timestep
        : public LSDynaInputCard
{
public:
    double DtScalingCoeff_;
    si::Time minDt_;

public:
    Control_Timestep(double DtScalingCoeff=0.9, const si::Time& minDt = 0);

    void write(std::ostream& os) const override;
};




class Damping_Global
    : public LSDynaInputCard
{
public:
    double dampingCoeff_;

public:
    Damping_Global(double dampingCoeff);

    void write(std::ostream& os) const override;
};



class Control_Output
        : public LSDynaInputCard
{
public:
    si::Time outputDt_;
    bool bndout_, glstat_, rcforc_, binaryd3_;

public:
    Control_Output(
            si::Time outputDt,
            bool bndout = true,
            bool glstat = true,
            bool rcforc = true,
            bool binaryd3 = true );

    void write(std::ostream& os) const override;
};


} // namespace LSDynaInputCards
} // namespace insight

#endif // INSIGHT_LSDYNAINPUTCARDS_CONTROL_H
