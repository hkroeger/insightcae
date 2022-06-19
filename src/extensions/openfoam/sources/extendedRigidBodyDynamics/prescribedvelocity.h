#ifndef FOAM_RBD_RESTRAINTS_PRESCRIBEDVELOCITY_H
#define FOAM_RBD_RESTRAINTS_PRESCRIBEDVELOCITY_H

#include "rigidBodyRestraint.H"
#include "Function1.H"

namespace Foam {
namespace RBD {
namespace restraints {

class prescribedVelocity
:
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
};



} // namespace restraints
} // namespace RBD
} // namespace Foam

#endif // FOAM_RBD_RESTRAINTS_PRESCRIBEDVELOCITY_H
