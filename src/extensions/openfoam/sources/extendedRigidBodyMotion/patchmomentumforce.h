#ifndef FOAM_EXTRBM_PATCHMOMENTUMFORCE_H
#define FOAM_EXTRBM_PATCHMOMENTUMFORCE_H


#include "word.H"
#include "vector.H"

#include "additionalforceandmoment.h"
#include "forcesources.h"


namespace Foam {

class fvMesh;

namespace extRBM {




class patchMomentumForce
      : public additionalForceAndMoment,
        public forceSource
{
    const fvMesh& mesh_;
    word patchName_;
    scalar rhoInf_;
    word rhoName_;

    vector F_, M_;

public:
    TypeName("patchMomentumForce");

    patchMomentumForce
    (
        const fvMesh& mesh,
        const word& patchName,
        scalar rhoInf,
        const word &rhoName
    );

    void calcFM();

    vector force() const override;
    std::pair<vector,vector> forceAndMoment() const override;
};




} // namespace extRBM
} // namespace Foam

#endif // FOAM_EXTRBM_PATCHMOMENTUMFORCE_H
