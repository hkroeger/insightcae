#include "radialprofile.h"

namespace Foam {



template<class T>
radialProfile<T>::radialProfile(Istream& is)
    : CylCoordProfile<T,RadialCylCoordVectorSpaceBase>(is)
{}


template<class T>
radialProfile<T>::radialProfile(const radialProfile<T>& o)
    : CylCoordProfile<T,RadialCylCoordVectorSpaceBase>(o)
{}

template<class T>
autoPtr<FieldDataProvider<T> > radialProfile<T>::clone() const
{
    return autoPtr<FieldDataProvider<T> >(new radialProfile<T>(*this));
}



} // namespace Foam
