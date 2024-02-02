#include "discretecurve.h"

#include "base/exception.h"

namespace insight {
namespace bmd {


CS::CS(arma::mat et0, const arma::mat& en0)
{
    et=et0/arma::norm(et0,2);

    eq=arma::cross(en0, et);
    eq/=arma::norm(eq,2);

    en=arma::cross(et, eq);
    en/=arma::norm(en,2);
}




CS DiscreteCurve::localCS(size_t i, arma::mat en) const
{
    insight::assertion(size()>2, "curve needs to be made up of at least three points!");
    en/=arma::norm(en,2);
    arma::mat et;
    if (i==0)
    {
        et=(*this)[1]-(*this)[0];
    }
    else if (i==size()-1)
    {
        et=(*this)[size()-1]-(*this)[size()-2];
    }
    else
    {
        et=(*this)[i+1]-(*this)[i-1];
    }
    return CS(et, en);
}


void DiscreteCurve::transform(std::function<arma::mat(const arma::mat&)> trsfFunc)
{
    for (auto& p: *this)
    {
        p = trsfFunc(p);
    }
}


void DiscreteCurve::transform(
        std::function<arma::mat(const arma::mat&, const CS&, size_t)> trsfFuncLocal,
        const arma::mat& en )
{
    std::vector<CS> lcs;
    for (size_t i=0; i<size(); ++i)
    {
        lcs.push_back(localCS(i, en));
    }
    for (size_t i=0; i<size(); ++i)
    {
        (*this)[i]=trsfFuncLocal( (*this)[i], lcs[i], i );
    }
}

void DiscreteCurve::interpolatedOffset(const arma::mat& ofsp0, const arma::mat& ofsp1, const arma::mat& en)
{
    std::vector<double> t{0};
    for (size_t i=1; i<size(); ++i)
    {
        t.push_back(t[i-1] + arma::norm((*this)[i]-(*this)[i-1],2));
    }
    double tmax=t.back();
    for (size_t i=0; i<size(); ++i)
    {
        t[i]/=tmax;
    }

    arma::mat ofsp0l, ofsp1l;
    {
        auto cs0 = localCS(0, en);
        ofsp0l=vec3(
                    arma::dot(cs0.et, ofsp0),
                    arma::dot(cs0.eq, ofsp0),
                    arma::dot(cs0.en, ofsp0)
                    );
    }
    {
        auto cs0 = localCS(size()-1, en);
        ofsp1l=vec3(
                    arma::dot(cs0.et, ofsp1),
                    arma::dot(cs0.eq, ofsp1),
                    arma::dot(cs0.en, ofsp1)
                    );
    }

    transform( [&](const arma::mat& p, const CS& lcs, size_t i) -> arma::mat
                {
                    arma::mat lofs = ofsp0l*(1.-t[i]) + ofsp1l*t[i];
                    return p + lcs.et*lofs[0] + lcs.eq*lofs[1] + lcs.en*lofs[2];
                },
                en
    );
}


} // namespace bmd
} // namespace insight
