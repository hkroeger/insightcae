#ifndef LIMITFIELD_H
#define LIMITFIELD_H

#include "cellSetOption.H"
#include "fvCFD.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{

template<class Type>
class limitField
: public cellSetOption
{

protected:

    word fieldName_;
    scalar min_, max_;

private:

    // Private Member Functions

    //- No copy construct
    limitField(const limitField&) = delete;

    //- No copy assignment
    void operator=(const limitField&) = delete;


public:

    //- Runtime type information
    TypeName("limitField");


    // Constructors

    //- Construct from components
    limitField
    (
            const word& name,
            const word& modelType,
            const dictionary& dict,
            const fvMesh& mesh
            );

    //- Destructor
    virtual ~limitField()
    {}


    // Member Functions

    //- Read dictionary
    virtual bool read(const dictionary& dict);

    //- Correct the energy field
    virtual void correct(GeometricField<Type, fvPatchField, volMesh>& U);

};

}
}

#ifdef NoRepository
    #include "limitfield.cpp"
#endif

#endif // LIMITFIELD_H
