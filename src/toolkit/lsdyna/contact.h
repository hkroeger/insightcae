#ifndef INSIGHT_LSDYNAINPUTCARDS_CONTACT_H
#define INSIGHT_LSDYNAINPUTCARDS_CONTACT_H

#include "lsdynainputcard.h"
#include "base/units.h"

namespace insight {
namespace LSDynaInputCards {




class ContactAutomaticGeneral
        : public LSDynaInputCard
{
public:
    enum InputSetType {

        Segment = 0,
            //Segment set ID.
        ShellElementSet = 1,
            //Shell element set ID.
        PartSet = 2,
            //Part set ID.
        Part = 3,
            //Part ID.
        All = 5,
            //All parts (SID is ignored).
        PartSetExclude = 6,
            //Part set ID. All parts are included in the contact, except the one defined in the part set ID.
        SubsetSet = 7
            //Subset set ID.
    };

private:
    const InputCardWithId* slaveSet_;
    InputSetType inputSetType_;
    double staticFriction_, dynamicFriction_, exponentialDecayCoeff_;
    si::Time tStart_, tEnd_;

public:
    ContactAutomaticGeneral(
            double staticFriction,
            double dynamicFriction,
            double exponentialDecayCoeff,
            const InputCardWithId* set =nullptr,
            InputSetType inputSetType = All,
            si::Time tStart = 0,
            si::Time tEnd = 1e30*si::seconds );

    void write(std::ostream& os) const override;
};




} // namespace LSDynaInputCards
} // namespace insight

#endif // INSIGHT_LSDYNAINPUTCARDS_CONTACT_H
