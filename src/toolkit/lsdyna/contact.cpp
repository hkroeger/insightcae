#include "contact.h"

#include "lsdyna/lsdynainputdeck.h"

namespace insight {
namespace LSDynaInputCards {

ContactAutomaticGeneral::ContactAutomaticGeneral(
        double staticFriction,
        double dynamicFriction,
        double exponentialDecayCoeff,
        const InputCardWithId* set,
        InputSetType inputSetType,
        si::Time tStart,
        si::Time tEnd )
    : slaveSet_(set),
      inputSetType_(inputSetType),
      staticFriction_(staticFriction),
      dynamicFriction_(dynamicFriction),
      exponentialDecayCoeff_(exponentialDecayCoeff),
      tStart_(tStart),
      tEnd_(tEnd)
 {}

void ContactAutomaticGeneral::write(std::ostream& os) const
{
    os << "*CONTACT_AUTOMATIC_GENERAL\n"

       << (slaveSet_?slaveSet_->id():0) << ",, "
       << int(inputSetType_) << "\n"

       << staticFriction_ << ", " <<dynamicFriction_ << ", "<<exponentialDecayCoeff_
       << ",,,, "
       << toValue(tStart_, inputDeck().timeUnit()) << ", "
       << toValue(tEnd_, inputDeck().timeUnit()) << "\n"

       << "0,0,0,0\n";
}

} // namespace LSDynaInputCards
} // namespace insight
