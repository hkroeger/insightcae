#ifndef INSIGHT_LSDYNAINPUTCARDS_SOLUTION_H
#define INSIGHT_LSDYNAINPUTCARDS_SOLUTION_H

#include "lsdyna/lsdynainputcard.h"
#include "base/linearalgebra.h"


namespace insight {
namespace LSDynaInputCards {


class Curve
        : public InputCardWithId
{
    arma::mat xy_;

public:
    Curve(int id, const arma::mat& xy);
    void write(std::ostream& os) const override;
};



} // namespace LSDynaInputCards
} // namespace insight

#endif // INSIGHT_LSDYNAINPUTCARDS_SOLUTION_H
