
#include "fvCFD.H"
#include "extendedForcesFunctionObject.H"
#include "OutputFilterFunctionObject.H"

namespace Foam
{
  
//- Construct for given objectRegistry and dictionary.
//  Allow the possibility to load fields from files
extendedForces::extendedForces
(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict,
    const bool loadFromFiles,
    const bool readFields
)
: forces(name, obr, dict, loadFromFiles, readFields)
{
}


//- Construct from components
extendedForces::extendedForces
(
    const word& name,
    const objectRegistry& obr,
    const labelHashSet& patchSet,
    const word& pName,
    const word& UName,
    const word& rhoName,
    const scalar rhoInf,
    const scalar pRef,
    const coordinateSystem& coordSys
)
: forces(name, obr, patchSet, pName, UName, rhoName, rhoInf, pRef, coordSys)
{
}


//- Destructor
extendedForces::~extendedForces()
{
}

void extendedForces::execute()
{
  const fvMesh& mesh = static_cast<const fvMesh&>(obr_);
  
  if (!pressureForce_.valid())
    pressureForce_.reset
    (
      new volVectorField
      (
	IOobject
	(
	  "pressureForce",
	  mesh.time().timeName(),
	  mesh
	),
       mesh,
       dimensionedVector("pressureForce", dimPressure, vector::zero),
       calculatedFvPatchField<vector>::typeName
      )
    );

  if (!viscousForce_.valid())
    viscousForce_.reset
    (
      new volVectorField
      (
	IOobject
	(
	  "viscousForce",
	  mesh.time().timeName(),
	  mesh
	),
       mesh,
       dimensionedVector("viscousForce", dimPressure, vector::zero),
       calculatedFvPatchField<vector>::typeName
      )
    );

    forces::execute();
  
    initialise();
    
    if (!active_)
    {
        return;
    }
    
    if (directForceDensity_)
    {
      WarningIn("extendedForces::execute") << "direct force density unsupported!" << endl;
    }
    else
    {
        const volVectorField& U = obr_.lookupObject<volVectorField>(UName_);
        const volScalarField& p = obr_.lookupObject<volScalarField>(pName_);

        const fvMesh& mesh = U.mesh();

        const surfaceVectorField::GeometricBoundaryField& Sfb =
            mesh.Sf().boundaryField();

        tmp<volSymmTensorField> tdevRhoReff = devRhoReff();
        const volSymmTensorField::GeometricBoundaryField& devRhoReffb
            = tdevRhoReff().boundaryField();

        // Scale pRef by density for incompressible simulations
        scalar pRef = pRef_/rho(p);

        //forAllConstIter(labelHashSet, patchSet_, iter)
	forAll(mesh.boundaryMesh(), patchI)
        {
//             label patchI = iter.key();

            //vectorField fN
	    pressureForce_().boundaryField()[patchI]=
            (
                rho(p)*Sfb[patchI]*(p.boundaryField()[patchI] - pRef)
            );

	    viscousForce_().boundaryField()[patchI]=
//             vectorField fT
            (
	      Sfb[patchI] & devRhoReffb[patchI]
	    );

        }
    }    
}

typedef OutputFilterFunctionObject<extendedForces> extendedForcesFunctionObject;

defineNamedTemplateTypeNameAndDebug(extendedForcesFunctionObject, 0);

addToRunTimeSelectionTable
(
    functionObject,
    extendedForcesFunctionObject,
    dictionary
);

}