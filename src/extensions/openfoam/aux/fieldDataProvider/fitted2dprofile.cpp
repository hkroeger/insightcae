#include "fitted2dprofile.h"

namespace Foam {




template<class T>
fitted2DProfile<T>::fitted2DProfile(Istream& is)
    : FieldDataProvider<T>(is)
{}

template<class T>
void fitted2DProfile<T>::appendInstant(Istream& is)
{
    auto nc=typename decltype(coeffs_)::value_type();

    for (int c=0; c<pTraits<T>::nComponents; c++)
    {
        for (int dir=0; dir<2; ++dir)
        {
            token ct(is);

            MinMax mima{-DBL_MAX,DBL_MAX};
            if (ct.isNumber())
            {
                // expect min/max pair
                mima.first=ct.number();
                is >> mima.second;

                ct=token(is);
            }

            if (ct.pToken()!=token::BEGIN_LIST)
            {
                FatalErrorIn("appendInstant")
                    << "Expected " << token::BEGIN_LIST
                    << abort(FatalError);
            }

            std::vector<double> coeff;
            do
            {
                token nt(is);
                if (!nt.isNumber())
                {
                    FatalErrorIn("appendInstant")
                        << "Expected number, got " << nt
                        << abort(FatalError);
                }
                coeff.push_back(nt.number());
                {
                    token nt2(is);
                    if (nt2.isPunctuation() && (nt2.pToken()==token::END_LIST))
                    {
                        break;
                    }
                    else
                    {
                        is.putBack(nt2);
                    }
                }
            } while (!is.eof());

            nc[c][dir]=CoeffsAndLimits{
                arma::mat(coeff.data(), coeff.size(), 1),
                mima };
        }
    }

    coeffs_.push_back(nc);
}

template<class T>
void fitted2DProfile<T>::writeInstant(int i, Ostream& is) const
{
    for(auto& nc: coeffs_)
    {
        for (int c=0; c<pTraits<T>::nComponents; c++)
        {
            for (int dir=0; dir<2; ++dir)
            {
                auto &cl=nc[c][dir];

                is << cl.second.first << token::SPACE
                   << cl.second.second << token::SPACE;

                is << token::BEGIN_LIST << token::SPACE;
                for (unsigned int j=0; j<cl.first.n_elem; j++)
                    is << cl.first(j) << token::SPACE;
                is << token::END_LIST << token::SPACE;
            }
        }
    }
}

template<class T>
tmp<Field<T> > fitted2DProfile<T>::atInstant(int idx, const pointField& target) const
{
    tmp<Field<T> > resPtr(new Field<T>(target.size(), pTraits<T>::zero));
    Field<T>& res=UNIOF_TMP_NONCONST(resPtr);

    forAll(target, pi)
    {
        double t = base_.t(target[pi]);
        double u = base_.u(target[pi]);
        for (int c=0; c<pTraits<T>::nComponents; c++)
        {
            arma::mat coefft = coeffs_[idx][c][0].first;
            auto tlim =        coeffs_[idx][c][0].second;

            arma::mat coeffu = coeffs_[idx][c][1].first;
            auto ulim =        coeffs_[idx][c][1].second;

            auto tclipped=
                std::max(tlim.first,
                    std::min(t, tlim.second));

            auto uclipped=
                std::max(ulim.first,
                    std::min(u, ulim.second));

            auto value =
                evalPolynomial(tclipped, coefft)
                *
                evalPolynomial(uclipped, coeffu);

            setComponent( res[pi], c ) = value;
        }
        res[pi]=base_(res[pi]);
    }
    return resPtr;
}

template<class T>
fitted2DProfile<T>::fitted2DProfile(const fitted2DProfile<T>& o)
    : FieldDataProvider<T>(o),
    base_(o.base_), //p0_(o.p0_), ep_(o.ep_), ex_(o.ex_), ez_(o.ez_),
    coeffs_(o.coeffs_)
{
}

template<class T>
void fitted2DProfile<T>::read(Istream& is)
{
    base_.read(is);
    FieldDataProvider<T>::read(is);
}

template<class T>
void fitted2DProfile<T>::writeSup(Ostream& os) const
{
    base_.writeSup(os);
}

template<class T>
autoPtr<FieldDataProvider<T> > fitted2DProfile<T>::clone() const
{
    return autoPtr<FieldDataProvider<T> >(new fitted2DProfile<T>(*this));
}


} // namespace Foam
