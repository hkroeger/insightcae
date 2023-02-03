#ifndef INSIGHT_BMD_DISCRETECURVE_H
#define INSIGHT_BMD_DISCRETECURVE_H

#include "base/linearalgebra.h"

namespace insight {
namespace bmd {

struct CS
{
    arma::mat et, eq, en;
    CS(arma::mat et, const arma::mat& en);
};


class DiscreteCurve
        : public std::vector<arma::mat>
{
public:
    CS localCS(size_t i, arma::mat en) const;
    void transform(std::function<arma::mat(const arma::mat&)> trsfFunc);
    void transform(std::function<arma::mat(const arma::mat&, const CS&, size_t)> trsfFuncLocal, const arma::mat& en);
    void interpolatedOffset(const arma::mat& ofsp0, const arma::mat& ofsp1, const arma::mat& en);
};

} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_DISCRETECURVE_H
