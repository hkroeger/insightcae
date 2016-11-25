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

#include "interfaceMarkerFunctionObject.H"
#include <boost/graph/graph_concepts.hpp>
#include "fvCFD.H"
#ifndef OF16ext
#include "fvcSmooth.H"
#endif
#include "addToRunTimeSelectionTable.H"
#include "surfaceFields.H"
#include "volFields.H"
#include "faceSet.H"
#include "cellSet.H"

#ifndef OF16ext
#include "polyMeshTetDecomposition.H"
#endif

#if (!( defined(OF16ext) || defined(OF21x) ))
#include "unitConversion.H"
#include "primitiveMeshTools.H"
#endif

#ifdef OF16ext
#define LABELULIST unallocLabelList
#else
#define LABELULIST labelUList
#endif

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(interfaceMarkerFunctionObject, 0);

    addToRunTimeSelectionTable
    (
        functionObject,
        interfaceMarkerFunctionObject,
        dictionary
    );
}


void Foam::interfaceMarkerFunctionObject::updateBlendingFactor()
{
  const volScalarField& pf = mesh_.lookupObject<volScalarField>(phaseFieldName_);
  surfaceScalarField spf = fvc::interpolate(pf);
  
  forAll(blendingFactors_, i)
  {
    blendingFactors_[i] =
      max
        (
            1.0 - mag(1.0 - Foam::pow(4.0*(spf - 0.5)*(0.5 - spf), 4.0)),
            0.0
        );  
	
    if (debug)
    {
      if (mesh_.time().outputTime())
      {
	volScalarField blendavg=fvc::average(blendingFactors_[i]);
	blendavg.rename(blendingFactors_[i].name());
	Info<<"Writing volScalarField "<<blendavg.name()<<endl;
	blendavg.write();  
      }
//       Info<<"Writing surfaceScalarField "<<blendingFactors_[i].name()<<endl;
//       blendingFactors_[i].write();  
    }
  }
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::interfaceMarkerFunctionObject::interfaceMarkerFunctionObject
(
    const word& name,
    const Time& t,
    const dictionary& dict
)
:
    functionObject(name),
    time_(t),
    regionName_(dict.lookupOrDefault<word>("region", polyMesh::defaultRegion)),
    phaseFieldName_(dict.lookupOrDefault<word>("phaseFieldName", "alpha1")),
    mesh_(time_.lookupObject<polyMesh>(regionName_))
{
    if (dict.found("blendingFieldNames"))
    	blendingFieldNames_.reset(new wordList(dict.lookup("blendingFieldNames")));
    else
    {
    	blendingFieldNames_.reset(new wordList(1));
        blendingFieldNames_()[0]="UBlendingFactor";
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::interfaceMarkerFunctionObject::start()
{
    if (!isA<fvMesh>(mesh_)) 
    {
	WarningIn("Foam::interfaceMarkerFunctionObject::start()")
	<<"mesh is not a fvMesh! Disabling."<<endl;
	return false;
    }
    Info << "Creating interface markers" << endl;
    const fvMesh& mesh=refCast<const fvMesh>(mesh_);
    blendingFactors_.resize(blendingFieldNames_().size());
    for (label i=0; i<blendingFieldNames_().size(); i++)
    {
     Info<<"Creating "<<blendingFieldNames_()[i]<<endl;
     blendingFactors_.set
     (
        i,
	new surfaceScalarField
        (
         IOobject
         (
          blendingFieldNames_()[i],
          mesh.time().timeName(),
          mesh,
          IOobject::NO_READ,
          IOobject::NO_WRITE
         ),
         mesh,
         dimensionedScalar("", dimless, 0.0),
	 calculatedFvPatchField<scalar>::typeName
        )
     );
    }
    updateBlendingFactor();
    return true;
}


bool Foam::interfaceMarkerFunctionObject::execute
(
#if not (defined(OF16ext)||defined(OFdev))
  bool
#endif
)
{
  updateBlendingFactor();
  return true;
}

bool Foam::interfaceMarkerFunctionObject::write()
{
    return false;
}


bool Foam::interfaceMarkerFunctionObject::read(const dictionary& dict)
{
    return false;
}

#if !defined(OF16ext) && !defined(OF21x)

          //- Update for changes of mesh
void Foam::interfaceMarkerFunctionObject::updateMesh(const mapPolyMesh& mpm)
{
}

        //- Update for changes of mesh
void Foam::interfaceMarkerFunctionObject::movePoints(const polyMesh& mesh)
{
}
#endif
// ************************************************************************* //
