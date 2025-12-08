#include "circumferentialprofile.h"

namespace Foam {


template<class T>
circumferentialProfile<T>::circumferentialProfile(Istream& is)
    : CylCoordProfile<T,CircumCylCoordVectorSpaceBase>(is)
{}


template<class T>
circumferentialProfile<T>::circumferentialProfile(
    const circumferentialProfile<T>& o
    )
    : CylCoordProfile<T,CircumCylCoordVectorSpaceBase>(o)
{}

template<class T>
autoPtr<FieldDataProvider<T> > circumferentialProfile<T>::clone() const
{
    return autoPtr<FieldDataProvider<T> >(new circumferentialProfile<T>(*this));
}



} // namespace Foam
