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

#include "inflowGeneratorFvPatchVectorField.H"
#include "transform.H"
#include "transformField.H"
#include "volFields.H"
#include "ListListOps.H"
#include "PstreamReduceOps.H"
#include "PstreamCombineReduceOps.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "vtkSurfaceWriter.H"

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
tmp<Field<TurbulentStructure> > 
inflowGeneratorFvPatchVectorField<TurbulentStructure>::filterVortons
(
    const inflowGeneratorFvPatchVectorField<TurbulentStructure>& ptf,
    const fvPatchFieldMapper& mapper,
    const Field<TurbulentStructure>& vlist
) const
{
  tmp<Field<TurbulentStructure> > rnewlist(new Field<TurbulentStructure>());
  Field<TurbulentStructure>& newlist=rnewlist();
  
  if (mapper.direct() && &mapper.directAddressing() && mapper.directAddressing().size())
  {
    const 
#ifdef OF16ext
    unallocLabelList
#else
    labelUList
#endif
    & addr=mapper.directAddressing();
    HashTable<label, label> inverseAddr;
    forAll(addr, j)
    {
      inverseAddr.insert(addr[j], j);
    }
  
    if (debug) Info<<"mapinfo: "<<mapper.direct()<<", "<<size()<<", "<<mapper.directAddressing().size()<<", "<<inverseAddr.size()<<endl;;
    
    forAll(vlist, i)
    {
      const TurbulentStructure& ov=vlist[i];
      if (inverseAddr.found(ov.creaFace()))
      {
	label myf=inverseAddr[ov.creaFace()];
	TurbulentStructure nv(ov);
	nv.setCreaFace(myf);
	newlist.resize(newlist.size()+1);
	newlist[newlist.size()-1]=nv;
      }
    }
    if (debug) Info<<"Kept "<<newlist.size()<<" vortons."<<endl;
  }
  else
  {
    // create new vortons, if meshes don't match
  }
  
  return rnewlist;
}

// this is called during decomposePar
template<class TurbulentStructure>
inflowGeneratorFvPatchVectorField<TurbulentStructure>::inflowGeneratorFvPatchVectorField
(
    const inflowGeneratorFvPatchVectorField<TurbulentStructure>& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    inflowGeneratorBaseFvPatchVectorField(ptf, p, iF, mapper),
    vortons_(filterVortons(ptf, mapper, ptf.vortons_)),
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
    
    // insert vortons and correct their creaFace member
    {
      if (debug) Info<<"mapinfo: "<<size()<<", "<<addr.size()<<endl;
      
      const Field<TurbulentStructure>& vlist=tiptf.vortons_;
      forAll(vlist, i)
      {
	const TurbulentStructure& ov=vlist[i];
	label myf=addr[ov.creaFace()];
	TurbulentStructure nv(ov);
	nv.setCreaFace(myf);
	vortons_.resize(vortons_.size()+1);
	vortons_[vortons_.size()-1]=nv;
      }
      if (debug) Info<<"Vorton list now at size "<<vortons_.size()<<endl;
    }
      
    if (tiptf.tau_)
    {
      if (!tau_) tau_.reset(new scalarField(size(), 0.0));
      tau_->rmap(*(tiptf.tau_), addr);
    }
    
    if (tiptf.crTimes_) 
    {
      if (!crTimes_) crTimes_.reset(new scalarField(size(), 0.0));
      crTimes_->rmap(*(tiptf.crTimes_), addr);
    }
    
    structureParameters_.rmap(ptf, addr);
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
    
    // get (already clipped) length scales
    scalar L1=mag(esa_[fi].c1()), L2=mag(esa_[fi].c2()), L3=mag(esa_[fi].c3());
    
    (*tau_)[fi] = L1*L2*L3 / (c_[fi] * patch().magSf()[fi] * (mag(Umean)+SMALL) );
  }

  Info<<"Average tau = "<<average(*tau_)<<" / min="<<min(*tau_)<<" / max="<<max(*tau_)<<endl;
  Info<<"Max spots per timestep: "<<(this->db().time().deltaTValue() / min(*tau_))<<endl;
}



template<class TurbulentStructure>
tmp<vectorField> inflowGeneratorFvPatchVectorField<TurbulentStructure>::continueFluctuationProcess(scalar t, ProcessStepInfo *info)
{

  {
    label nclip1=0;
    bool init_needed=false;
    
    if (size() != esa_.size())
    {
      scalarField maxEdgeL(maxEdgeLengths());
      
      esa_.setSize(size());
      forAll(*this, fi)
      {
	scalar minL=2.*maxEdgeL[fi];
	esa_.set(fi, new ESAnalyze(L_[fi]));
	if (esa_[fi].clip(minL)) nclip1++;	
      }
      init_needed=true;
    }
    
    reduce(init_needed, orOp<bool>());
    
    if (init_needed)
    {
      reduce(nclip1, sumOp<label>());
      
      if (nclip1)
	Info<<" Inflow generator ["<<patch().name()<<"]: Extended local length scale to minimum length "<<nclip1<<" time(s)."<<endl;
    }
  }


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
  
  label nclip2=0;
  //scalar dt=this->db().time().deltaT().value();
  vector Umean;
  if (uniformConvection_) Umean=averageMeanVelocity();
  
  forAll(*this, fi)
  {
    vector in_dir = -patch().Sf()[fi]/patch().magSf()[fi];
    
    scalar Lalong=esa_[fi].Lalong(in_dir);
    
    if (!uniformConvection_) Umean=Umean_[fi];
    
    if ((Umean&in_dir) < SMALL)
    {
      Umean += in_dir*(-(Umean&in_dir)+SMALL);
      nclip2++;
    }
    
    scalar horiz = t + 0.5*Lalong/(SMALL+mag(Umean)) + this->db().time().deltaT().value();

      // if creation time is within the current time step then create structure now
//       if (debug>=2) Info<<fi<<" cf="<<patch().Cf()[fi]<<" : umean="<<Umean<<"/L="<<L_[fi]<<" Ldiag="<<L<<" Lmax="<<Lmax<<" "<<flush;
    
    while ( (horiz - (*crTimes_)[fi]) > /*dt*/ 0.0 )
    {
      point pf = randomFacePosition(fi); ;
      
      TurbulentStructure snew(ranGen_, pf, -Umean*( (*crTimes_)[fi] - t ), Umean, esa_[fi].Leig(), fi);
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
  
  reduce(nclip2, sumOp<label>());
  
  if (nclip2)
    Info<<" Inflow generator ["<<patch().name()<<"]: Increased local inflow velocity to SMALL "<<nclip2<<" time(s)."<<endl;
  //autoPtr<indexedOctree<treeDataPoint> > tree = buildTree();


  if (debug) Info<<"Generating fluctuations."<<endl;
  /**
    * ==================== Generation of turbulent fluctuations ========================
    */
  scalarField cglob(globalPatch_->size(), 0.0);
  globalPatch_->insertLocalFaceValues(c_, cglob);

  reduce(cglob, sumOp<scalarField>());

  vectorField gfluc(globalPatch_->size(), vector::zero);

  RecursiveApply<TurbulentStructure, globalPatch> apl(globalPatch_(), cglob, structureParameters_, gfluc);
  labelList n_affected(vortons_.size(), 0);
  forAll(vortons_, j)
  {
    label gfi=globalPatch_->toGlobalFaceI(vortons_[j].creaFace());
    n_affected[j]=apl.apply(vortons_[j], gfi);
    //Pout<<"vorton #"<<j<<": "<<vortons_[j].creaFace()<<" ("<<gfi<<") affected n="<<n_affected<<endl;
  }
  Info<<"n_affected: min="<<gMin(n_affected)<<" / max="<<gMax(n_affected)<<" / avg="<<gAverage(n_affected)<<endl;
  
#ifndef OF16ext
  if ( (debug>0) && (patch().boundaryMesh().mesh().time().outputTime()) )
  {
    vtkSurfaceWriter().write
    (
      this->patch().boundaryMesh().mesh().time().path() /
      this->patch().boundaryMesh().mesh().time().timeName(), 
      "globalPatch", 
      globalPatch_().points(), 
      globalPatch_(),
      
      "u",
      gfluc,
      false,
      true
    );
  }
#endif
    
  // Make fluctuations global
  reduce(gfluc, sumOp<vectorField>());
  
#ifndef OF16ext
  if ( (debug>0) && (patch().boundaryMesh().mesh().time().outputTime()) )
  {
    vtkSurfaceWriter().write
    (
      this->patch().boundaryMesh().mesh().time().path() /
      this->patch().boundaryMesh().mesh().time().timeName(), 
      "globalPatch_globalizedFluctuations", 
      globalPatch_().points(), 
      globalPatch_(),
      
      "u",
      gfluc,
      false,
      true
    );
  }
#endif

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
