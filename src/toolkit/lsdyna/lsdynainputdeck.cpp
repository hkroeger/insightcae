#include "lsdynainputdeck.h"

#include "base/exception.h"


namespace insight {




LSDynaInputDeck::LSDynaInputDeck(LSDynaInputCards::Control_Units units)
    : units_(units)
{
    units_.setInputDeck(this);
}

void LSDynaInputDeck::append(LSDynaInputCardPtr inputCard)
{
    inputCard->setInputDeck(this);
    inputCards_.push_back(inputCard);
}

void LSDynaInputDeck::append(const std::vector<LSDynaInputCardPtr> &inputCards)
{
    for (auto& c: inputCards)
    {
        append(c);
    }
}


void LSDynaInputDeck::write(std::ostream &os) const
{
    os << units_;
    for (const auto& card: inputCards_)
    {
        os << *card;
    }
    os << "*END\n";
}



si::Pressure LSDynaInputDeck::stressUnit() const
{
    return massUnit()
                /
           (pow<2>(timeUnit())*lengthUnit());
}


boost::units::quantity<si::mass_density, double> LSDynaInputDeck::densityUnit() const
{
    return massUnit()
               /
           pow<3>(lengthUnit());
}


std::ostream &operator<<(std::ostream& os, const LSDynaInputDeck& id)
{
    id.write(os);
    return os;
}




} // namespace insight
