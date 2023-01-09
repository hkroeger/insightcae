#include "lsdynainputcard.h"

#include "base/exception.h"

namespace insight {



const LSDynaInputDeck& LSDynaInputCard::inputDeck() const
{
    insight::assertion(
                inputDeck_!=nullptr,
                "Reference to input deck is not set!" );
    return *inputDeck_;
}



std::ostream &operator<<(std::ostream& os, const LSDynaInputCard& ic)
{
    ic.write(os);
    return os;
}




namespace LSDynaInputCards {




InputCardWithId::InputCardWithId(int id)
    : id_(id)
{}





LSDynaInputCards::IncludeKey::IncludeKey(const path &fp)
    : boost::filesystem::path(fp)
{}

void LSDynaInputCards::IncludeKey::write(std::ostream &os) const
{
    os << "*INCLUDE\n";
    os << this->string() << "\n";
}


}



} // namespace insight
