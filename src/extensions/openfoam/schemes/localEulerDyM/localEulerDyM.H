#ifndef LOCALEULERDYM_H
#define LOCALEULERDYM_H

#include "localEulerDdtScheme.H"
#include "fvMesh.H"

namespace Foam
{
namespace fv
{

template<class Type>
class localEulerDyMDdtScheme
: public localEulerDdtScheme<Type>
{

public:

    //- Runtime type information
    TypeName("localEulerDyM");


    // Constructors

        //- Construct from mesh
        localEulerDyMDdtScheme(const fvMesh& mesh)
        :
            localEulerDdtScheme<Type>(mesh)
        {}

        //- Construct from mesh and Istream
        localEulerDyMDdtScheme(const fvMesh& mesh, Istream& is)
        :
            localEulerDdtScheme<Type>(mesh, is)
        {}

        tmp<surfaceScalarField> meshPhi
        (
            const GeometricField<Type, fvPatchField, volMesh>&
        )
        {
          return this->mesh().phi();
        }

        // boilerplate
        typedef typename ddtScheme<Type>::fluxFieldType fluxFieldType;

        tmp<fluxFieldType> fvcDdtUfCorr
        (
            const GeometricField<Type, fvPatchField, volMesh>& U,
            const GeometricField<Type, fvsPatchField, surfaceMesh>& Uf
        )
        {
          return localEulerDdtScheme<Type>::fvcDdtUfCorr(U, Uf);
        }

        tmp<fluxFieldType> fvcDdtPhiCorr
        (
            const GeometricField<Type, fvPatchField, volMesh>& U,
            const fluxFieldType& phi
        )
        {
          return localEulerDdtScheme<Type>::fvcDdtPhiCorr(U, phi);
        }

        tmp<fluxFieldType> fvcDdtUfCorr
        (
            const volScalarField& rho,
            const GeometricField<Type, fvPatchField, volMesh>& U,
            const GeometricField<Type, fvsPatchField, surfaceMesh>& Uf
        )
        {
          return localEulerDdtScheme<Type>::fvcDdtUfCorr(rho, U, Uf);
        }

        tmp<fluxFieldType> fvcDdtPhiCorr
        (
            const volScalarField& rho,
            const GeometricField<Type, fvPatchField, volMesh>& U,
            const fluxFieldType& phi
        )
        {
          return localEulerDdtScheme<Type>::fvcDdtPhiCorr(rho, U, phi);
        }
};


// more boilerplate
template<>
tmp<surfaceScalarField> localEulerDyMDdtScheme<scalar>::fvcDdtUfCorr
(
    const GeometricField<scalar, fvPatchField, volMesh>& U,
    const GeometricField<scalar, fvsPatchField, surfaceMesh>& Uf
);

template<>
tmp<surfaceScalarField> localEulerDyMDdtScheme<scalar>::fvcDdtPhiCorr
(
    const volScalarField& U,
    const surfaceScalarField& phi
);

template<>
tmp<surfaceScalarField> localEulerDyMDdtScheme<scalar>::fvcDdtUfCorr
(
    const volScalarField& rho,
    const volScalarField& U,
    const surfaceScalarField& Uf
);

template<>
tmp<surfaceScalarField> localEulerDyMDdtScheme<scalar>::fvcDdtPhiCorr
(
    const volScalarField& rho,
    const volScalarField& U,
    const surfaceScalarField& phi
);

}
}

#endif // LOCALEULERDYM_H
