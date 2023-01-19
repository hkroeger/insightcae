#include "solution.h"

namespace insight {
namespace LSDynaInputCards {




LSDynaInputCards::Curve::Curve(int id, const arma::mat &xy)
    : InputCardWithId(id),
      xy_(xy)
{}


void LSDynaInputCards::Curve::write(std::ostream& os) const
{
    os << "*DEFINE_CURVE\n";
    os << id() << ", 1, 1, 0, 0\n";
    for (int i=0; i<xy_.n_rows; ++i)
    {
        os << xy_(i,0) << ", " << xy_(i,1) << "\n";
    }
}



} // namespace LSDynaInputCards
} // namespace insight
