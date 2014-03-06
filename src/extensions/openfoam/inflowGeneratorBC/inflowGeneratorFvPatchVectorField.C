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

#include <vector>
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
  
  for (int i=1; i<1000; i++)
  {
    
    vectorField u( continueFluctuationProcess() );
    
    scalar alpha = scalar(i - 1)/scalar(i);
    scalar beta = 1.0/scalar(i);
    
    uMean = alpha*uMean + beta*u;
    uPrime2Mean = alpha*uPrime2Mean + beta*sqr(u); // - sqr(uMean); //uMean shoudl be zero
    
    Info<<"Averages: uMean="<<average(uMean)<<" \t R^2="<< average(uPrime2Mean) << endl;
		
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
    vtk_patch.appendCellVectorField
    (
      "uMean", 
      uMean.component(vector::X)().cdata(),
      uMean.component(vector::Y)().cdata(),
      uMean.component(vector::Z)().cdata()
    );
    vtk_patch.appendCellTensorField
    (
      "R", 
      uPrime2Mean.component(tensor::XX)().cdata(),
      uPrime2Mean.component(tensor::XY)().cdata(),
      uPrime2Mean.component(tensor::XZ)().cdata(),
      uPrime2Mean.component(tensor::YX)().cdata(),
      uPrime2Mean.component(tensor::YY)().cdata(),
      uPrime2Mean.component(tensor::YZ)().cdata(),
      uPrime2Mean.component(tensor::ZX)().cdata(),
      uPrime2Mean.component(tensor::ZY)().cdata(),
      uPrime2Mean.component(tensor::ZZ)().cdata()
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
  
  FatalErrorIn("computeConditioningFactor") << "STOP" << abort(FatalError);
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
}

template<class TurbulentStructure>
inflowGeneratorFvPatchVectorField<TurbulentStructure>::inflowGeneratorFvPatchVectorField
(
    const inflowGeneratorFvPatchVectorField& ptf
)
: inflowGeneratorBaseFvPatchVectorField(ptf),
  vortons_(ptf.vortons_),
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
  structureParameters_(ptf.structureParameters_)
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
      scalar ovl = ( 0.5*(l1+l2) - mag(d) ) / (Foam::min(l1, l2));
      //Info<<"d="<<d<<", l1="<<l1<<", l2="<<l2<< ", ovl="<<ovl<<endl;
      return ovl;
    }
  }
  return 0;
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
      //Info<<ovl<<" "<<flush;
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
//     //Info<<endl;
    
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

    Info<<"Generated "<<n_generated<<" turbulent structures, removed "<<n_removed<<" (now total "<<vortons_.size()<<")"<<endl;
    
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
void inflowGeneratorFvPatchVectorField<TurbulentStructure>::write(Ostream& os) const
{
    structureParameters_.write(os);
    os.writeKeyword("vortons") << vortons_ << token::END_STATEMENT <<nl;    
    inflowGeneratorBaseFvPatchVectorField::write(os);
}

} // End namespace Foam

// ************************************************************************* //
