#include "prescribedvelocity.h"
#include "addToRunTimeSelectionTable.H"

namespace Foam {
namespace RBD {
namespace restraints {

defineTypeNameAndDebug(prescribedVelocity, 0);

addToRunTimeSelectionTable
(
    restraint,
    prescribedVelocity,
    dictionary
);


prescribedVelocity::prescribedVelocity
(
    const word& name,
    const dictionary& dict,
    const rigidBodyModel& model
)
:
    forceSource(name),
    restraint(name, dict, model),
    velocitySet_(nullptr),
    oldForce_(0),
    error0_(0),
    integral0_(0),
    force_(0),
    F_(vector::zero, vector::zero)
{
    read(dict);
}


prescribedVelocity::~prescribedVelocity()
{}



void prescribedVelocity::updateForce() const
{
    // Adding rotation to a given body
    scalar velocity = model_.v(model_.master(bodyID_)).l() & direction_;

    scalar Inertia = model_.I(model_.master(bodyID_)).m();

    // from the definition of the angular momentum:
    // force = m*ddt(x)

    scalar t = model_.time().value();
    scalar dt = model_.time().deltaTValue();

    scalar error = velocitySet_->value(t) - velocity;
    scalar integral = integral0_ + error;
    scalar derivative = error - error0_;

    scalar force = (p_*error + i_*integral + d_*derivative);
    force *= Inertia/dt;

    force_ = relax_*force + (1- relax_)*oldForce_;
    F_ = model_.X0(bodyID_).T() & spatialVector(Zero, force_*direction_);

    if (model_.debug)
    {
        Info<< " velocity  " << velocity << endl
            << " wanted " << velocitySet_->value(t) << endl
            << " force " << force_ << endl
            << " inertia " << Inertia << endl
            << " error " << error << endl
            << " integral " << integral << endl
            << " derivative " << derivative << endl
            << " direction " << direction_
            << endl;
    }

    oldForce_ = force_;
    error0_ = error;
    integral0_ = integral;
}




void prescribedVelocity::restrain
(
    scalarField& tau,
    Field<spatialVector>& fx,
    const rigidBodyModelState& state
) const
{
    updateForce();

    // Accumulate the force for the restrained body
    fx[bodyIndex_] += F_;
}



bool prescribedVelocity::read
(
    const dictionary& dict
)
{
    restraint::read(dict);

    direction_ = vector(coeffs_.lookup("direction"));

    relax_=readScalar(coeffs_.lookup("relax"));

    p_=readScalar(coeffs_.lookup("p"));
    i_=readScalar(coeffs_.lookup("i"));
    d_=readScalar(coeffs_.lookup("d"));

    // Read the actual entry
    velocitySet_.reset(Function1<scalar>::New("velocity", coeffs_, &model_.time()));

    return true;
}


void prescribedVelocity::write
(
    Ostream& os
) const
{
    restraint::write(os);

    os << "direction" << token::SPACE << direction_ << token::END_STATEMENT << endl;
    os << "relax" << token::SPACE << relax_ << token::END_STATEMENT << endl;
    os << "p" << token::SPACE << p_ << token::END_STATEMENT << endl;
    os << "i" << token::SPACE << i_ << token::END_STATEMENT << endl;
    os << "d" << token::SPACE << d_ << token::END_STATEMENT << endl;

    velocitySet_->writeData(os);
}

vector prescribedVelocity::force() const
{
    vector F=F_.l();
    reduce(F, maxMagSqrOp<vector>());
    return F;
}




} // namespace restraints
} // namespace RBD
} // namespace Foam
