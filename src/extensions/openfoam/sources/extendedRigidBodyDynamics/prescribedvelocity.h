#ifndef FOAM_RBD_RESTRAINTS_PRESCRIBEDVELOCITY_H
#define FOAM_RBD_RESTRAINTS_PRESCRIBEDVELOCITY_H

#include "rigidBodyRestraint.H"
#include "Function1.H"
#include "forcesources.h"

namespace Foam {
namespace RBD {
namespace restraints {

class prescribedVelocity
      : public forceSource,
        public restraint
{
    // Private data

        //- Global unit axis along which the velocity is controlled
        vector direction_;

        //- Rotational velocity [rad/sec]
        autoPtr<Function1<scalar> > velocitySet_;

        //- Cache previous momentum
        mutable scalar oldForce_;

        //- Relax momentum
        scalar relax_;

        //- PID constants

            mutable scalar error0_;

            mutable scalar integral0_;

            mutable scalar p_;

            mutable scalar i_;

            mutable scalar d_;

        mutable scalar force_;
        mutable spatialVector F_;
public:

    //- Runtime type information
    TypeName("prescribedVelocity");


    // Constructors

        //- Construct from components
        prescribedVelocity
        (
            const word& name,
            const dictionary& dict,
            const rigidBodyModel& model
        );

        //- Construct and return a clone
        virtual autoPtr<restraint> clone() const
        {
            return autoPtr<restraint>
            (
                new prescribedVelocity(*this)
            );
        }


    //- Destructor
    virtual ~prescribedVelocity();


    // Member Functions

        void updateForce() const;

        //- Accumulate the retraint internal joint forces into the tau field and
        //  external forces into the fx field
        void restrain
        (
            scalarField& tau,
            Field<spatialVector>& fx,
            const rigidBodyModelState& state
        ) const override;

        //- Update properties from given dictionary
        bool read(const dictionary& dict) override;

        //- Write
        void write(Ostream&) const override;

        vector force() const override;
};



} // namespace restraints
} // namespace RBD
} // namespace Foam

#endif // FOAM_RBD_RESTRAINTS_PRESCRIBEDVELOCITY_H
