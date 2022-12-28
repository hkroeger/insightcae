#ifndef FOAM_EXTRBM_ADDITIONALFORCEANDMOMENT_H
#define FOAM_EXTRBM_ADDITIONALFORCEANDMOMENT_H

#include "vector.H"

namespace Foam {
namespace extRBM {

class additionalForceAndMoment
{
public:
    TypeName("additionalForceAndMoment");

    virtual ~additionalForceAndMoment();

    virtual std::pair<vector,vector> forceAndMoment() const =0;
};


} // namespace extRBM
} // namespace Foam

#endif // FOAM_EXTRBM_ADDITIONALFORCEANDMOMENT_H
