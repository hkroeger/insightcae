#ifndef FITTEDPROFILE_H
#define FITTEDPROFILE_H

#include "fielddataprovider.h"

namespace Foam
{

template<class T>
class fittedProfile
    : public FieldDataProvider<T>
{
    LinearVectorSpaceBase base_;
    std::vector< std::vector<arma::mat> > coeffs_;

    virtual void appendInstant(Istream& is);
    virtual void writeInstant(int i, Ostream& os) const;

public:
    //- Runtime type information
    TypeName("fittedProfile");

    fittedProfile(Istream& is);
    fittedProfile(const fittedProfile<T>& o);

    virtual void read(Istream& is);
    virtual void writeSup(Ostream& os) const;

    virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
    virtual autoPtr<FieldDataProvider<T> > clone() const;
};

}

#ifdef NoRepository
#   include "fittedprofile.cpp"
#endif

#endif // FITTEDPROFILE_H
