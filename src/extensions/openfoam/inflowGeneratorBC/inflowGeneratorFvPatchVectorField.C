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
#include "addToRunTimeSelectionTable.H"

#include "base/vtktools.h"

#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::computeConditioningFactor()
{

  vectorField uMean(size(), vector::zero);
  tensorField uPrime2Mean(size(), tensor::zero);
  
  for (int i=0; i<50; i++)
  {
    
    vectorField u( continueFluctuationProcess() );
    
    scalar alpha = scalar(i - 1)/scalar(i);
    scalar beta = 1.0/scalar(i);
    
    uMean = alpha*uMean + beta*u;
    uPrime2Mean = alpha*uPrime2Mean + beta*sqr(u) - sqr(uMean);
    
    Info<<"Averages: uMean="<<average(uMean)<<" \t R^2="<< average(uPrime2Mean) << endl;
		
    insight::vtk::vtkModel vtk;
    
    vtk.setPoints(
      vortons_.size(),
      vortons_.component(vector::X)().data(),
      vortons_.component(vector::Y)().data(),
      vortons_.component(vector::Z)().data()
      );
    
    {
      IOobject oo
      (
	"vortons_"+this->patch().name()+"_"+lexical_cast<string>(i)+".vtk",
	this->db().time().timeName(),
	this->db().time(),
	IOobject::NO_READ,
	IOobject::AUTO_WRITE
      );
      Info<<"Writing "<<oo.objectPath()<<endl;
      mkDir(oo.path());
      std::ofstream f(oo.objectPath().c_str());
      vtk.writeLegacyFile(f);
      f.close();
    }
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
    fixedValueFvPatchField<vector>(p, iF),
    ranGen_(1),
    vortons_(),
    Umean_(),
    R_(),
    L_(),
    structureParameters_(),
    conditioningFactor_(),
    overlap_(0.5),
    curTimeIndex_(-1)
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
    fixedValueFvPatchField<vector>(ptf, p, iF, mapper),
    ranGen_(1),
    vortons_(ptf.vortons_),
    Umean_(ptf.Umean_),
    R_(ptf.R_),
    L_(ptf.L_),
    structureParameters_(ptf.structureParameters_),
    conditioningFactor_(ptf.conditioningFactor_),
    overlap_(ptf.overlap_),
    curTimeIndex_(ptf.curTimeIndex_)
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
    fixedValueFvPatchField<vector>(p, iF, dict),
    ranGen_(1),
    Umean_("Umean", dict, size()),
    R_("R", dict, size()),
    L_("L", dict, size()),
    structureParameters_(dict),
    overlap_(dict.lookupOrDefault<scalar>("overlap", 0.5)),
    curTimeIndex_(-1)
{
    if (dict.found("vortons"))
    {
        //vortons_=SLList<TurbulentStructure>(dict.lookup("vortons"));
      vortons_=Field<TurbulentStructure>(dict.lookup("vortons"));
    }
    if (dict.found("conditioningFactor"))
    {
        conditioningFactor_.reset
        (
	  new scalarField("conditioningFactor", 
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
: fixedValueFvPatchField<vector>(ptf),
  ranGen_(1),
  vortons_(ptf.vortons_),
  Umean_(ptf.Umean_),
  R_(ptf.R_),
  L_(ptf.L_),
  structureParameters_(ptf.structureParameters_),
  conditioningFactor_(ptf.conditioningFactor_),
  overlap_(ptf.overlap_),
  curTimeIndex_(ptf.curTimeIndex_)
{}

template<class TurbulentStructure>
inflowGeneratorFvPatchVectorField<TurbulentStructure>::inflowGeneratorFvPatchVectorField
(
    const inflowGeneratorFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
: fixedValueFvPatchField<vector>(ptf, iF),
  ranGen_(1),
  vortons_(ptf.vortons_),
  Umean_(ptf.Umean_),
  R_(ptf.R_),
  L_(ptf.L_),
  structureParameters_(ptf.structureParameters_),
  conditioningFactor_(ptf.conditioningFactor_),
  overlap_(ptf.overlap_),
  curTimeIndex_(ptf.curTimeIndex_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::autoMap
(
    const fvPatchFieldMapper& m
)
{
    fixedValueFvPatchField<vector>::autoMap(m);
    Umean_.autoMap(m);
    R_.autoMap(m);
    L_.autoMap(m);
    structureParameters_.autoMap(m);
}


template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::rmap
(
    const fvPatchField<vector>& ptf,
    const labelList& addr
)
{
    fixedValueFvPatchField<vector>::rmap(ptf, addr);
    const inflowGeneratorFvPatchVectorField<TurbulentStructure>& tiptf = 
      refCast<const inflowGeneratorFvPatchVectorField<TurbulentStructure> >(ptf);
    Umean_.rmap(tiptf.Umean_, addr);
    R_.rmap(tiptf.R_, addr);
    L_.rmap(tiptf.L_, addr);
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
      scalar ovl = ( l1+l2 - mag(d) ) / Foam::min(l1, l2);
      Info<<"d="<<d<<" l1="<<l1<<" l2="<<l2<<" ovl="<<ovl<<endl;
      return ovl;
    }
  }
  return 0;
}

template<class TurbulentStructure>
vector inflowGeneratorFvPatchVectorField<TurbulentStructure>::randomTangentialDeflection(label fi)
{
  vector n=patch().Sf()[fi]; n/=mag(n);
  vector e1=n^vector(1,0,0);
  if (mag(e1)<SMALL) e1=n^vector(0,1,0);
  vector e2=n^e1;
  
  scalar dist=Foam::sqrt(patch().magSf()[fi]);

  return (0.5-ranGen_.scalar01())*dist*e1 + (0.5-ranGen_.scalar01())*dist*e2;
}

template<class TurbulentStructure>
autoPtr<indexedOctree<treeDataPoint> > inflowGeneratorFvPatchVectorField<TurbulentStructure>::buildTree(const pointField& vloc) const
{
  if (vloc.size()>3)
  {
    treeBoundBox overallBb(vloc);
    Random rndGen(123456);
    overallBb = overallBb.extend(rndGen, 1E-4);
    overallBb.min() -= point(ROOTVSMALL, ROOTVSMALL, ROOTVSMALL);
    overallBb.max() += point(ROOTVSMALL, ROOTVSMALL, ROOTVSMALL);

    return autoPtr<indexedOctree<treeDataPoint> > 
    (
      new indexedOctree<treeDataPoint>
      ( 
	treeDataPoint( vloc ),
	overallBb,                      // overall search domain
	8,                              // maxLevel
	10,                             // leafsize
	3.0                             // duplicity
      )
    );
  }
  return autoPtr<indexedOctree<treeDataPoint> > ();
}

template<class TurbulentStructure>
tmp<vectorField> inflowGeneratorFvPatchVectorField<TurbulentStructure>::continueFluctuationProcess()
{

//     // build the global list of face centres
//     Field<vector> myFaceCentres = patch().Cf();    
//     List<List<vector> > faceCentres(Pstream::nProcs());
//     faceCentres[Pstream::myProcNo()]=myFaceCentres;
//     Pstream::gatherList(faceCentres);
//     List<vector> globalFaceCentres = ListListOps::combine<List<vector> >(faceCentres, accessOp<List<vector> >());
    
    
    /**
     * ==================== Generation of new turbulent structures ========================
     */
    pointField vloc(vortons_.size());
    autoPtr<indexedOctree<treeDataPoint> > tree;
    
    if (vortons_.size()>0)
    {
      forAll(vortons_, vi) vloc[vi]=vortons_[vi];
      tree=buildTree(vloc);
    }
    label n_generated=0;
    forAll(patch().Cf(), fi)
    {
      // location of the new turbulent structure: randomly deflected around the current face centre
      const point& p = patch().Cf()[fi] + randomTangentialDeflection(fi);
      
      TurbulentStructure snew(ranGen_, p, Umean_[fi], L_[fi]);
      snew.randomize(ranGen_);
      
      scalar ovl = computeMinOverlap(tree, snew);
      //Info<<"gen: "<<patch().Cf()[fi] <<" "<< p <<" => ovl="<<ovl<<endl;
      Info<<ovl<<" "<<flush;
      if ( ovl < overlap_)
      {
	// append new structure to the end of the list
	vortons_.resize(vortons_.size()+1);
	vortons_[vortons_.size()-1]=snew;
	
	//Info << "rebuilding tree" <<endl;
	vloc.resize(vortons_.size());
	vloc[vloc.size()-1]=vortons_[vortons_.size()-1];
	tree=buildTree(vloc);
	
	n_generated++;
      }
    }
    Info<<endl;
    Info<<"Generated "<<n_generated<<" turbulent structures (now total "<<vortons_.size()<<")"<<endl;
    
    /**
     * ==================== Generation of turbulent fluctuations ========================
     */
    tmp<vectorField> tfluctuations(new vectorField(size(), pTraits<vector>::zero));
    vectorField& fluctuations = tfluctuations();
    
    forAll(*this, fi)
    {
      // superimpose turbulent velocity in affected faces
      forAll(vortons_, j)
	fluctuations[fi] += vortons_[j].fluctuation(structureParameters_, patch().Cf()[fi]);
    }
    
    forAll(vortons_, j)
    {
      // convect structure
      vortons_[j].moveForward(this->db().time().deltaT().value());
    }
    
//     List<List<TurbulentStructure> > scatterList(Pstream::nProcs());
// 
//     if ( Pstream::master() )
//     {
//         label n_x=label(ceil((bb.max().x()-bb.min().x())/(overlap_*Lspot_)));
//         label n_y=label(ceil((bb.max().y()-bb.min().y())/(overlap_*Lspot_)));
// 
//         scalar dx=ranGen_.scalar01() * overlap_ * Lspot_;
//         scalar dy=ranGen_.scalar01() * overlap_ * Lspot_;
//         
//         for (label i=0;i<n_x;i++)
//         {
//             for (label j=0;j<n_y;j++)
//             {
//                 vector x
//                     (
//                         bb.min().x() + scalar(i)*Lspot_ + dx,
//                         bb.min().y() + scalar(j)*Lspot_ + dy,
//                         -Lspot_
//                     );
// 
//                 bool keep=true;
//                 for (typename SLList<TurbulentStructure>::iterator vorton
//                          = vortons_.begin(); vorton!=vortons_.end(); ++vorton)
//                 {
//                     if (mag(vorton().location() - x) < overlap_*Lspot_)
//                     {
//                         keep=false;
//                         break;
//                     }
//                 }
// 
//                 if (keep)
//                 {
//                     TurbulentStructure newvorton(x);
//                     newvorton.randomize(ranGen_);
//                     vortons_.insert(newvorton);
//                 }
//             }
//         }
// 
//         scatterList[Pstream::myProcNo()].setSize(vortons_.size());
//         // transfer vortons to scatterList
//         label i=0;
//         for (typename SLList<TurbulentStructure>::iterator vorton
//                  = vortons_.begin(); vorton!=vortons_.end(); ++vorton)
//             scatterList[Pstream::myProcNo()][i++]=vorton();
//     }
// 
//     // Distribute vortons to all processors
//     Pstream::scatterList(scatterList);
//     
//     List<TurbulentStructure> allvortons= 
//         ListListOps::combine<List<TurbulentStructure> >
//         (
//             scatterList,
//             accessOp<List<TurbulentStructure> >()
//         );
// 
//     // =========== Calculation of induced turbulent fluctuations ==========
//     if (size()>0)
//     {
//         Field<vector> turbulent(referenceField_.size(), pTraits<vector>::zero);
// 
//         forAll(patchField, I)
//         {
//             vector pt=tloc[I];
//             
//             forAll(allvortons, vI)
//             {
//                 const TurbulentStructure& vorton=allvortons[vI];
// 
//                 if (mag(vorton.location() - pt) < Lspot_)
//                 {
//                     turbulent[I]+=
//                         cmptMultiply
//                         (
//                             fluctuationScale_,
//                             transform(rotback, vorton.fluctuation(param_, pt))
//                         );
//                 }
//             }
//         }
// 
//         tensorField a(turbulent.size(), pTraits<tensor>::zero);
// 	a.replace(tensor::XX, sqrt(RField_.component(symmTensor::XX)));
// 	a.replace(tensor::YX, RField_.component(symmTensor::XY)/a.component(tensor::XX));
// 	a.replace(tensor::ZX, RField_.component(symmTensor::XZ)/a.component(tensor::XX));
// 	a.replace(tensor::YY, sqrt(RField_.component(symmTensor::YY)-sqr(a.component(tensor::YX))));
// 	a.replace(tensor::ZY, ( RField_.component(symmTensor::YZ)
//              -a.component(tensor::YX)*a.component(tensor::ZX) )/a.component(tensor::YY));
//         a.replace(tensor::ZZ, sqrt(RField_.component(symmTensor::ZZ)
//              -sqr(a.component(tensor::ZX))-sqr(a.component(tensor::ZY))));
// 
//        fixedValueFvPatchField<vector>::operator==(referenceField_+ (a&turbulent) );
//     }
// 
//     scalar um=gAverage(referenceField_ & (-patch().nf()) );
// 
//     if (Pstream::master())
//     {
//         // =================== Vorton motion =======================
//         
//         if (um*this->db().time().deltaT().value() > 0.5*Lspot_)
//         {
//             WarningIn("inflowGeneratorFvPatchVectorField::updateCoeffs()")
//                 << "timestep to large: spots pass inlet within one timestep"<<endl;
//         }
//         
//         for (typename SLList<TurbulentStructure>::iterator vorton
//                  = vortons_.begin(); vorton!=vortons_.end(); ++vorton)
//         {
//             vorton().moveForward(um*this->db().time().deltaT().value()*vector(0,0,1));
//         }        
//         
//         // ================ remove Vortons that left inlet plane =====================
//         
//         bool modified;
//         do
//         {
//             modified=false;
//             for (typename SLList<TurbulentStructure>::iterator vorton
//                      = vortons_.begin(); vorton!=vortons_.end(); ++vorton)
//             {
//                 if (vorton().location().z() > Lspot_) 
//                 {
//                     vortons_.remove(vorton);
//                     modified=true;
//                     break;
//                 }
//             } 
//         } while (modified);
//         
//         Pout<<"Number of turbulent spots: "<<vortons_.size()<<endl;
//     }

  return tfluctuations;
}

template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::updateCoeffs()
{
  if (!conditioningFactor_.valid())
    computeConditioningFactor();
  
  if (this->updated())
  {
      return;
  }

  if (curTimeIndex_ != this->db().time().timeIndex())
  {
      fixedValueFvPatchField<vector>::operator==(Umean_ + continueFluctuationProcess());
      curTimeIndex_ = this->db().time().timeIndex();
  }

  fixedValueFvPatchField<vector>::updateCoeffs();
}

template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::write(Ostream& os) const
{
    Umean_.writeEntry("Umean", os);
    R_.writeEntry("R", os);
    L_.writeEntry("L", os);
    structureParameters_.write(os);
    os.writeKeyword("overlap") << overlap_ << token::END_STATEMENT << nl;
    structureParameters_.write(os);
    
    if (conditioningFactor_.valid())
    {
        conditioningFactor_().writeEntry("conditioningFactor", os);
    }
    
    os.writeKeyword("vortons") << vortons_ << token::END_STATEMENT <<nl;
    
    fixedValueFvPatchField<vector>::write(os);
}

} // End namespace Foam

// ************************************************************************* //
