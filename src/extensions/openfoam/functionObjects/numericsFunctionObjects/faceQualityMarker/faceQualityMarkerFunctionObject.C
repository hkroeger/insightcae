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

#if OF_VERSION>=010700 //ndef OF16ext
#include "fvcSmooth.H"
#endif

#include "addToRunTimeSelectionTable.H"
#include "surfaceFields.H"
#include "volFields.H"
#include "faceSet.H"
#include "cellSet.H"
#include "emptyFvPatch.H"

#if OF_VERSION>=010700 //ndef OF16ext
#include "polyMeshTetDecomposition.H"
#endif

#if OF_VERSION>020100 //(!( defined(OF16ext) || defined(OF21x) ))
#include "unitConversion.H"
#include "primitiveMeshTools.H"
#endif

#include "uniof.h"

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

inline void markFace(label fI, surfaceScalarField& UBlendingFactor, scalar value)
{
  const fvMesh& mesh=UBlendingFactor.mesh();
  
  if (fI<mesh.nInternalFaces())
  {
    UBlendingFactor[fI]=1.0;
  }
  else
  {
    label pI = mesh.boundaryMesh().whichPatch(fI);
    const polyPatch& patch = mesh.boundaryMesh()[pI];
//       Info<<pI<<" "<<patch.start()<<" "<<fI<<endl;
    
    fvsPatchField<scalar>& UBFp = UNIOF_BOUNDARY_NONCONST(UBlendingFactor)[pI];
    
    if (!isA<emptyFvPatch>(mesh.boundary()[pI]))
      UBFp[fI-patch.start()]=1.0;
  }
}

void markFaceSet1(const faceSet& faces, surfaceScalarField& UBlendingFactor)
{
  const labelList& fl=faces.toc();
  forAll(fl, i) 
  {
    label fI=fl[i];
    markFace(fI, UBlendingFactor, 1.0);
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
    
    GeometricField<Type, fvPatchField, volMesh>& vf = UNIOF_TMP_NONCONST(tvf);

    const UNIOF_LABELULIST& owner = mesh.owner();
    const UNIOF_LABELULIST& neighbour = mesh.neighbour();

    const Field<Type>& issf = ssf;

    forAll(owner, facei)
    {
        vf[owner[facei]] = max(vf[owner[facei]], issf[facei]);
        vf[neighbour[facei]] = max(vf[neighbour[facei]], issf[facei]);
    }

    forAll(mesh.boundary(), patchi)
    {
        const UNIOF_LABELULIST& pFaceCells =
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
    
    GeometricField<Type, fvsPatchField, surfaceMesh>& sf = UNIOF_TMP_NONCONST(tsf);

    const UNIOF_LABELULIST& owner = mesh.owner();
    const UNIOF_LABELULIST& neighbour = mesh.neighbour();

    forAll(sf, facei)
    {
        sf[facei]=max(vsf[owner[facei]], vsf[neighbour[facei]]);
    }

    forAll(mesh.boundary(), patchi)
    {
        const UNIOF_LABELULIST& pFaceCells =
            mesh.boundary()[patchi].faceCells();

        fvsPatchField<Type>& pssf = UNIOF_BOUNDARY_NONCONST(sf)[patchi];

        forAll(mesh.boundary()[patchi], facei)
        {
            pssf[facei] = vsf[pFaceCells[facei]];
        }
    }

    return tsf;
}


void Foam::faceQualityMarkerFunctionObject::markFaceSet(label mi, const faceSet& faces)
{
    auto& blendingFactors = blendingFactors_[mi];

    forAll(blendingFactors, i)
    {
      markFaceSet1(faces, *blendingFactors[i]);
    }
}

void Foam::faceQualityMarkerFunctionObject::updateBlendingFactor()
{
  forAll(meshes_, mi)
  {
    Info<<"Marking faces for region "<<regionNames_[mi]<<endl;
    const auto& mesh = *meshes_[mi];
    auto& blendingFactors = blendingFactors_[mi];

    forAll(blendingFactors, i) (*blendingFactors[i])=0.0;

    if (markNonOrthFaces_)
    {
      faceSet faces(mesh, "nonOrthoFaces", mesh.nFaces()/100 + 1);
      
#if OF_VERSION<=020100 //(defined(OF16ext)||defined(OF21x))
      WarningIn("faceQualityMarkerFunctionObject::updateBlendingFactor()")
          <<"Consideration of non-orthogonality threshold unavailable in OF16ext and OF21x! Using built-in threshold."
	<<endl;
      
      mesh.checkFaceOrthogonality(true, &faces);
      label nFaces=faces.size();
      markFaceSet(mi, faces);
#else
      scalar lo=::cos(degToRad(lowerNonOrthThreshold_));
      scalar up=::cos(degToRad(upperNonOrthThreshold_));
      
      tmp<scalarField> tortho = primitiveMeshTools::faceOrthogonality
          (
            mesh,
            mesh.faceAreas(),
            mesh.cellCentres()
            );
      const scalarField& ortho = tortho();

      label nFaces=0;
      forAll(ortho, faceI)
      {
        scalar o=ortho[faceI];

        if (o < lo )
        {
          nFaces++;

          scalar val = 1. - min(1., max(0., ( (o - up) / (lo - up))));

          //Info<<o<<" "<<lo<<" "<<up<<" : "<<val<<endl;

          forAll(blendingFactors, i)
              markFace(faceI, *blendingFactors[i], val);
        }
      }
#endif
      
      reduce(nFaces, sumOp<label>());
      Info<<"Marking "
	  <<nFaces
         <<" non orthogonal faces."<<endl;
    }

    if (markSkewFaces_)
    {
      faceSet faces(mesh, "skewFaces", mesh.nFaces()/100 + 1);
      mesh.checkFaceSkewness(true, &faces);
      label nFaces = faces.size();
      reduce(nFaces, sumOp<label>());
      Info << "Marking "
           << nFaces
           << " skewed faces." << endl;

      markFaceSet(mi, faces);
    }
    
    if (markWarpedFaces_)
    {
      faceSet faces(mesh, "warpedFaces", mesh.nFaces()/100 + 1);

      mesh.checkFaceFlatness
          (
            true,
      #if OF_VERSION>=010700 //ndef OF16ext
            0.8,
      #endif
            &faces
            );

      label nFaces=faces.size();
      reduce(nFaces, sumOp<label>());
      Info<<"Marking "<<nFaces<<" warped faces."<<endl;

      markFaceSet(mi, faces);
    }
    
    if (markConcaveFaces_)
    {
      faceSet faces(mesh, "concaveFaces", mesh.nFaces()/100 + 1);

      mesh.checkFaceAngles
          (
            true,
#if OF_VERSION>=010700 //ndef OF16ext
            10,
#endif
            &faces
            );

      label nFaces=faces.size();
      reduce(nFaces, sumOp<label>());
      Info<<"Marking "<<nFaces<<" concave faces."<<endl;

      markFaceSet(mi, faces);
    }

    if (markLowQualityTetFaces_)
    {
#if OF_VERSION>=010700 //ndef OF16ext
      faceSet faces(mesh, "lowTetQualityFaces", mesh.nFaces()/100 + 1);

      polyMeshTetDecomposition::checkFaceTets
          (
            mesh,
            polyMeshTetDecomposition::minTetQuality,
            true,
            &faces
            );

      label nFaces=faces.size();
      reduce(nFaces, sumOp<label>());
      Info<<"Marking "<<nFaces
         <<" faces with low quality or negative volume "
	<< "decomposition tets."<<endl;

      markFaceSet(mi, faces);
#else
      WarningIn("faceQualityMarker::updateBlendingFactor()")
          << "Criterion markLowTetQualityFaces unavailable in OF16ext! Ignored." <<endl;
#endif
      
    }
    
    if (markHighAspectFaces_)
    {

      cellSet acells(mesh, "aspectCells", mesh.nCells()/100 + 1);

#if OF_VERSION<=020100 //( defined(OF16ext) || defined(OF21x)  )
#warning aspect ratio threshold will be ignored in OF16ext and OF21x!
      cellSet cells(mesh, "nonClosedCells", mesh.nCells()/100 + 1);
      mesh.checkClosedCells
          (
            true,
            &cells,
            &acells
            );
#else
      scalarField openness;
      scalarField aspectRatio;
      primitiveMeshTools::cellClosedness
          (
            mesh,
            mesh.geometricD(),
            mesh.faceAreas(),
            mesh.cellVolumes(),
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
#endif

      faceSet faces(mesh, "aspectFaces", mesh.nFaces()/100 + 1);
      const labelList& cl=acells.toc();
      forAll(cl, i)
      {
        label ci=cl[i];
        const labelList &cfs = mesh.cells()[ci];
        forAll(cfs,j) faces.insert(cfs[j]);
      }

      label nFaces=faces.size();
      reduce(nFaces, sumOp<label>());

      Info<<"Marking "<<nFaces
         <<" faces of high aspect ratio cells."<<endl;

      markFaceSet(mi, faces);
    }

    if (markVelocityPeaks_)
    {
      const label procI = Pstream::myProcNo();

      const volVectorField& U = mesh.lookupObject<volVectorField>("U");
      volScalarField magField(mag(U));

      labelList maxIs(Pstream::nProcs());
      scalarList maxVs(Pstream::nProcs());
      List<vector> maxCs(Pstream::nProcs());

      label maxProcI = findMax(magField);
      maxVs[procI] = magField[maxProcI];
      maxCs[procI] = U.mesh().C()[maxProcI];

      Pstream::gatherList(maxVs);
      Pstream::gatherList(maxCs);
      Pstream::scatterList(maxVs);
      Pstream::scatterList(maxCs);

      label maxI = findMax(maxVs);
      //     scalar maxValue = maxVs[maxI];
      //     const vector& maxC = maxCs[maxI];

      faceSet faces(mesh, "dummy", 1);
      if (maxI==procI)
      {
        const cell& c = mesh.cells()[maxProcI];
        forAll(c, i) faces.insert(c[i]);
      }

      Pout<<"Marking "<<faces.size()
         <<" faces of velocity peak cell on proc #"<<maxI<<"."<<endl;

      markFaceSet(mi, faces);
    }


    if (sets_.size())
    {
      forAll(sets_, i)
      {
        faceSet faces(mesh, sets_[i]);

        label nFaces=faces.size();
        reduce(nFaces, sumOp<label>());

        Info<<"Marking "<<nFaces
           <<" faces from faceSet "<<sets_[i]<<"."<<endl;

        markFaceSet(mi, faces);
      }
    }

    forAll(blendingFactors, i)
    {
      if (smoothMarkerField_)
      {
#if OF_VERSION>020100 //(!( defined(OF16ext) || defined(OF21x) ))
        // smoothing the field
        volScalarField avgBlendingFactor( static_cast<const volScalarField&>(surfaceMax2(*blendingFactors[i])) );
        avgBlendingFactor.rename("avg_"+blendingFactors[i]->name());

        fvc::smooth(avgBlendingFactor, smoothingCoeff_);
        // 	  fvc::spread(avgBlendingFactor, avgBlendingFactor, 1);
        // 	  fvc::smooth(avgBlendingFactor, smoothingCoeff_);

        *blendingFactors[i] = surfaceMax3(avgBlendingFactor);

        if (debug)
        {
          Info<<"Writing volScalarField "<<avgBlendingFactor.name()<<endl;
          avgBlendingFactor.write();
        }
#else
        WarningIn("QualityMarkerFunctionObject::updateBlendingFactor()")
            <<"Smoothing unavailable in OF16ext and OF21x. Omitting."
	<<endl;
#endif
      }

      if (debug)
      {
        Info<<"Writing surfaceScalarField "<<blendingFactors[i]->name()<<endl;
        blendingFactors[i]->write();
      }
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
    regionNames_(time_.names<polyMesh>()),
    blendingFactors_(regionNames_.size()),
    markNonOrthFaces_(dict.lookupOrDefault<bool>("markNonOrthFaces", true)),
    markSkewFaces_(dict.lookupOrDefault<bool>("markSkewFaces", true)),
    markWarpedFaces_(dict.lookupOrDefault<bool>("markWarpedFaces", false)),
    markConcaveFaces_(dict.lookupOrDefault<bool>("markConcaveFaces", false)),
    markHighAspectFaces_(dict.lookupOrDefault<bool>("markHighAspectFaces", true)),
    markLowQualityTetFaces_(dict.lookupOrDefault<bool>("markLowQualityTetFaces", true)),
    markVelocityPeaks_(dict.lookupOrDefault<bool>("markVelocityPeaks", false)),
    smoothMarkerField_(dict.lookupOrDefault<bool>("smoothMarkerField", true)),
    updateOnMeshChange_(dict.lookupOrDefault<bool>("updateOnMeshChange", true)),
    aspectThreshold_(dict.lookupOrDefault<scalar>("aspectThreshold", 500.0)),
    lowerNonOrthThreshold_(dict.lookupOrDefault<scalar>("lowerNonOrthThreshold", 35.0)),
    upperNonOrthThreshold_(dict.lookupOrDefault<scalar>("upperNonOrthThreshold", 65.0)),
    smoothingCoeff_(dict.lookupOrDefault<scalar>("smoothingCoeff", 0.25)),
    meshes_(regionNames_.size())
{
  forAll(regionNames_, ri)
  {
    Info<<"Setting up face quality markers in region "<<regionNames_[ri]<<flush;
    meshes_[ri]=&time_.lookupObject<polyMesh>(regionNames_[ri]);
    Info<<" (nCells="<<meshes_[ri]->nCells()<<", nFaces="<<meshes_[ri]->nFaces()<<")"<<endl;
  }
  if (dict.found("blendingFieldNames"))
  {
    blendingFieldNames_=wordList(dict.lookup("blendingFieldNames"));
  }
  else
  {
    blendingFieldNames_=wordList(1);
    blendingFieldNames_[0]="UBlendingFactor";
  }
  if (dict.found("sets"))
  {
    sets_=wordList(dict.lookup("sets"));
  }

#if OF_VERSION>=040000 //defined(OFdev)||defined(OFplus)||defined(OFesi1806)
  start();
#endif
}

Foam::faceQualityMarkerFunctionObject::~faceQualityMarkerFunctionObject()
{
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::faceQualityMarkerFunctionObject::start()
{
  forAll(meshes_, mi)
  {
    const auto& pmesh = *meshes_[mi];
    auto& blendingFactors = blendingFactors_[mi];

    if (!isA<fvMesh>(pmesh))
    {
	WarningIn("Foam::faceQualityMarkerFunctionObject::start()")
	<<"mesh is not a fvMesh! Disabling."<<endl;
	return false;
    }
    
    Info << "Creating face quality markers" << endl;
    fvMesh& mesh=const_cast<fvMesh&>(refCast<const fvMesh>(pmesh));

    blendingFactors.setSize(blendingFieldNames_.size());
    for (label i=0; i<blendingFieldNames_.size(); i++)
    {
     Info<<"Creating "<<blendingFieldNames_[i]<<endl;
     blendingFactors[i] = &mesh.objectRegistry::store
     (
	new surfaceScalarField
        (
         IOobject
         (
          blendingFieldNames_[i],
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
  }

  updateBlendingFactor();
  return true;
}


bool Foam::faceQualityMarkerFunctionObject::execute
(
#if (defined(OF_FORK_extend) && OF_VERSION>=010604) || (!defined(OF_FORK_extend) && OF_VERSION<040000) //!(defined(OF16ext) || defined(OFdev)||defined(OFplus)||defined(OFesi1806)) || defined(Fx41)
  bool
#endif
)
{
  bool updateNeeded=false;

  forAll(meshes_, mi)
    if (meshes_[mi]->changing() && updateOnMeshChange_) updateNeeded=true;

  if (updateNeeded)
    updateBlendingFactor();
  return true;
}

bool Foam::faceQualityMarkerFunctionObject::write()
{
    return false;
}


bool Foam::faceQualityMarkerFunctionObject::read(const dictionary& dict)
{
    return false;
}


#if OF_VERSION>=020100 || (defined(OF_FORK_extend) && OF_VERSION>=010604) //||(!defined(OF16ext)||defined(Fx41)) && !defined(OF21x)
//- Update for changes of mesh
void Foam::faceQualityMarkerFunctionObject::updateMesh(const mapPolyMesh& mpm)
{
}

//- Update for changes of mesh
void Foam::faceQualityMarkerFunctionObject::movePoints(
    #if (defined(OF_FORK_extend) && OF_VERSION>=010604) //defined(Fx41)
            const pointField&
    #else
            const polyMesh& mesh
    #endif
)
{
}
#endif

// ************************************************************************* //
