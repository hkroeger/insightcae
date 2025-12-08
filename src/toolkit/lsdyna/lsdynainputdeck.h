#ifndef INSIGHT_LSDYNAINPUTDECK_H
#define INSIGHT_LSDYNAINPUTDECK_H

#include "base/boost_include.h"
#include "base/linearalgebra.h"
#include "base/units.h"

#include <vector>
#include <array>
#include <map>
#include <ostream>


#include "lsdyna/lsdynainputcard.h"
#include "lsdyna/control.h"
#include "lsdyna/mesh.h"


namespace insight {


class LSDynaInputDeck
{
    LSDynaInputCards::Control_Units units_;
    std::vector<LSDynaInputCardPtr> inputCards_;
public:
    LSDynaInputDeck(LSDynaInputCards::Control_Units units);

    template<class Card>
    Card* append(Card *inputCard)
    {
        std::shared_ptr<Card> ic(inputCard);
        append(ic);
        return ic.get();
    }

    void append(LSDynaInputCardPtr inputCard);
    void append(const std::vector<LSDynaInputCardPtr>& inputCards);
    void write(std::ostream& os) const;

    inline si::Length lengthUnit() const { return units_.lengthUnit_; }
    inline si::Time timeUnit() const { return units_.timeUnit_; }
    inline si::Mass massUnit() const { return units_.massUnit_; }
    si::Pressure stressUnit() const;
    boost::units::quantity<si::mass_density, double> densityUnit() const;

    static
    std::map<int, boost::filesystem::path>
    resultFiles(
        const boost::filesystem::path& workDir,
        const std::string& prefix );
};


std::ostream &operator<<(std::ostream& os, const LSDynaInputDeck& id);


} // namespace insight

#endif // INSIGHT_LSDYNAINPUTDECK_H
