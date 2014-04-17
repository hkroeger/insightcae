/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Author
    Hrvoje Jasak, Wikki Ltd.  All rights reserved

\*---------------------------------------------------------------------------*/

#include "faceQualityMarkerFunctionObject.H"
#include "addToRunTimeSelectionTable.H"
#include "surfaceFields.H"
#include "faceSet.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(faceQualityMarkerFunctionObject, 0);

    addToRunTimeSelectionTable
    (
        functionObject,
        faceQualityMarkerFunctionObject,
        dictionary
    );
}

void markFaceSet(const faceSet& faces, surfaceScalarField& UBlendingFactor)
{
  const labelList& fl=faces.toc();
  const fvMesh& mesh=UBlendingFactor.mesh();
  forAll(fl, i) 
    {
      label fI=fl[i];
      if (fI<mesh.nInternalFaces())
	UBlendingFactor[fI]=1.0;
      else
	{
	  label pI=mesh.boundaryMesh().whichPatch(fI);
	  const polyPatch& patch=mesh.boundaryMesh()[pI];
	  UBlendingFactor[fI-patch.start()]=1.0;
	}
    }
 }

void Foam::faceQualityMarkerFunctionObject::updateBlendingFactor()
{
  UBlendingFactor_()=0.0;  

  if (markNonOrthFaces_)
    {
      faceSet faces(mesh_, "nonOrthoFaces", mesh_.nFaces()/100 + 1);
      mesh_.checkFaceOrthogonality(true, &faces);
      label nFaces=faces.size();
      reduce(nFaces, sumOp<label>());
      Info<<"Marking "
	  <<nFaces
	  <<" non orthogonal faces."<<endl;
      markFaceSet(faces, UBlendingFactor_());
    }

  if (markSkewFaces_)
    {
      faceSet faces(mesh_, "skewFaces", mesh_.nFaces()/100 + 1);
      mesh_.checkFaceSkewness(true, &faces);
      label nFaces=faces.size();
      reduce(nFaces, sumOp<label>());
      Info<<"Marking "
	  <<nFaces
	  <<" skewed faces."<<endl;

      markFaceSet(faces, UBlendingFactor_());
    }
    
  if (markWarpedFaces_)
    {
      faceSet faces(mesh_, "warpedFaces", mesh_.nFaces()/100 + 1);
      mesh_.checkFaceFlatness(true, 
#ifndef OF16ext
			      0.8, 
#endif
			      &faces);
      label nFaces=faces.size();
      reduce(nFaces, sumOp<label>());
      Info<<"Marking "
	  <<nFaces
	  <<" warped faces."<<endl;

      markFaceSet(faces, UBlendingFactor_());
    }
    
  if (markConcaveFaces_)
    {
      faceSet faces(mesh_, "concaveFaces", mesh_.nFaces()/100 + 1);
      mesh_.checkFaceAngles(true, 
#ifndef OF16ext
			    10, 
#endif
			    &faces);
      label nFaces=faces.size();
      reduce(nFaces, sumOp<label>());
      Info<<"Marking "
	  <<nFaces
	  <<" concave faces."<<endl;

      markFaceSet(faces, UBlendingFactor_());
    }
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::faceQualityMarkerFunctionObject::faceQualityMarkerFunctionObject
(
    const word& name,
    const Time& t,
    const dictionary& dict
)
:
    functionObject(name),
    time_(t),
    regionName_(dict.lookupOrDefault<word>("region", polyMesh::defaultRegion)),
    markNonOrthFaces_(dict.lookupOrDefault<bool>("markNonOrthFaces", true)),
    markSkewFaces_(dict.lookupOrDefault<bool>("markSkewFaces", true)),
    markWarpedFaces_(dict.lookupOrDefault<bool>("markWarpedFaces", true)),
    markConcaveFaces_(dict.lookupOrDefault<bool>("markConcaveFaces", true)),
    mesh_(time_.lookupObject<polyMesh>(regionName_))
{
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::faceQualityMarkerFunctionObject::start()
{
    if (!isA<fvMesh>(mesh_)) 
    {
	WarningIn("Foam::faceQualityMarkerFunctionObject::start()")
	<<"mesh is not a fvMesh! Disabling."<<endl;
	return false;
    }
    Info << "Creating face quality markers" << endl;
    const fvMesh& mesh=refCast<const fvMesh>(mesh_);
    UBlendingFactor_.reset
    (
	new surfaceScalarField
    (
     IOobject
     (
      "UBlendingFactor",
      mesh.time().timeName(),
      mesh,
      IOobject::NO_READ,
      IOobject::NO_WRITE
     ),
     mesh,
     dimensionedScalar("", dimless, 0.0)
    )

    );
    updateBlendingFactor();
    return true;
}


bool Foam::faceQualityMarkerFunctionObject::execute
(
#ifndef OF16ext
  bool
#endif
)
{
  if (mesh_.changing()) updateBlendingFactor();
  return true;
}


bool Foam::faceQualityMarkerFunctionObject::read(const dictionary& dict)
{
    return false;
}

#if !defined(OF16ext) && !defined(OF21x)

          //- Update for changes of mesh
void Foam::faceQualityMarkerFunctionObject::updateMesh(const mapPolyMesh& mpm)
{
}

        //- Update for changes of mesh
void Foam::faceQualityMarkerFunctionObject::movePoints(const polyMesh& mesh)
{
}
#endif
// ************************************************************************* //
