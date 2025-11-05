#ifndef CIRCUMFERENTIALPROFILE_H
#define CIRCUMFERENTIALPROFILE_H

#include "fielddataprovider.h"

namespace Foam {

template<class T>
class circumferentialProfile
    : public CylCoordProfile<T,CircumCylCoordVectorSpaceBase>
{
public:
    //- Runtime type information
    TypeName("circumferentialProfile");

    circumferentialProfile(Istream& is);
    circumferentialProfile(const circumferentialProfile<T>& o);

    virtual autoPtr<FieldDataProvider<T> > clone() const;
};

} // namespace Foam

#ifdef NoRepository
#   include "circumferentialprofile.cpp"
#endif

#endif // CIRCUMFERENTIALPROFILE_H
