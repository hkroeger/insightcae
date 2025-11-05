#ifndef RADIALPROFILE_H
#define RADIALPROFILE_H

#include "fielddataprovider.h"

namespace Foam {


template<class T>
class radialProfile
    : public CylCoordProfile<T,RadialCylCoordVectorSpaceBase>
{
public:
    //- Runtime type information
    TypeName("radialProfile");

    radialProfile(Istream& is);
    radialProfile(const radialProfile<T>& o);

    virtual autoPtr<FieldDataProvider<T> > clone() const;
};


} // namespace Foam

#ifdef NoRepository
#   include "radialprofile.cpp"
#endif

#endif // RADIALPROFILE_H
