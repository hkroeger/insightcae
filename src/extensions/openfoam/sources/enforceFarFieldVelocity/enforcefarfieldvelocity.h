#ifndef ENFORCEFARFIELDVELOCITY_H
#define ENFORCEFARFIELDVELOCITY_H


#include "fvOption.H"
#include "fvCFD.H"
#include "patchDistMethod.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{

class enforceFarFieldVelocity
: public option
{

protected:

    word farFieldVelocityFieldName_;
    wordReList farFieldPatches_;
    scalar transitionDistance_;

    autoPtr<patchDistMethod> ym_;
    volScalarField y_;
    autoPtr<volVectorField> zero_;
    const volVectorField* UFarField_;

private:

    // Private Member Functions

    //- No copy construct
    enforceFarFieldVelocity(const enforceFarFieldVelocity&) = delete;

    //- No copy assignment
    void operator=(const enforceFarFieldVelocity&) = delete;


public:

    //- Runtime type information
    TypeName("enforceFarFieldVelocity");


    // Constructors

    //- Construct from components
    enforceFarFieldVelocity
    (
            const word& name,
            const word& modelType,
            const dictionary& dict,
            const fvMesh& mesh
            );

    //- Destructor
    virtual ~enforceFarFieldVelocity();

    // Member Functions

    //- Read dictionary
    virtual bool read(const dictionary& dict);

    //- Correct the energy field
    virtual void correct(volVectorField& U);

};

}
}


#endif // ENFORCEFARFIELDVELOCITY_H
