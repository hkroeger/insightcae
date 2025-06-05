#include "lsdynainputdeck.h"

#include "base/exception.h"
#include "boost/lexical_cast.hpp"


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

std::map<int, boost::filesystem::path>
LSDynaInputDeck::resultFiles(
    const boost::filesystem::path &workDir,
    const std::string &prefix)
{
    std::map<int, boost::filesystem::path> result;

    boost::regex refn("^"+prefix+"A([0-9]+)\\.vtk$");

    for (boost::filesystem::directory_iterator i(workDir);
         i!=boost::filesystem::directory_iterator(); ++i)
    {
        boost::smatch m;
        auto fn = i->path().filename().string();
        if (boost::regex_search(fn, m, refn, boost::match_default))
        {
            int idx=boost::lexical_cast<int>(m[1]);
            result[idx]=i->path();
        }
    }
    return result;
}


std::ostream &operator<<(std::ostream& os, const LSDynaInputDeck& id)
{
    id.write(os);
    return os;
}




} // namespace insight
