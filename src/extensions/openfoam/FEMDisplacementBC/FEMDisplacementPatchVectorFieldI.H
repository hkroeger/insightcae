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

\*---------------------------------------------------------------------------*/

#include "FEMDisplacementPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "Time.H"
#include "transformField.H"
#include "fvMesh.H"
#include "fvsPatchField.H"
#include "displacementLaplacianFvMotionSolver.H"
#include "primitivePatchInterpolation.H"
#include "IFstream.H"
#include "triSurfaceMesh.H"
#include "triSurfaceTools.H"
#include <unistd.h>
#include "faceSet.H"
#include "faCFD.H"

#include "base/vtktools.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


namespace Foam
{

template<class PointPatch>
vectorField smooth
(
 const fvPatch& fvpatch, 
 const PointPatch& patch, 
 const vectorField& f
)
{
  primitivePatchInterpolation ipol(fvpatch.patch());
  vectorField fdisp=ipol.pointToFaceInterpolate(f);
  vectorField fdisp_smooth(fdisp.size());
  forAll(fdisp, i)
    {
      const labelList& nb=fvpatch.patch().faceFaces()[i];
      vector s=vector::zero;
      forAll(nb, nbi)
	{
	  s+=fdisp[nb[nbi]];
	}
      fdisp_smooth[i]=s/scalar(nb.size());
    }
  return ipol.faceToPointInterpolate(fdisp_smooth);
}

template<class PointPatch>
vectorField extrapolate
(
 const fvPatch& fvpatch, 
 const PointPatch& patch, 
 const vectorField& f
)
{
  return f;
}

}


#if defined(FACE_DECOMP)

#include "tetPolyPatchFields.H"
#include "tetPolyBoundaryMesh.H"
#include "tetPolyMesh.H"
#include "tetPolyPatchInterpolation.H"

namespace Foam
{

template<>
vectorField smooth<tetPolyPatchFaceDecomp>
(
 const fvPatch& fvpatch, 
 const tetPolyPatchFaceDecomp& patch, 
 const vectorField& f
)
{
  const polyMesh& mesh=patch.boundaryMesh().mesh()();
  const polyPatch& ppatch=mesh.boundaryMesh()[patch.index()];

  tetPolyPatchInterpolationFaceDecomp ipol(refCast<const faceTetPolyPatchFaceDecomp>(patch));

  SubField<vector> fdisp(f, ppatch.size(), ppatch.nPoints()); // extract face values
  //SubField<vector> fdisp(f, ppatch.nPoints(), 0); // extract point values (leave out center values)

  vectorField fdisp_smooth(fdisp.size());
  forAll(fdisp, i)
    {
      const labelList& nb=fvpatch.patch().faceFaces()[i];
      vector s=vector::zero;
      forAll(nb, nbi)
	{
	  s+=fdisp[nb[nbi]];
	}
      fdisp_smooth[i]=s/scalar(nb.size());
    }

  return ipol.faceToPointInterpolate(fdisp_smooth);
}

template<>
vectorField extrapolate<tetPolyPatchFaceDecomp>
(
 const fvPatch& fvpatch, 
 const tetPolyPatchFaceDecomp& patch, 
 const vectorField& f
)
{
  const polyPatch& ppatch=fvpatch.patch();
  const fvMesh& mesh=fvpatch.boundaryMesh().mesh();
    
  // extract face values
  SubField<vector> fdisp(f, ppatch.size(), ppatch.nPoints());
    
  // get faces, on which the motion is to extrapolate
  word zoneName(fvpatch.name()+"MotionExtrapolate");

  label zoneI=mesh.faceZones().findZoneID(zoneName);
  if (zoneI<0) 
    FatalErrorIn("extrapolate")
      <<"Could not find zone "<<zoneName
      <<abort(FatalError);
  labelList faceLabels(mesh.faceZones()[zoneI]);
    
  faMesh aMesh(mesh, faceLabels);
  const indirectPrimitivePatch& ipatch = aMesh.patch();

  // lump all edges of faMesh-patch into a single boundary
  labelList displacement_bound(ipatch.nEdges()-ipatch.nInternalEdges(), -1);
  forAll(displacement_bound, i)
    displacement_bound[i]=ipatch.nInternalEdges()+i;
    
  //Info<<"# faces = "<<faceLabels.size()<<endl;
  //Info<<"# edges = "<<displacement_bound.size()<<endl;

  SLList<faPatch*> faPatchLst;
  faPatchLst.append
    (
     new faPatch
     (
      "displacement",
      displacement_bound,
      0,
      aMesh.boundary(),
      -1
      )
     );
  aMesh.addFaPatches(List<faPatch*>(faPatchLst));

  areaVectorField adisplacement
    (
     IOobject
     (
      "displacement",
      mesh.time().timeName(),
      mesh,
      IOobject::NO_READ,
      IOobject::NO_WRITE
      ),
     aMesh,
     dimensioned<vector>("Us", dimLength, vector::zero),
     "fixedValue"
     );

  const faPatch& apatch=adisplacement.boundaryField()[0].patch();

  vectorField disp=adisplacement.boundaryField()[0];
  scalarField dispn(disp.size(), 0.0);

  // set boundary conditions
  const labelList& mp=patch.meshPoints();
  Map<label> mpmap, bndpts;
  forAll(mp, i) 
    mpmap.insert(mp[i], i);
  forAll(ppatch.boundaryPoints(), i) 
    bndpts.insert(ppatch.meshPoints()[ppatch.boundaryPoints()[i]], i);
  forAll(apatch, ei)
    {
      const edge& e=ipatch.edges()[apatch[ei]];
      forAll(e, eci)
	{
	  label pl=ipatch.meshPoints()[e[eci]]; // global point id of faPatch-point
	  label lvi=mpmap[pl]; // local point id of former
	  if (!bndpts.found(pl)) // leave zero on boundary points
	    {
	      disp[ei]+=0.5*f[lvi];
	      dispn[ei]+=1.0;
	    }
	}
    }
    
  adisplacement.boundaryField()[0]==disp;
    
  for (label j=0; j<3; j++)
    {
      solve
	(
	 fam::laplacian(1.0, adisplacement)
	 );
    }

    /*
  {
    aMesh.write();
    adisplacement.write();
  }
  */

  // insert computed values into face displacement field
  forAll(faceLabels, fi)
    {
      label lfi=faceLabels[fi]-ppatch.start();
      fdisp[lfi]=adisplacement[fi];
    }
    
  tetPolyPatchInterpolationFaceDecomp ipol
    (
     refCast<const faceTetPolyPatchFaceDecomp>(patch)
    );

  return ipol.faceToPointInterpolate(vectorField(fdisp));
}

}
#endif

namespace Foam
{


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //



// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template
<
    template<class> class PatchField,
    class Mesh,
    class PointPatch,
    template<class> class MatrixType
>
FEMDisplacementPatchVectorField<PatchField,Mesh,PointPatch,MatrixType>::
FEMDisplacementPatchVectorField
(
 const PointPatch& p,
 const DimensionedField<vector, Mesh>& iF
)
:
  FixedValuePointPatchField
  <
   PatchField,
   Mesh,
   PointPatch,
   MatrixType,
   vector
  >(p, iF),
  FEMCaseDir_("."),
  //lengthScale_(1e3),
  pressureScale_(1e-3),
  minPressure_(-0.1),
  nSmoothIter_(4),
  wallCollisionCheck_(true),
  relax_(0.2),
  lateralRelax_(1.),
  prescribeMotionVelocity_(false),
  initialPosition_(this->patch().localPoints()),
  oldPressure_(this->pressurePatchSize(), 0.0),
  curTimeIndex_(-1)
{}


template
<
    template<class> class PatchField,
    class Mesh,
    class PointPatch,
    template<class> class MatrixType
>
FEMDisplacementPatchVectorField<PatchField,Mesh,PointPatch,MatrixType>::
FEMDisplacementPatchVectorField
(
 const PointPatch& p,
 const DimensionedField<vector, Mesh>& iF,
 const dictionary& dict
)
:
  FixedValuePointPatchField
  <
  PatchField,
  Mesh,
  PointPatch,
  MatrixType,
  vector
  >(p, iF, dict),
  FEMCaseDir_(dict.lookup("FEMCaseDir")),
  //lengthScale_(dict.lookupOrDefault<scalar>("lengthScale", 1e3)),
  /*
  transform_(dict.lookupOrDefault<septernion>
	     (
	      "transform", 
	      septernion(vector::zero, quaternion::I)
	     )),
	     */
  transform_(FSITransform::New(dict)),
  pressureScale_(dict.lookupOrDefault<scalar>("pressureScale", 1e-3)),
  minPressure_(dict.lookupOrDefault<scalar>("minPressure", -0.1)),
  nSmoothIter_(dict.lookupOrDefault<scalar>("nSmoothIter", 4)),
  wallCollisionCheck_(dict.lookupOrDefault<bool>("wallCollisionCheck", true)),
  relax_(dict.lookup("relax")),
  lateralRelax_(dict.lookupOrDefault<scalar>("lateralRelax", 1.0)),
  prescribeMotionVelocity_(dict.lookupOrDefault<Switch>
			   (
			    "prescribeMotionVelocity", 
			    false
			   )),
  initialPosition_(this->patch().localPoints()),
  oldPressure_(this->pressurePatchSize(), 0.0),
  curTimeIndex_(-1)
{
  if (dict.found("initialPosition"))
    {
      initialPosition_=vectorField("initialPosition", 
				   dict, 
				   this->patch().localPoints().size());
    }
  if (dict.found("oldPressure"))
    {
      oldPressure_=scalarField("oldPressure", 
			       dict, 
			       this->pressurePatchSize());
    }
}


template
<
    template<class> class PatchField,
    class Mesh,
    class PointPatch,
    template<class> class MatrixType
>
FEMDisplacementPatchVectorField<PatchField,Mesh,PointPatch,MatrixType>::
FEMDisplacementPatchVectorField
(
 const FEMDisplacementPatchVectorField& ppf,
 const PointPatch& p,
 const DimensionedField<vector, Mesh>& iF,
 const PointPatchFieldMapper& mapper
)
:
  FixedValuePointPatchField
  <
   PatchField,
   Mesh,
   PointPatch,
   MatrixType,
   vector
  >(ppf, p, iF, mapper),
  FEMCaseDir_(ppf.FEMCaseDir_),
  //lengthScale_(ppf.lengthScale_),
  transform_(ppf.transform_),
  pressureScale_(ppf.pressureScale_),
  minPressure_(ppf.minPressure_),
  nSmoothIter_(ppf.nSmoothIter_),
  wallCollisionCheck_(ppf.wallCollisionCheck_),
  relax_(ppf.relax_),
  lateralRelax_(ppf.lateralRelax_),
  prescribeMotionVelocity_(ppf.prescribeMotionVelocity_),
  initialPosition_(ppf.initialPosition_),
  oldPressure_(ppf.oldPressure_),
  curTimeIndex_(-1)
{}


template
<
    template<class> class PatchField,
    class Mesh,
    class PointPatch,
    template<class> class MatrixType
>
FEMDisplacementPatchVectorField<PatchField,Mesh,PointPatch,MatrixType>::
FEMDisplacementPatchVectorField
(
    const FEMDisplacementPatchVectorField& ppf
)
: FixedValuePointPatchField
  <
   PatchField,
   Mesh,
   PointPatch,
   MatrixType,
   vector
  >(ppf),
  FEMCaseDir_(ppf.FEMCaseDir_),
  //lengthScale_(ppf.lengthScale_),
  transform_(ppf.transform_),
  pressureScale_(ppf.pressureScale_),
  minPressure_(ppf.minPressure_),
  nSmoothIter_(ppf.nSmoothIter_),
  wallCollisionCheck_(ppf.wallCollisionCheck_),
  relax_(ppf.relax_),
  lateralRelax_(ppf.lateralRelax_),
  prescribeMotionVelocity_(ppf.prescribeMotionVelocity_),
  initialPosition_(ppf.initialPosition_),
  oldPressure_(ppf.oldPressure_),
  curTimeIndex_(-1)
{}


template
<
    template<class> class PatchField,
    class Mesh,
    class PointPatch,
    template<class> class MatrixType
>
FEMDisplacementPatchVectorField<PatchField,Mesh,PointPatch,MatrixType>::
FEMDisplacementPatchVectorField
(
    const FEMDisplacementPatchVectorField& ppf,
    const DimensionedField<vector, Mesh>& iF
)
: FixedValuePointPatchField
  <
   PatchField,
   Mesh,
   PointPatch,
   MatrixType,
   vector
  >(ppf, iF),
  FEMCaseDir_(ppf.FEMCaseDir_),
  //lengthScale_(ppf.lengthScale_),
  transform_(ppf.transform_),
  pressureScale_(ppf.pressureScale_),
  minPressure_(ppf.minPressure_),
  nSmoothIter_(ppf.nSmoothIter_),
  wallCollisionCheck_(ppf.wallCollisionCheck_),
  relax_(ppf.relax_),
  lateralRelax_(ppf.lateralRelax_),
  prescribeMotionVelocity_(ppf.prescribeMotionVelocity_),
  initialPosition_(ppf.initialPosition_),
  oldPressure_(ppf.oldPressure_),
  curTimeIndex_(-1)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


template
<
    template<class> class PatchField,
    class Mesh,
    class PointPatch,
    template<class> class MatrixType
>
void FEMDisplacementPatchVectorField<PatchField,Mesh,PointPatch,MatrixType>::
initEvaluate
(
 const Pstream::commsTypes commsType
)
{
  if (!Pstream::master())
  {
    FatalErrorIn("initEvaluate")
    <<"Parallel runs not supported!"
    <<abort(FatalError);
  }
  
  if (curTimeIndex_ != this->db().time().timeIndex())
    {

      Field<vector>& patchField = *this;

      const polyMesh& mesh = this->patch().boundaryMesh().mesh()();
      
      const fvPatchScalarField& presfield = pressure();
      const fvPatch& fvpatch = presfield.patch();
      
      // pressure field to use for FEM: current CFD solution, relaxed with pressure from previous FEM solution time
      scalarField curp = relax() * presfield  +  (1.-relax()) * oldPressure_;
      oldPressure_=presfield;

      primitivePatchInterpolation ipol(presfield.patch().patch());

      FEMCaseDir_.expand();
      if (exists(FEMCaseDir_))
	{
	  Info<<"Using scratch dir: "<<FEMCaseDir_<<endl;
	}
      else
	{
	  Info<<"Creating scratch dir: "<<FEMCaseDir_<<endl;
	  mkDir(FEMCaseDir_);
	}

      // write point locations (=mesh points)
      {
	fileName ulocfn(FEMCaseDir_/("locations."+this->patch().name()));
	OFstream f(ulocfn);
	for (label i=0; i<initialPosition_.size(); i++)
	  {
	    point p=transform_->locationCFDtoFEM(initialPosition_[i]);
	    f<<p.x()<<";"<<p.y()<<";"<<p.z()<<nl;
	  }
      }

      vector fp_cfd = vector::zero;
      vector fp_fem = vector::zero;
      // write pressure values (at points)
      {

	// compute resultant force for comparison
	forAll(curp, fi)
	  {
	    vector F=curp[fi] * fvpatch.Sf()[fi];
	    fp_cfd += F;
	    fp_fem += transform_->vectorCFDtoFEM(fvpatch.Cf()[fi], F);
	  }
	Info<<"Resultant force (CFD) = "<<fp_cfd<<endl;
	Info<<"Resultant force (FEM) = "<<fp_fem<<endl;

	fileName pfn(FEMCaseDir_/("pressure_values."+this->patch().name()));
	scalarField patch_p=ipol.faceToPointInterpolate(curp);
	OFstream f(pfn);
	for (label i=0; i<patch_p.size(); i++)
	  {
	    f<<pressureScale_*max(minPressure_, patch_p[i])<<nl;
	  }
	if (patch_p.size() < this->patch().localPoints().size())
	  {
	    // it is a face decomp patch field! Write face center values
	    for (label i=0; i<curp.size(); i++)
	      {
		f<<pressureScale_*max(minPressure_, curp[i])<<nl;
	      }
	  }
      }
      
      // write signal file and wait for it to be deleted
      {
	fileName sfn(FEMCaseDir_/("complete."+this->patch().name()));      
	{
	  OFstream f(sfn);
	  f << "COMPLETE" << endl;
	}
	// wait for file to disappear again
	do
	  {
	    sleep(1);
	  }
	while (exists(sfn));
      }
      
      // read in the displacements
      vectorField FEMdisplacement(this->size());
      vectorField rawdisplacement(this->size());
      vectorField smoothdisplacement(this->size());
      scalarField smoothdelta(this->size());
      vectorField curdisplacement(this->size());
      vectorField delta0(this->size()); // required mesh motion
      vectorField delta(this->size()); // final mesh motion (after wall collisison check)

      {
	fileName fn(FEMCaseDir_/("displacements."+this->patch().name()));
	IFstream file(fn);
	// read
	FEMdisplacement=vectorField(file);
	// apply transformation
	for(label i=0; i<rawdisplacement.size(); i++)
	  {
	    //disp[i]=transform_.r().invTransform(disp[i])/lengthScale_;
	    rawdisplacement[i]=transform_->vectorFEMtoCFD(initialPosition_[i], FEMdisplacement[i]);
	  }

	smoothdisplacement=rawdisplacement;
	// smooth
	Info<<"Smoothing: "<<flush;
	for (label j=0; j<nSmoothIter_/2; j++)
	  {
	    if (j>0 && (j%5==0)) Info<<j<<flush; else Info<<"."<<flush;
	    smoothdisplacement=smooth(fvpatch, this->patch(), smoothdisplacement);
	  }
	Info<<"done."<<endl;

	// extrapolate
	Info<<"Extrapolation into uncovered areas"<<endl;
	smoothdisplacement=extrapolate(fvpatch, this->patch(), smoothdisplacement);

	// smooth again
	Info<<"Smoothing again: "<<flush;
	for (label j=0; j<nSmoothIter_/2; j++)
	  {
	    if (j>0 && (j%5==0)) Info<<j<<flush; else Info<<"."<<flush;
	    smoothdisplacement=smooth(fvpatch, this->patch(), smoothdisplacement);
	  }
	Info<<"done."<<endl;

	smoothdelta=mag(smoothdisplacement-rawdisplacement);
	Info<<"min/max/avg delta: "
	    <<min(smoothdelta)
	    <<" / "<<max(smoothdelta)
	    <<" / "<<average(smoothdelta)<<endl;
	
      }

      Info<<"min/max/avg displacement: "
	  <<min(smoothdisplacement)
	  <<" / "<<max(smoothdisplacement)
	  <<" / "<<average(smoothdisplacement)<<endl;

      curdisplacement  = this->patch().localPoints() - initialPosition_;

      // required motion of current points to match FEM solution
      delta0 = smoothdisplacement-curdisplacement; 
      
      // relax motion
      Info<<"Relaxing displacement by "<<this->relax()<<endl;
      //delta *= this->relax();

      vectorField normals=this->patch().pointNormals();
      normals/=mag(normals);
      vectorField dn=normals*(normals&delta0);
      vectorField dt=delta0-dn;
      delta0 = this->relax()* (dn + (lateralRelax_*dt));

      delta = delta0;
      if (wallCollisionCheck_)
	{
	  labelHashSet p1;
	  forAll(mesh.boundaryMesh(), i) p1.insert(i);

	  triSurfaceMesh boundary
	    (
	      IOobject
	      (
	      "bnd",
	      mesh.time().timeName(),
	      mesh,
	      IOobject::NO_READ,
	      IOobject::NO_WRITE
	      ),
	      triSurface
	      (
	      triSurfaceTools::triangulate
	      (
		mesh.boundaryMesh(),
		p1
		)
	      )
	      );

	  label nPts=initialPosition_.size();
	  List<pointIndexHit> hits( nPts );
	  boundary.findLine
	    (
	      // start search at some distance from patch surface, 
	      // will find collision with start patch otherwise
	      this->patch().localPoints()+1e-3*delta,
	      this->patch().localPoints()+1.1*delta, 
	      hits
	      );

	  label cnt=0;
	  double min_h=4e-6; // 4mum
	  forAll(hits, i)
	    {
	      if (hits[i].hit())
		{
		  cnt++;
		  scalar maxd=mag(hits[i].hitPoint()-this->patch().localPoints()[i]);
		  //delta[i]*=0.9*maxd/mag(delta[i]);
		  delta[i] = (maxd - min_h)*delta[i]/mag(delta[i]); // leave a gap of min_h
		}
	    }

	  Info<<"Number of intersecting motions: "
	      <<cnt<<" ("<<1e2*scalar(cnt)/scalar(nPts)<<"%)"<<endl;

	}

	
      // finally set motion BC
      if (!prescribeMotionVelocity_)
	{
	  patchField=delta;
	}
      else
	{
	  patchField=
	    ( 
	      delta
	      /
	      mesh.time().deltaT().value()
	    );
	}


      if (filePtr_.empty())
	{
	  fileName probeDir;
	  fileName probeSubDir = "FSI_"+this->patch().name();
	  probeSubDir = probeSubDir/mesh.time().timeName();
	  probeDir = mesh.time().path()/probeSubDir;
	  mkDir(probeDir);
	  OFstream *fptr=new OFstream(probeDir/"fsi.dat");
	  Info<<"Creating file "<<fptr->name()<<endl;
	  filePtr_.reset(fptr);
	  filePtr_()
	    <<"# 1:time"
	    " 2,3,4: force (CFD)"
	    " 5,6,7: force (FEM)"
	    " 8:min(delta due to smoothing)"
	    " 9:max(delta)"
	    " 10:avg(delta)"
	    " 11,12,13:min(smoothed FEM displacement results)"
	    " 14,15,16:max(displacement)"
	    " 17,18,19:avg(displacement)"
	    " 20,21,22:min(current mesh displacement)"
	    " 23,24,25:max(current mesh displacement)"
	    " 26,27,28:avg(current mesh displacement)"
	    <<endl;
	}

      filePtr_()<<mesh.time().value();
      
      filePtr_()<<token::SPACE<<fp_cfd;
      filePtr_()<<token::SPACE<<fp_fem;
      
      filePtr_()<<token::SPACE<<min(smoothdelta)
		<<token::SPACE<<max(smoothdelta)
		<<token::SPACE<<average(smoothdelta);
	
      filePtr_()<<token::SPACE<<min(smoothdisplacement)
		<<token::SPACE<<max(smoothdisplacement)
		<<token::SPACE<<average(smoothdisplacement);

      filePtr_()<<token::SPACE<<min(curdisplacement)
		<<token::SPACE<<max(curdisplacement)
		<<token::SPACE<<average(curdisplacement);
		
      filePtr_()<<endl;

      if (this->db().time().outputTime())
      {
	IOobject oo
	(
	  "FSIGeometry_"+this->patch().name()+".vtk",
	  this->db().time().timeName(),
	  this->db().time(),
	  IOobject::NO_READ,
	  IOobject::AUTO_WRITE
	);
	insight::vtk::vtkModel2d vtk;
	vtk.setPoints
	(
	  this->size(), 
	  initialPosition_.component(vector::X)().data(),
	  initialPosition_.component(vector::Y)().data(),
	  initialPosition_.component(vector::Z)().data()
	);
	const polyPatch& ppatch = fvpatch.patch();
	for(label fi=0; fi<ppatch.size(); fi++)
	{
	  const face& f = ppatch.localFaces()[fi];
	  vtk.appendPolygon(f.size(), f.cdata());
	}
	{
	  vectorField tp(initialPosition_);
	  forAll(tp, pi)
	    tp[pi]=transform_->locationCFDtoFEM(tp[pi]);
	  vtk.appendPointVectorField
	  (
	    "FEMpoints", 
	    tp.component(vector::X)().cdata(),
	    tp.component(vector::Y)().cdata(),
	    tp.component(vector::Z)().cdata()
	  );
	}
	vtk.appendPointVectorField
	(
	  "sol_DEPL_Vector", 
	  FEMdisplacement.component(vector::X)().cdata(),
	  FEMdisplacement.component(vector::Y)().cdata(),
	  FEMdisplacement.component(vector::Z)().cdata()
	);
	vtk.appendPointVectorField
	(
	  "rawdisplacement", 
	  rawdisplacement.component(vector::X)().cdata(),
	  rawdisplacement.component(vector::Y)().cdata(),
	  rawdisplacement.component(vector::Z)().cdata()
	);
	vtk.appendPointVectorField
	(
	  "smoothdisplacement", 
	  smoothdisplacement.component(vector::X)().cdata(),
	  smoothdisplacement.component(vector::Y)().cdata(),
	  smoothdisplacement.component(vector::Z)().cdata()
	);
	vtk.appendPointVectorField
	(
	  "delta0", 
	  delta0.component(vector::X)().cdata(),
	  delta0.component(vector::Y)().cdata(),
	  delta0.component(vector::Z)().cdata()
	);
	vtk.appendPointVectorField
	(
	  "delta", 
	  delta.component(vector::X)().cdata(),
	  delta.component(vector::Y)().cdata(),
	  delta.component(vector::Z)().cdata()
	);
	{
	  Info<<"Writing "<<oo.objectPath()<<endl;
	  std::ofstream f(oo.objectPath().c_str());
	  vtk.writeLegacyFile(f);
	}
      }
      curTimeIndex_ = this->db().time().timeIndex();
    }

  FixedValuePointPatchField
    <
      PatchField,
      Mesh,
      PointPatch,
      MatrixType,
      vector
    >::initEvaluate(commsType);
}


template
<
    template<class> class PatchField,
    class Mesh,
    class PointPatch,
    template<class> class MatrixType
>
void FEMDisplacementPatchVectorField<PatchField,Mesh,PointPatch,MatrixType>::
write(Ostream& os) const
{
  FixedValuePointPatchField
    <
      PatchField,
      Mesh,
      PointPatch,
      MatrixType,
      vector
    >::write(os);

  os.writeKeyword("FEMCaseDir") << FEMCaseDir_
				<< token::END_STATEMENT << nl;
				
  transform_->writeEntry(os);
  
  os.writeKeyword("pressureScale") << pressureScale_
				   << token::END_STATEMENT << nl;
  os.writeKeyword("minPressure") << minPressure_
				 << token::END_STATEMENT << nl;
  os.writeKeyword("nSmoothIter") << nSmoothIter_
				 << token::END_STATEMENT << nl;
  os.writeKeyword("wallCollisionCheck") << wallCollisionCheck_
					<< token::END_STATEMENT << nl;

  os.writeKeyword("relax");
  relax_.write(os);
  os<< token::END_STATEMENT << nl;

  os.writeKeyword("lateralRelax") << lateralRelax_
				  << token::END_STATEMENT << nl;
  os.writeKeyword("prescribeMotionVelocity") << prescribeMotionVelocity_
					     << token::END_STATEMENT << nl;

  initialPosition_.writeEntry("initialPosition", os);

  oldPressure_.writeEntry("oldPressure", os);
}



// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
