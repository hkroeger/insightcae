#ifndef FITTED2DPROFILE_H
#define FITTED2DPROFILE_H

#include "fielddataprovider.h"

namespace Foam {


template<class T>
class fitted2DProfile
    : public FieldDataProvider<T>
{
public:
    typedef std::pair<scalar,scalar> MinMax;
    typedef std::pair<arma::mat,MinMax> CoeffsAndLimits;

private:
    Linear2DVectorSpaceBase base_;
    std::vector<
        std::array<
            std::array<CoeffsAndLimits, 2>, // per direction
            pTraits<T>::nComponents> // per component
        > coeffs_;

    virtual void appendInstant(Istream& is);
    virtual void writeInstant(int i, Ostream& os) const;

public:
    //- Runtime type information
    TypeName("fitted2DProfile");

    fitted2DProfile(Istream& is);
    fitted2DProfile(const fitted2DProfile<T>& o);

    virtual void read(Istream& is);
    virtual void writeSup(Ostream& os) const;

    virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
    virtual autoPtr<FieldDataProvider<T> > clone() const;
};


} // namespace Foam


#ifdef NoRepository
#   include "fitted2dprofile.cpp"
#endif

#endif // FITTED2DPROFILE_H
