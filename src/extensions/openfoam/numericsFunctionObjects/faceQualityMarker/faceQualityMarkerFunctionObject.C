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

#include "faceQualityMarkerFunctionObject.H"
#include "fvCFD.H"
#include "fvcSmooth.H"
#include "addToRunTimeSelectionTable.H"
#include "surfaceFields.H"
#include "volFields.H"
#include "faceSet.H"
#include "cellSet.H"
#include "primitiveMeshTools.H"

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

void markFaceSet1(const faceSet& faces, surfaceScalarField& UBlendingFactor)
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
	  UBlendingFactor.boundaryField()[pI][fI-patch.start()]=1.0;
	}
    }
}

  
template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh> >
surfaceMax2
(
    const GeometricField<Type, fvsPatchField, surfaceMesh>& ssf
)
{
    const fvMesh& mesh = ssf.mesh();
    
    tmp<GeometricField<Type, fvPatchField, volMesh> > tvf
    (
        new GeometricField<Type, fvPatchField, volMesh>
        (
            IOobject
            (
                "surfaceMax("+ssf.name()+')',
                ssf.instance(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            dimensioned<Type>
            (
                "0",
                ssf.dimensions(),
                pTraits<Type>::zero
            ),
            calculatedFvPatchField<Type>::typeName
        )
    );
    
    GeometricField<Type, fvPatchField, volMesh>& vf = tvf();

    const labelUList& owner = mesh.owner();
    const labelUList& neighbour = mesh.neighbour();

    const Field<Type>& issf = ssf;

    forAll(owner, facei)
    {
        vf[owner[facei]] = max(vf[owner[facei]], issf[facei]);
        vf[neighbour[facei]] = max(vf[neighbour[facei]], issf[facei]);
    }

    forAll(mesh.boundary(), patchi)
    {
        const labelUList& pFaceCells =
            mesh.boundary()[patchi].faceCells();

        const fvsPatchField<Type>& pssf = ssf.boundaryField()[patchi];

        forAll(mesh.boundary()[patchi], facei)
        {
            vf[pFaceCells[facei]] = max(vf[pFaceCells[facei]], pssf[facei]);
        }
    }

    return tvf;
}

template<class Type>
tmp<GeometricField<Type, fvsPatchField, surfaceMesh> >
surfaceMax3
(
    const GeometricField<Type, fvPatchField, volMesh>& vsf
)
{
    const fvMesh& mesh = vsf.mesh();
    
    tmp<GeometricField<Type, fvsPatchField, surfaceMesh> > tsf
    (
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        (
            IOobject
            (
                "surfaceMax3("+vsf.name()+')',
                vsf.instance(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            dimensioned<Type>
            (
                "0",
                vsf.dimensions(),
                pTraits<Type>::zero
            ),
            calculatedFvsPatchField<Type>::typeName
        )
    );
    
    GeometricField<Type, fvsPatchField, surfaceMesh>& sf = tsf();

    const labelUList& owner = mesh.owner();
    const labelUList& neighbour = mesh.neighbour();

    forAll(sf, facei)
    {
        sf[facei]=max(vsf[owner[facei]], vsf[neighbour[facei]]);
    }

    forAll(mesh.boundary(), patchi)
    {
        const labelUList& pFaceCells =
            mesh.boundary()[patchi].faceCells();

        fvsPatchField<Type>& pssf = sf.boundaryField()[patchi];

        forAll(mesh.boundary()[patchi], facei)
        {
            pssf[facei] = vsf[pFaceCells[facei]];
        }
    }

    return tsf;
}


void Foam::faceQualityMarkerFunctionObject::markFaceSet(const faceSet& faces)
{
  forAll(blendingFactors_, i)
  {
      markFaceSet1(faces, blendingFactors_[i]);
  }   
}

void Foam::faceQualityMarkerFunctionObject::updateBlendingFactor()
{
  forAll(blendingFactors_, i)
    blendingFactors_[i]=0.0;  

  if (markNonOrthFaces_)
    {
      faceSet faces(mesh_, "nonOrthoFaces", mesh_.nFaces()/100 + 1);
      mesh_.checkFaceOrthogonality(true, &faces);
      label nFaces=faces.size();
      reduce(nFaces, sumOp<label>());
      Info<<"Marking "
	  <<nFaces
	  <<" non orthogonal faces."<<endl;
      markFaceSet(faces);
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

      markFaceSet(faces);
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

      markFaceSet(faces);
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

      markFaceSet(faces);
    }

  if (markHighAspectFaces_)
    {

      faceSet faces(mesh_, "aspectFaces", mesh_.nFaces()/100 + 1);

      cellSet cells(mesh_, "nonClosedCells", mesh_.nCells()/100 + 1);
      cellSet acells(mesh_, "aspectCells", mesh_.nCells()/100 + 1);
      /*
      mesh_.checkClosedCells
      (
//	mesh_.faceAreas(),
//	mesh_.cellVolumes(),
	true, 
	&cells, 
	&acells, 
	mesh_.geometricD()
      );
      */
      
      scalarField openness;
      scalarField aspectRatio;
      primitiveMeshTools::cellClosedness
      (
	  mesh_,
	  mesh_.geometricD(),
	  mesh_.faceAreas(),
	  mesh_.cellVolumes(),
	  openness,
	  aspectRatio
      );
      forAll(aspectRatio, cellI)
      {
	  if (aspectRatio[cellI] > aspectThreshold_)
	  {
	      acells.insert(cellI);
	  }
      }

      const labelList& cl=acells.toc();
      forAll(cl, i)
      {
	label ci=cl[i];
	const labelList &cfs = mesh_.cells()[ci];
	forAll(cfs,j)
	  faces.insert(cfs[j]);
      }

      label nFaces=faces.size();
      reduce(nFaces, sumOp<label>());
      Info<<"Marking "
          <<nFaces
          <<" faces of high aspect ratio cells."<<endl;

      markFaceSet(faces);
    }

    forAll(blendingFactors_, i)
    {
	// smoothing the field
	volScalarField avgBlendingFactor( static_cast<const volScalarField&>(surfaceMax2(blendingFactors_[i])) );
	avgBlendingFactor.rename("avg_"+blendingFactors_[i].name());
	
	fvc::smooth(avgBlendingFactor, smoothingCoeff_);
// 	fvc::spread(avgBlendingFactor, avgBlendingFactor, 1);
// 	fvc::smooth(avgBlendingFactor, smoothingCoeff_);
	
	blendingFactors_[i] = surfaceMax3(avgBlendingFactor);
	
	if (debug)
	{
	  Info<<"Writing volScalarField "<<avgBlendingFactor.name()<<endl;
	  avgBlendingFactor.write();  
	  Info<<"Writing surfaceScalarField "<<blendingFactors_[i].name()<<endl;
	  blendingFactors_[i].write();  
	}
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
    markHighAspectFaces_(dict.lookupOrDefault<bool>("markHighAspectFaces", true)),
    aspectThreshold_(dict.lookupOrDefault<scalar>("aspectThreshold", 500.0)),
    smoothingCoeff_(dict.lookupOrDefault<scalar>("smoothingCoeff", 0.75)),
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
