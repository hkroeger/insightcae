#include "control.h"
#include "base/exception.h"
#include "boost/units/systems/cgs.hpp"

#include "lsdyna/lsdynainputdeck.h"

namespace insight {
namespace LSDynaInputCards {




Control_Units::Control_Units()
    : lengthUnit_(si::millimeter*1),
      timeUnit_(si::second*1),
      massUnit_(si::metric_ton*1)
{}




void Control_Units::write(std::ostream &os) const
{
    std::string l, t, m;
    if (fabs(toValue(lengthUnit_, si::meter)-1.)<SMALL)
    {
        l="m";
    }
    else if (fabs(toValue(lengthUnit_, si::millimeter)-1.)<SMALL)
    {
        l="mm";
    }
    else if (fabs(toValue(lengthUnit_, si::centi*si::meter)-1.)<SMALL)
    {
        l="cm";
    }
    else
        throw insight::Exception("Unsupported length unit!");

    if (fabs(toValue(timeUnit_, si::seconds)-1.)<SMALL)
    {
        t="s";
    }
    else if (fabs(toValue(timeUnit_, si::milli*si::second)-1.)<SMALL)
    {
        t="ms";
    }
    else if (fabs(toValue(timeUnit_, si::micro*si::second)-1.)<SMALL)
    {
        t="micro_s";
    }
    else
        throw insight::Exception("Unsupported time unit!");

    if (fabs(toValue(massUnit_, si::kilogram)-1.)<SMALL)
    {
        m="kg";
    }
    else if (fabs(toValue(massUnit_, boost::units::cgs::gram)-1.)<SMALL)
    {
        m="g";
    }
    else if (fabs(toValue(massUnit_, si::milli*boost::units::cgs::gram)-1.)<SMALL)
    {
        m="mg";
    }
    else if (fabs(toValue(massUnit_, si::metric_ton)-1.)<SMALL)
    {
        m="mtrc_ton";
    }
    else
        throw insight::Exception("Unsupported mass unit!");

    os << "*CONTROL_UNITS\n";
    os << str(boost::format("%10s%10s%10s\n")%l%t%m);
    os << "\n";
}





Control_Termination::Control_Termination(
        si::Time endTime,
        OptionalValue endEnergyError,
        OptionalValue endMassError
        )
    : endTime_(endTime),
      endEnergyError_(endEnergyError),
      endMassError_(endMassError)
{
}

void Control_Termination::write(std::ostream& os) const
{
    os << "*CONTROL_TERMINATION\n"

       << toValue(endTime_, inputDeck().timeUnit()) << ",,,";
    if (const auto *err = boost::get<double>(&endEnergyError_))
    {
        os << (*err);
    }
    if (const auto *err = boost::get<double>(&endMassError_))
    {
        os << ", " << (*err);
    }
    os << "\n";
}





Control_Timestep::Control_Timestep(double DtScalingCoeff, const si::Time& minDt)
    : DtScalingCoeff_(DtScalingCoeff),
      minDt_(minDt)
{}

void Control_Timestep::write(std::ostream& os) const
{
    os << "*CONTROL_TIMESTEP\n";
    os << ", "<<DtScalingCoeff_<<",,, "<<toValue(-minDt_, inputDeck().timeUnit())<<"\n";
}




Damping_Global::Damping_Global(double dampingCoeff)
    : dampingCoeff_(dampingCoeff)
{}

void Damping_Global::write(std::ostream& os) const
{
    os << "*DAMPING_GLOBAL\n";
    os << ", "<< dampingCoeff_ <<"\n";
}



Control_Output::Control_Output(
        si::Time outputDt,
        bool bndout,
        bool glstat,
        bool rcforc,
        bool binaryd3 )
    : outputDt_(outputDt),
      bndout_(bndout),
      glstat_(glstat),
      rcforc_(rcforc),
      binaryd3_(binaryd3)
{}

void Control_Output::write(std::ostream& os) const
{
    if (bndout_)
    {
        os << "*DATABASE_BNDOUT\n"
           << toValue(outputDt_, inputDeck().timeUnit()) << "\n";
    }
    if (glstat_)
    {
        os << "*DATABASE_GLSTAT\n"
           << toValue(outputDt_, inputDeck().timeUnit()) << "\n";
    }
    if (rcforc_)
    {
        os << "*DATABASE_RCFORC\n"
           << toValue(outputDt_, inputDeck().timeUnit()) << "\n";
    }
    if (binaryd3_)
    {
        os << "*DATABASE_BINARY_D3PLOT\n"
           << toValue(outputDt_, inputDeck().timeUnit()) << "\n";
    }
}




} // namespace LSDynaInputCards
} // namespace insight
