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

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class TurbulentStructure>
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::computeConditioningFactor()
{

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
    Umean_(dict.lookup("Umean")),
    R_(dict.lookup("R")),
    L_(dict.lookup("L")),
    structureParameters_(dict),
    overlap_(dict.lookupOrDefault<scalar>("overlap", 0.5)),
    curTimeIndex_(-1)
{
    if (dict.found("vortons"))
    {
        vortons_=SLList<TurbulentStructure>(dict.lookup("vortons"));
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
scalar inflowGeneratorFvPatchVectorField<TurbulentStructure>::computeMinOverlap(const indexedOctree<treeDataPoint>& tree, const TurbulentStructure& snew) const
{
  
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
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::doUpdate()
{
    Field<vector>& patchField = *this;

//     // build the global list of face centres
//     Field<vector> myFaceCentres = patch().Cf();    
//     List<List<vector> > faceCentres(Pstream::nProcs());
//     faceCentres[Pstream::myProcNo()]=myFaceCentres;
//     Pstream::gatherList(faceCentres);
//     List<vector> globalFaceCentres = ListListOps::combine<List<vector> >(faceCentres, accessOp<List<vector> >());
    
    
    /**
     * ==================== Generation of new turbulent structures ========================
     */
    pointField locs;//(vortons_);
    autoPtr<indexedOctree<treeDataPoint> > tree (new indexedOctree<treeDataPoint>(treeDataPoint(locs)) );
    forAll(patch().Cf(), fi)
    {
      // location of the new turbulent structure: randomly deflected around the current face centre
      const point& p = patch().Cf()[fi] + randomTangentialDeflection(fi);
      
      TurbulentStructure snew(p, Umean_[fi], L_[fi]);
      snew.randomize(ranGen_);
      
      if (computeMinOverlap(tree, snew) < overlap_)
      {
	vortons_.insert(snew);
	//locs=vortons_;
	tree.reset( new indexedOctree<treeDataPoint>(locs) );
      }
    }
    
    /**
     * ==================== Generation of turbulent fluctuations ========================
     */
//     vectorField fluctuations(size(), pTraits<vector>::zero);
//     for (typename SLList<TurbulentStructure>::iterator s
// 	      = vortons_.begin(); s!=vortons_.end(); ++s)
//     {      
//       // do tree search for finding faces affected by the current structure
//       labelList ifl = findAffectedFaces( s() );
//       
//       // superimpose turbulent velocity in affected faces
//       forAll(ifl, j)
// 	fluctuations[ifl[j]] += s().fluctuation(patch().Cf()[ifl[j]]);
//       
//       // convect structure
//       s().moveForward(mesh().time().deltaT().value());
//     }
    
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
//         fixedValueFvPatchField<vector>::operator==(referenceField_+ (a&turbulent) );
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
      doUpdate();
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
