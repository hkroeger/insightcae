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

Class
    inflowGeneratorFvPatchVectorField

Description
    Generates turbulent fluctuations using Nikolai Kornev's method of turbulent 
    spots.

    Parallelisation is archieved by holding the vorton list on the master
    processor and distribute the modified list on each timestep to all
    slave processors.

SourceFiles


\*---------------------------------------------------------------------------*/

#include "inflowGeneratorFvPatchVectorField.H"
#include "transform.H"
#include "transformField.H"
#include "volFields.H"
#include "ListListOps.H"
#include "PstreamReduceOps.H"
#include "PstreamCombineReduceOps.H"
#include "addToRunTimeSelectionTable.H"

#include "base/vtktools.h"

#include <vector>
#include "boost/lexical_cast.hpp"
#include "boost/foreach.hpp"
#include "boost/iterator/counting_iterator.hpp"

using namespace std;
using namespace boost;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::computeConditioningFactor()
{

  vectorField uMean(size(), vector::zero);
  symmTensorField uPrime2Mean(size(), symmTensor::zero);
//   scalar N=0.0;
  label N_total=0;
  scalar A=gSum(patch().magSf()), V=0.0;
  
  Info<<"A="<<A<<endl;
  
  scalar dt=this->db().time().deltaTValue();
  
  for (int i=1; i<100000; i++)
  {
    scalar t=dt*scalar(i-1);
    
    ProcessStepInfo info;
    vectorField u( continueFluctuationProcess(t, &info) );
    N_total+=info.n_generated;
    V += gSum( (-patch().Sf()&Umean()) * dt );
    
    scalar alpha = scalar(i - 1)/scalar(i);
    scalar beta = 1.0/scalar(i);
    
    uPrime2Mean += sqr(uMean);
    uMean = alpha*uMean + beta*u;
//     N = alpha*N + beta*scalar(info.n_induced);
    uPrime2Mean = alpha*uPrime2Mean + beta*sqr(u) - sqr(uMean); //uMean shoudl be zero
    
    Info<<"Averages: uMean="
	<<gSum(uMean*patch().magSf())/gSum(patch().magSf())
	<<" \t R^2="
	<<gSum(uPrime2Mean*patch().magSf())/gSum(patch().magSf())
	/*<< "\t N="<<N*/<<"\t N_tot="<<N_total<<"\t V="<<V<< endl;
		    
    if (i%1000==0) writeStateVisualization(i, u, &uMean, &uPrime2Mean);
  }
  
  FatalErrorIn("computeConditioningFactor") << "STOP" << abort(FatalError);
}


template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::writeStateVisualization
(
  int i,
  const vectorField& u,
  const vectorField* uMean,
  const symmTensorField* uPrime2Mean
) const
{
  insight::vtk::vtkModel vtk_vortons;
  
  vtk_vortons.setPoints(
    vortons_.size(),
    vortons_.component(vector::X)().data(),
    vortons_.component(vector::Y)().data(),
    vortons_.component(vector::Z)().data()
    );
  
  for (label k=0; k<3; k++)
  {
    std::vector<double> Lx, Ly, Lz;
    forAll(vortons_, j)
    {
      const vector& L = vortons_[j].L(k);
      Lx.push_back(L.x()); Ly.push_back(L.y()); Lz.push_back(L.z());
    }
    vtk_vortons.appendPointVectorField("L"+lexical_cast<string>(k), Lx.data(), Ly.data(), Lz.data());
  }
  
  insight::vtk::vtkModel2d vtk_patch;
  // set cells
  const polyPatch& ppatch = patch().patch();
  vtk_patch.setPoints
  (
    ppatch.localPoints().size(), 
    ppatch.localPoints().component(vector::X)().data(),
    ppatch.localPoints().component(vector::Y)().data(),
    ppatch.localPoints().component(vector::Z)().data()
  );
  for(label fi=0; fi<ppatch.size(); fi++)
  {
    const face& f = ppatch.localFaces()[fi];
    vtk_patch.appendPolygon(f.size(), f.cdata());
  }
  
  vtk_patch.appendCellVectorField
  (
    "u", 
    u.component(vector::X)().cdata(),
    u.component(vector::Y)().cdata(),
    u.component(vector::Z)().cdata()
  );
  if (uMean)
  {
    vtk_patch.appendCellVectorField
    (
      "uMean", 
      uMean->component(vector::X)().cdata(),
      uMean->component(vector::Y)().cdata(),
      uMean->component(vector::Z)().cdata()
    );
  }
  if (uPrime2Mean)
  {
    vtk_patch.appendCellTensorField
    (
      "R", 
      uPrime2Mean->component(symmTensor::XX)().cdata(),
      uPrime2Mean->component(symmTensor::XY)().cdata(),
      uPrime2Mean->component(symmTensor::XZ)().cdata(),
      uPrime2Mean->component(symmTensor::XY)().cdata(),
      uPrime2Mean->component(symmTensor::YY)().cdata(),
      uPrime2Mean->component(symmTensor::YZ)().cdata(),
      uPrime2Mean->component(symmTensor::XZ)().cdata(),
      uPrime2Mean->component(symmTensor::YZ)().cdata(),
      uPrime2Mean->component(symmTensor::ZZ)().cdata()
    );
  }
  {
    
    IOobject oo
    (
      "vortons_"+this->patch().name()+"_"+lexical_cast<string>(i)+".vtk",
      this->db().time().timeName(),
      this->db().time(),
      IOobject::NO_READ,
      IOobject::AUTO_WRITE
    );
    IOobject oop
    (
      "patch_"+this->patch().name()+"_"+lexical_cast<string>(i)+".vtk",
      this->db().time().timeName(),
      this->db().time(),
      IOobject::NO_READ,
      IOobject::AUTO_WRITE
    );
    mkDir(oo.path());
    
    Info<<"Writing "<<oo.objectPath()<<endl;
    std::ofstream f(oo.objectPath().c_str());
    vtk_vortons.writeLegacyFile(f);
    f.close();
    
    Info<<"Writing "<<oop.objectPath()<<endl;
    std::ofstream f2(oop.objectPath().c_str());
    vtk_patch.writeLegacyFile(f2);
    f2.close();
  }
}




// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class TurbulentStructure>
inflowGeneratorFvPatchVectorField<TurbulentStructure>::inflowGeneratorFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    inflowGeneratorBaseFvPatchVectorField(p, iF),
    vortons_(),
    structureParameters_()
{
}

template<class TurbulentStructure>
inflowGeneratorFvPatchVectorField<TurbulentStructure>::inflowGeneratorFvPatchVectorField
(
    const inflowGeneratorFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    inflowGeneratorBaseFvPatchVectorField(ptf, p, iF, mapper),
    vortons_(ptf.vortons_),
    tau_(ptf.tau_?new scalarField(*(ptf.tau_), mapper):NULL),
    crTimes_(ptf.crTimes_?new scalarField(*(ptf.crTimes_), mapper):NULL),
    structureParameters_(ptf.structureParameters_)
{
}

template<class TurbulentStructure>
inflowGeneratorFvPatchVectorField<TurbulentStructure>::inflowGeneratorFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    inflowGeneratorBaseFvPatchVectorField(p, iF, dict),
    structureParameters_(dict)
{
    if (dict.found("vortons"))
    {
        //vortons_=SLList<TurbulentStructure>(dict.lookup("vortons"));
      vortons_=Field<TurbulentStructure>(dict.lookup("vortons"));
    }
    if (dict.found("crTimes"))
    {
      crTimes_.reset
      (
	new scalarField("crTimes", 
			dict, 
			this->size())
      );
    }
}

template<class TurbulentStructure>
inflowGeneratorFvPatchVectorField<TurbulentStructure>::inflowGeneratorFvPatchVectorField
(
    const inflowGeneratorFvPatchVectorField& ptf
)
: inflowGeneratorBaseFvPatchVectorField(ptf),
  vortons_(ptf.vortons_),
  tau_(ptf.tau_?new scalarField(*(ptf.tau_)):NULL),
  crTimes_(ptf.crTimes_?new scalarField(*(ptf.crTimes_)):NULL),
  structureParameters_(ptf.structureParameters_)
{}

template<class TurbulentStructure>
inflowGeneratorFvPatchVectorField<TurbulentStructure>::inflowGeneratorFvPatchVectorField
(
    const inflowGeneratorFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
: inflowGeneratorBaseFvPatchVectorField(ptf, iF),
  vortons_(ptf.vortons_),
  tau_(ptf.tau_?new scalarField(*(ptf.tau_)):NULL),
  crTimes_(ptf.crTimes_?new scalarField(*(ptf.crTimes_)):NULL),
  structureParameters_(ptf.structureParameters_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::autoMap
(
    const fvPatchFieldMapper& m
)
{
    inflowGeneratorBaseFvPatchVectorField::autoMap(m);
    if (tau_) tau_->autoMap(m);
    if (crTimes_) crTimes_->autoMap(m);
    structureParameters_.autoMap(m);
}


template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::rmap
(
    const fvPatchField<vector>& ptf,
    const labelList& addr
)
{
    inflowGeneratorBaseFvPatchVectorField::rmap(ptf, addr);
    const inflowGeneratorFvPatchVectorField<TurbulentStructure>& tiptf =
      refCast<const inflowGeneratorFvPatchVectorField<TurbulentStructure> >(ptf);
    if (tau_) tau_->rmap(*(tiptf.tau_), addr);
    if (crTimes_) crTimes_->rmap(*(tiptf.crTimes_), addr);
    structureParameters_.rmap(ptf, addr);
}

template<class TurbulentStructure>
scalar inflowGeneratorFvPatchVectorField<TurbulentStructure>::computeMinOverlap
(
  const autoPtr<indexedOctree<treeDataPoint> >& tree, 
  const TurbulentStructure& snew
) const
{
  if (tree.valid())
  {
    scalar nearestDistSqr=GREAT;
    pointIndexHit r = tree().findNearest(snew, nearestDistSqr);
    if (r.hit())
    {
      vector d = r.hitPoint() - snew;
      scalar l1 = snew.Lalong(d);
      scalar l2 = vortons_[r.index()].Lalong(-d);
      scalar ovl = ( 0.5*(l1+l2) - mag(d) ) / (Foam::min(l1, l2));

      return ovl;
    }
  }
  return 0;
}

template<class TurbulentStructure>
point inflowGeneratorFvPatchVectorField<TurbulentStructure>::randomFacePosition(label fi)
{
  const pointField& pts = patch().patch().localPoints();
  const face& f=patch().patch().localFaces()[fi];
 
  faceList tris(f.nTriangles());
  label tril=0;
  f.triangles(pts, tril, tris); // split face into triangles
  
  scalar A_tot=0.0;
  std::vector<double> A_f;
  forAll(tris, ti)
  {
    scalar A=tris[ti].mag(pts);
    A_tot+=A;
    A_f.push_back(A);
  }
  
  // setup discrete random generator with area-weighted probability

  boost::random::discrete_distribution<> tri_selector(A_f.begin(), A_f.end());
  label i = tri_selector(ranGen_);

  const face& t=tris[i];
  
  // choose random triangle coordinates inside selected triangle
  scalar u=ranGen_(), v=ranGen_();
  return pts[t[0]]*u + pts[t[1]]*v + pts[t[2]]*(1.-u-v); // compute location from u and v
}

template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::computeTau()
{
  tau_.reset(new scalarField(size(), 0.0));
  
  vector Umean=vector::zero;
  if (uniformConvection_) Umean=averageMeanVelocity();
  
  forAll(*this, fi)
  {
    if (!uniformConvection_) Umean=Umean_[fi];
    
    vector L=eigenValues(L_[fi]);
    scalar minL=sqrt(patch().magSf()[fi]);
    (*tau_)[fi] = max(minL,L.x())*max(minL,L.y())*max(minL,L.z()) 
		    / (c_[fi] * patch().magSf()[fi] * (mag(Umean)+SMALL) );
  }

  Info<<"Average tau = "<<average(*tau_)<<" / min="<<min(*tau_)<<" / max="<<max(*tau_)<<endl;
  Info<<"Max spots per timestep: "<<(this->db().time().deltaTValue() / min(*tau_))<<endl;
}


template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::induceInNeighbours
(
  vectorField& fluctuations, 
  TurbulentStructure& v, 
  const typename TurbulentStructure::StructureParameters& sp, 
  label faceI, 
  labelList& visited,
  label& depth
) const
{
  depth++;
  if (faceI<0) return;
  
  vector u = v.fluctuation(sp, patch().Cf()[faceI]);
  
  if (mag(u)>SMALL)
  {
    
    fluctuations[faceI] += u / sqrt(c_[faceI]);
    visited[faceI]=1;
    
    forAll(this->patch().patch().faceFaces()[faceI], j)
    {
      label nfi=this->patch().patch().faceFaces()[faceI][j];
      if (!visited[nfi])
      {
	induceInNeighbours(fluctuations, v, sp, nfi, visited, depth);
      }
    }
  }
}

template<class TurbulentStructure>
tmp<vectorField> inflowGeneratorFvPatchVectorField<TurbulentStructure>::continueFluctuationProcess(scalar t, ProcessStepInfo *info)
{

    if (!tau_.get())
    {
      computeTau();
    }
    
    if (!globalPatch_.valid())
    {
      globalPatch_.reset(new globalPatch(this->patch().patch()));
    }
    
    if (!crTimes_.get())
    {
      Info<<"reset crTimes"<<endl;
      crTimes_.reset(new scalarField(size(), this->db().time().value()));
      scalar rnum=ranGen_();
      forAll((*crTimes_), fi)
	(*crTimes_)[fi] += (1.0 - 2.0*rnum)*(*tau_)[fi];
    }
    
    /**
     * ==================== Generation of new turbulent structures ========================
     */    
    label n_generated=0;

    
    /**
     * Creation of new spots
     */
    
    if (debug) Info<<"Generating new spots"<<endl;
    
    label nclip1=0, nclip2=0;
    //scalar dt=this->db().time().deltaT().value();
    vector Umean;
    if (uniformConvection_) Umean=averageMeanVelocity();
    forAll(*this, fi)
    {
      scalar minL=sqrt(patch().magSf()[fi]);
      vector L=eigenValues(L_[fi]);
      scalar Lmax=max(L.x(), max(L.y(), L.z()));
      if (!uniformConvection_) Umean=Umean_[fi];
      vector in_dir = -patch().Sf()[fi]/patch().magSf()[fi];
      
      if ((Umean&in_dir) < SMALL)
      {
	Umean += in_dir*(-(Umean&in_dir)+SMALL);
	nclip2++;
      }
      
      if (Lmax<minL)
      {
	Lmax=minL;
	nclip1++;
      }
      scalar horiz = t + 0.5*Lmax/(SMALL+mag(Umean)) + this->db().time().deltaT().value();
 
       // if creation time is within the current time step then create structure now
      if (debug>=2) Info<<fi<<" cf="<<patch().Cf()[fi]<<" : umean="<<Umean<<"/L="<<L_[fi]<<" Ldiag="<<L<<" Lmax="<<Lmax<<" "<<flush;
      
      while ( (horiz - (*crTimes_)[fi]) > /*dt*/ 0.0 )
      {
	point pf = randomFacePosition(fi); ;
	
	TurbulentStructure snew(ranGen_, pf, -Umean*( (*crTimes_)[fi] - t ), Umean, L_[fi], minL, fi);
	snew.randomize(ranGen_);
	
	// append new structure to the end of the list
	vortons_.resize(vortons_.size()+1);
	vortons_[vortons_.size()-1]=snew;
	
	if (debug>=2) Info<<"."<<pf<<" "<<flush;
	n_generated++;
	
	scalar rnum=ranGen_();
	(*crTimes_)[fi] += 2.0*rnum*(*tau_)[fi];
	
	//Info<<(*crTimes_)[fi]<<" "<<rnum<<" "<<(*tau_)[fi]<<" "<<horiz<<endl;
      }
      
      if (debug>=2) Info<<endl;
    }
    
    reduce(nclip1, sumOp<label>());
    reduce(nclip2, sumOp<label>());
    
    if (nclip1)
      Info<<" Inflow generator ["<<patch().name()<<"]: Extended local length scale to face size "<<nclip1<<" time(s)."<<endl;
    if (nclip2)
      Info<<" Inflow generator ["<<patch().name()<<"]: Increased local inflow velocity to SMALL "<<nclip2<<" time(s)."<<endl;
    //autoPtr<indexedOctree<treeDataPoint> > tree = buildTree();


    if (debug) Info<<"Generating fluctuations."<<endl;
    /**
     * ==================== Generation of turbulent fluctuations ========================
     */
    
    vectorField gfluc(globalPatch_->size());
    
    RecursiveApply<TurbulentStructure, globalPatch> apl(globalPatch_(), c_, structureParameters_, gfluc);
    forAll(vortons_, j)
    {
      label n_affected=apl.apply(vortons_[j], globalPatch_->toGlobalFaceI(vortons_[j].creaFace(), Pstream::myProcNo()));
    }
    
    // Make fluctuations global
    combineReduce(gfluc, sumOp<vectorField>());
    
    tmp<vectorField> tfluctuations=globalPatch_->extractLocalFaceValues(gfluc);
    vectorField& fluctuations = tfluctuations();
    
    
    if (debug) Info<<"Convecting and removing structures."<<endl;
    forAll(vortons_, j)
    {
      // convect structure
      vortons_[j].moveForward(this->db().time().deltaT().value());
    }
    
    Field<TurbulentStructure> os(vortons_);
    label kept=0;
    forAll(vortons_, j)
    {
      if (!vortons_[j].passedThrough())
      {
	vortons_[kept++]=os[j];
      }
    }
    vortons_.resize(kept);
    label n_removed=os.size()-kept;
    label n_total=vortons_.size();

    reduce(n_generated, sumOp<label>());
    reduce(n_removed, sumOp<label>());
    reduce(n_total, sumOp<label>());
    //reduce(n_induced, sumOp<label>());
    
    Info<<" Inflow generator ["<<patch().name()<<"]: "
      "Generated "<<n_generated<<
      ", removed "<<n_removed<<
      " now total "<<n_total<<
      //", contributions by "<<n_induced<<
      endl;

    if (info) 
    {
      //info->n_induced=n_induced;
      info->n_removed=n_removed;
      info->n_generated=n_generated;
      info->n_total=n_total;
    }

  if (debug>=3)
  {
   Pout<<" fluctuations: min/max/avg = "<<min(fluctuations)<<" / "<<max(fluctuations) << " / "<<average(fluctuations)<<endl;
   forAll(fluctuations, j)
    if (mag(fluctuations[j])>1e3) Pout<<j<<": "<<tfluctuations<<endl;
  }

  return tfluctuations;
}


template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::write(Ostream& os) const
{
    structureParameters_.write(os);
    if (crTimes_.get())
    {
        crTimes_->writeEntry("crTimes", os);
    }
    os.writeKeyword("vortons") << vortons_ << token::END_STATEMENT <<nl;
    
    inflowGeneratorBaseFvPatchVectorField::write(os);
}

} // End namespace Foam

// ************************************************************************* //
