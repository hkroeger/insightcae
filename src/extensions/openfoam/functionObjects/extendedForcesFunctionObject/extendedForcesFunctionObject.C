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

#if OF_VERSION>=040000 //defined(OFdev)||defined(OFplus)||defined(OFesi1806)
#include "functionObject.H"
#else
#include "OutputFilterFunctionObject.H"
#endif

#include "wallFvPatch.H"
#include "addToRunTimeSelectionTable.H"

#include "uniof.h"

#include <vector>

namespace Foam
{







void extendedForces::createFields()
{
  const fvMesh& mesh = static_cast<const fvMesh&>(obr_);
  
  for (auto& f: std::vector<std::pair<volVectorField*&, word> >(
        { {pressureForce_, "pressureForce"}, {viscousForce_, "viscousForce"} } ) )
  {
      if (mesh.foundObject<volVectorField>(f.second))
      {
          Info<<"    retrieving field "<<f.second<<endl;
          f.first = const_cast<volVectorField*>(&mesh.lookupObject<volVectorField>(f.second));
      }
      else
      {
          Info<<"    creating field "<<f.second<<endl;
          f.first = &mesh.objectRegistry::store
          (
              new volVectorField
              (
               IOobject
               (
                 f.second,
                 mesh.time().timeName(),
                 mesh,
                 IOobject::NO_READ,
                 IOobject::AUTO_WRITE
               ),
               mesh,
               dimensionedVector(f.second, dimPressure, vector::zero),
               calculatedFvPatchField<vector>::typeName
              )
          );
      }
  }
}



#if OF_VERSION>=040000 //(defined(OFplus)||defined(OFdev)||defined(OFesi1806))
//- Construct for given objectRegistry and dictionary.
//  Allow the possibility to load fields from files
extendedForces::extendedForces
(
    const word& name,
    const Time& time,
    const dictionary& dict
#if OF_VERSION<060000
    ,
    const bool readFields
    #endif
)
: functionObjects::forces
  (
    name, time, dict
#if OF_VERSION<060000
    , readFields
#endif
  ),
  forceSource(name),
  maskFieldName_(dict.lookupOrDefault<word>("maskField", ""))
{
  createFields();
  if (maskFieldName_!="")
   Info<<name<<": Masking force integration with field "<<maskFieldName_<<endl;
}
#endif



//- Construct for given objectRegistry and dictionary.
//  Allow the possibility to load fields from files
extendedForces::extendedForces
(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict
    #if OF_VERSION<040000 //not (defined(OFplus)||defined(OFdev)||defined(OFesi1806))
    ,
    const bool loadFromFiles
    #endif
    #if OF_VERSION>=010700 && OF_VERSION<=060000
    ,
    const bool readFields
    #endif
)
:
#if OF_VERSION>=040000 //(defined(OFplus)||defined(OFdev)||defined(OFesi1806))
 functionObjects::
#endif
 forces(name, obr, dict
#if OF_VERSION<040000 //not (defined(OFplus)||defined(OFdev)||defined(OFesi1806))
      , loadFromFiles
#endif
#if OF_VERSION>=010700 && OF_VERSION<=060000
	  , readFields
#endif
	),
  forceSource(name),
  maskFieldName_(dict.lookupOrDefault<word>("maskField", ""))
{
  if (maskFieldName_!="")
   Info<<name<<": Masking force integration with field "<<maskFieldName_<<endl;
  createFields();
}



#if OF_VERSION>=010700 && OF_VERSION<040000
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
: forces(name, obr, patchSet, pName, UName, rhoName, rhoInf, pRef, coordSys),
  forceSource(name),
  maskFieldName_("")
{
  createFields();
}
#endif




#if OF_VERSION>=040000
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
  
#if OF_VERSION<040000
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

    pr_force_ = vector::zero;
    vi_force_ = vector::zero;
    po_force_ = vector::zero;
    pr_moment_ = vector::zero;
    vi_moment_ = vector::zero;
    po_moment_ = vector::zero;

    //forAllConstIter(labelHashSet, patchSet_, iter)
    forAll(mesh.boundaryMesh(), patchI)
    {
      if (isA<wallFvPatch>(mesh.boundary()[patchI]))
      {
        scalarField mask(mesh.boundary()[patchI].size(), 1.0);
        if (maskFieldName_!="")
        {
            const volScalarField& vmask = obr_.lookupObject<volScalarField>(maskFieldName_);
            mask = vmask.boundaryField()[patchI];
        }

        const vectorField& Sfb = mesh.Sf().boundaryField()[patchI];
        const vectorField nfb ( Sfb / mesh.magSf().boundaryField()[patchI] );

        const symmTensorField& devRhoReffb
            = tdevRhoReff().boundaryField()[patchI];

        UNIOF_BOUNDARY_NONCONST(*pressureForce_)[patchI]
#if OF_VERSION>=040000
        =
#else
        ==
#endif
        (
            rho(p)*nfb*(p.boundaryField()[patchI] - pRef)
        );

        UNIOF_BOUNDARY_NONCONST(*viscousForce_)[patchI]
#if OF_VERSION>=040000
        =
#else
        ==
#endif
        (
            nfb & devRhoReffb
        );
        
        if ( (maskFieldName_!="") && (patchSet_.found(patchI)) )
        {
            vectorField Md
            (
                mesh.C().boundaryField()[patchI] -
#if defined(OF_FORK_extend)
                  CofR_
#elif (OF_VERSION>=060505)
                  coordSysPtr_().origin()
#else
                  coordSys_.origin()
#endif
            );

            vectorField fN
            (
                mask*rho(p)*Sfb*(p.boundaryField()[patchI] - pRef)
            );

            vectorField fT(mask*(Sfb & devRhoReffb));

            vectorField fP(Md.size(), vector::zero);
            
            pr_force_ += sum(fN);
            vi_force_ += sum(fT);
            po_force_ += sum(fP);
            pr_moment_ += sum(Md^fN);
            vi_moment_ += sum(Md^fT);
            po_moment_ += sum(Md^fP);

        }
      }

    }
    
    Pstream::combineGather(pr_force_, plusEqOp<vector>());
    Pstream::combineGather(vi_force_, plusEqOp<vector>());
    Pstream::combineGather(po_force_, plusEqOp<vector>());
    Pstream::combineGather(pr_moment_, plusEqOp<vector>());
    Pstream::combineGather(vi_moment_, plusEqOp<vector>());
    Pstream::combineGather(po_moment_, plusEqOp<vector>());
    Pstream::combineScatter(pr_force_);
    Pstream::combineScatter(vi_force_);
    Pstream::combineScatter(po_force_);
    Pstream::combineScatter(pr_moment_);
    Pstream::combineScatter(vi_moment_);
    Pstream::combineScatter(po_moment_);
  }
  
#if OF_VERSION>=040000
  return true;
#endif
}



#if OF_VERSION>=040000
bool
#else
void 
#endif 
extendedForces::end()
{
#if OF_VERSION>=040000
  return
#endif    
  Foam::
#if OF_VERSION>=040000
  functionObjects::
#endif    
  forces::end();
}




#if OF_VERSION>=040000
bool
#else
void 
#endif
extendedForces::write()
{
//   const fvMesh& mesh = static_cast<const fvMesh&>(obr_);

  forces::write();
  
#if OF_VERSION<040000
  if (!active_)
  {
      return;
  }
#endif

  if (maskFieldName_!="")
  {
    const volVectorField& U = obr_.lookupObject<volVectorField>(UName_);
    const fvMesh& mesh = U.mesh();
    
    if (!maskedForceFile_.valid())
    {
        if (Pstream::master())
        {
            fileName outdir;
            
            word startTimeName =
                mesh.time().timeName(mesh.time().startTime().value());

            if (Pstream::parRun())
            {
                // Put in undecomposed case (Note: gives problems for
                // distributed data running)
                outdir = mesh.time().path()/".."/"postProcessing"/(name()+"_masked")/startTimeName;
            }
            else
            {
                outdir = mesh.time().path()/"postProcessing"/(name()+"_masked")/startTimeName;
            }

            // Create directory if does not exist.
            mkDir(outdir);

            // Open new file at start up
#if OF_VERSION>=040000
            maskedForceFile_.reset(new OFstream(outdir/"force.dat"));
            maskedForceFile2_.reset(new OFstream(outdir/"moment.dat"));
#else
            maskedForceFile_.reset(new OFstream(outdir/"forces.dat"));
#endif
        }
    }
    
    if (Pstream::master()) {
#if OF_VERSION>=040000
    maskedForceFile_() << obr_.time().value()
            << tab << (pr_force_+vi_force_)
            << tab << pr_force_
            << tab << vi_force_;
    if (porosity_)
    {
        maskedForceFile_()  << tab << po_force_;
    }
    maskedForceFile_()  << endl;

    maskedForceFile2_() << obr_.time().value()
            << tab << (pr_moment_+vi_moment_)
            << tab << pr_moment_
            << tab << vi_moment_;
    if (porosity_)
    {
        maskedForceFile2_()  << tab << po_moment_;
    }
    maskedForceFile2_()  << endl;
#else
    maskedForceFile_() << obr_.time().value() << tab << '('
        << (pr_force_) << ' '
        << (vi_force_) << ' '
        << (po_force_) << ") ("
        << (pr_moment_) << ' '
        << (vi_moment_) << ' '
        << (po_moment_) << ')'
        << endl;
#endif
   }
  }
  
#if OF_VERSION>=040000
  return true;
#endif
}



vector extendedForces::force() const
{
#if OF_VERSION>=017000
    return forceEff();
#else
    auto fm=calcForcesMoment();
    return fm.first().first()+fm.first().second();
#endif
}




#if OF_VERSION>=040000

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
