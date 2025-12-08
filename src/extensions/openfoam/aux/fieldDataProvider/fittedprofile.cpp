#include "fittedprofile.h"

namespace Foam {

template<class T>
fittedProfile<T>::fittedProfile(Istream& is)
    : FieldDataProvider<T>(is)
{}

template<class T>
void fittedProfile<T>::appendInstant(Istream& is)
{
    std::vector<arma::mat> ccoeffs;
    for (int c=0; c<pTraits<T>::nComponents; c++)
    {
        token ct(is);
        if (ct.pToken()!=token::BEGIN_LIST)
        {
            FatalErrorIn("appendInstant")
            << "Expected "<<token::BEGIN_LIST
            << ", got " << ct
            << abort(FatalError);
        }
        std::vector<double> coeff;
        do
        {
            token nt(is);
            if (!nt.isNumber())
            {
                FatalErrorIn("appendInstant")
                << "Expected number, got "
                << nt
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

        ccoeffs.push_back(arma::mat(coeff.data(), coeff.size(), 1));
    }

    coeffs_.push_back(ccoeffs);
}

template<class T>
void fittedProfile<T>::writeInstant(int i, Ostream& is) const
{
    const std::vector<arma::mat>& ccoeffs=coeffs_[i];
    for(const auto& c: ccoeffs)
    {
        is << token::BEGIN_LIST << token::SPACE;
        for (unsigned int j=0; j<c.n_elem; j++)
            is << c(j) << token::SPACE;
        is << token::END_LIST << token::SPACE;
    }
}

template<class T>
tmp<Field<T> > fittedProfile<T>::atInstant(int idx, const pointField& target) const
{
    tmp<Field<T> > resPtr(new Field<T>(target.size(), pTraits<T>::zero));
    Field<T>& res=UNIOF_TMP_NONCONST(resPtr);

    forAll(target, pi)
    {
        double t = base_.t(target[pi]);
        for (int c=0; c<pTraits<T>::nComponents; c++)
        {
            arma::mat coeff = coeffs_[idx][c];
            setComponent( res[pi], c )=evalPolynomial(t, coeff);
        }
        res[pi]=base_(res[pi]);
    }
    return resPtr;
}

template<class T>
fittedProfile<T>::fittedProfile(const fittedProfile<T>& o)
    : FieldDataProvider<T>(o),
    base_(o.base_), //p0_(o.p0_), ep_(o.ep_), ex_(o.ex_), ez_(o.ez_),
    coeffs_(o.coeffs_)
{
}

template<class T>
void fittedProfile<T>::read(Istream& is)
{
    base_.read(is);
    FieldDataProvider<T>::read(is);
}

template<class T>
void fittedProfile<T>::writeSup(Ostream& os) const
{
    base_.writeSup(os);
}

template<class T>
autoPtr<FieldDataProvider<T> > fittedProfile<T>::clone() const
{
    return autoPtr<FieldDataProvider<T> >(new fittedProfile<T>(*this));
}

}
