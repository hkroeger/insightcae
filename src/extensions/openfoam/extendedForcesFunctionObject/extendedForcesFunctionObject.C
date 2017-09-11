/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "fvCFD.H"
#include "extendedForcesFunctionObject.H"

#if defined(OFdev)||defined(OFplus)
#include "functionObject.H"
#else
#include "OutputFilterFunctionObject.H"
#endif

#include "wallFvPatch.H"
#include "addToRunTimeSelectionTable.H"

namespace Foam
{


void extendedForces::createFields()
{
  const fvMesh& mesh = static_cast<const fvMesh&>(obr_);
  
  pressureForce_=&mesh.objectRegistry::store
  (
      new volVectorField
      (
	IOobject
	(
	  "pressureForce",
	  mesh.time().timeName(),
	  mesh,
	  IOobject::NO_READ,
	  IOobject::AUTO_WRITE
	),
       mesh,
       dimensionedVector("pressureForce", dimPressure, vector::zero),
       calculatedFvPatchField<vector>::typeName
      )
  );
     
  viscousForce_=&mesh.objectRegistry::store
  (
      new volVectorField
      (
	IOobject
	(
	  "viscousForce",
	  mesh.time().timeName(),
	  mesh,
	  IOobject::NO_READ,
	  IOobject::AUTO_WRITE
	),
       mesh,
       dimensionedVector("viscousForce", dimPressure, vector::zero),
       calculatedFvPatchField<vector>::typeName
      )
  );
}

#ifdef OFplus
//- Construct for given objectRegistry and dictionary.
//  Allow the possibility to load fields from files
extendedForces::extendedForces
(
    const word& name,
    const Time& time,
    const dictionary& dict,
    const bool readFields
)
: functionObjects::forces(name, time, dict, readFields)
{
  createFields();
}
#endif

//- Construct for given objectRegistry and dictionary.
//  Allow the possibility to load fields from files
extendedForces::extendedForces
(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict
#ifndef OFplus
    ,
    const bool loadFromFiles
#endif
#ifndef OF16ext
    ,
    const bool readFields
#endif
)
:

#ifdef OFplus
 functionObjects::
#endif
 forces(name, obr, dict
#ifndef OFplus
      , loadFromFiles
#endif
#ifndef OF16ext
	  , readFields
#endif
	)

{
  createFields();
}

#if !(defined(OF16ext)||defined(OFplus))
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
  createFields();
}
#endif

//- Destructor
extendedForces::~extendedForces()
{
}

#ifdef OFplus
bool
#else
void 
#endif
extendedForces::execute()
{
//   const fvMesh& mesh = static_cast<const fvMesh&>(obr_);

  forces::execute();

#ifndef OF16ext
  initialise();
#endif
  
#ifndef OFplus
  if (!active_)
  {
      return;
  }
#endif
  
  if (directForceDensity_)
  {
    WarningIn("extendedForces::execute") << "direct force density unsupported!" << endl;
  }
  else
  {
    const volVectorField& U = obr_.lookupObject<volVectorField>(UName_);
    const volScalarField& p = obr_.lookupObject<volScalarField>(pName_);

    const fvMesh& mesh = U.mesh();


    tmp<volSymmTensorField> tdevRhoReff = devRhoReff();

    // Scale pRef by density for incompressible simulations
    scalar pRef = pRef_/rho(p);

    //forAllConstIter(labelHashSet, patchSet_, iter)
    forAll(mesh.boundaryMesh(), patchI)
    {
      if (isA<wallFvPatch>(mesh.boundary()[patchI]))
      {
        const vectorField nfb =
            mesh.Sf().boundaryField()[patchI] / mesh.magSf().boundaryField()[patchI];

        const symmTensorField& devRhoReffb
            = tdevRhoReff().boundaryField()[patchI];

        pressureForce_->boundaryField()[patchI]
#ifdef OFplus
        =
#else
        ==
#endif
        (
            rho(p)*nfb*(p.boundaryField()[patchI] - pRef)
        );

        viscousForce_->boundaryField()[patchI]
#ifdef OFplus
        =
#else
        ==
#endif
        (
            nfb & devRhoReffb
        );
      }

    }
  }
  
#ifdef OFplus
  return true;
#endif
}

#ifdef OFplus
bool
#else
void 
#endif 
extendedForces::end()
{
#ifdef OFplus
  return
#endif    
  Foam::
#ifdef OFplus
  functionObjects::
#endif    
  forces::end();
}


#ifdef OFplus

defineTypeNameAndDebug(extendedForces, 0);
addToRunTimeSelectionTable
(
    functionObject,
    extendedForces,
    dictionary
);

#else

defineTypeNameAndDebug(extendedForces, 0);
typedef OutputFilterFunctionObject<extendedForces> extendedForcesFunctionObject;
defineNamedTemplateTypeNameAndDebug(extendedForcesFunctionObject, 0);
addToRunTimeSelectionTable
(
    functionObject,
    extendedForcesFunctionObject,
    dictionary
);

#endif



}
