
#include "inflowGeneratorBaseFvPatchVectorField.H" 

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

namespace Foam 
{


defineTypeNameAndDebug(inflowGeneratorBaseFvPatchVectorField, 0);

label inflowGeneratorBaseFvPatchVectorField::getNearestFace(const point& p) const
{
  if (!boundaryTree_.valid())
  {
    Info << "rebuilding tree" <<endl;
    const polyPatch& pp = this->patch().patch();
    
    if (pp.size() > 0)
    {
        labelList bndFaces(pp.size());
	
        forAll(bndFaces, i)
        {
            bndFaces[i] =  pp.start() + i;
        }

        treeBoundBox overallBb(pp.points());
        Random rndGen(123456);
        overallBb = overallBb.extend(rndGen, 1e-4);
        overallBb.min() -= point(ROOTVSMALL, ROOTVSMALL, ROOTVSMALL);
        overallBb.max() += point(ROOTVSMALL, ROOTVSMALL, ROOTVSMALL);

        boundaryTree_.reset
        (
	  new indexedOctree<treeDataFace> 
	  (
	      treeDataFace    // all information needed to search faces
	      (
		  false,                      // do not cache bb
		  this->patch().boundaryMesh().mesh(),
		  bndFaces                    // patch faces only
	      ),
	      overallBb,                      // overall search domain
	      8,                              // maxLevel
	      10,                             // leafsize
	      3.0                             // duplicity
	  )
	);
    }
  }
  
  if (!boundaryTree_.valid())
    return -1;
  else
  {
    scalar span = boundaryTree_->bb().mag();

    pointIndexHit info = boundaryTree_->findNearest
    (
      p,
      Foam::sqr(span)
    );
    
    if (!info.hit())
    {
	info = boundaryTree_->findNearest
	(
	    p,
	    Foam::sqr(GREAT)
	);
    }
    
    label faceI = boundaryTree_->shapes().faceLabels()[info.index()];
    
    return faceI;
  }
    
}

inflowGeneratorBaseFvPatchVectorField::inflowGeneratorBaseFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchField<vector>(p, iF),
    ranGen_(Pstream::myProcNo()),
    Umean_(p.size(), vector::zero),
    R_(p.size(), symmTensor::zero),
    L_(p.size(), symmTensor::zero),
    c_(p.size(), 16),
    conditioningFactor_(),
    overlap_(0.5),
    curTimeIndex_(-1)
{
}

inflowGeneratorBaseFvPatchVectorField::inflowGeneratorBaseFvPatchVectorField
(
    const inflowGeneratorBaseFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchField<vector>(ptf, p, iF, mapper),
    ranGen_(Pstream::myProcNo()),
    Umean_(ptf.Umean_, mapper),
    R_(ptf.R_, mapper),
    L_(ptf.L_, mapper),
    c_(ptf.c_, mapper),
    conditioningFactor_(),
    overlap_(ptf.overlap_),
    curTimeIndex_(ptf.curTimeIndex_)
{
}

inflowGeneratorBaseFvPatchVectorField::inflowGeneratorBaseFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchField<vector>(p, iF, dict),
    ranGen_(Pstream::myProcNo()),
    Umean_("Umean", dict, size()),
    uniformConvection_(dict.lookupOrDefault<Switch>("uniformConvection", false)),
    R_("R", dict, size()),
    L_("L", dict, size()),
    c_("c", dict, size()),
    overlap_(dict.lookupOrDefault<scalar>("overlap", 0.5)),
    curTimeIndex_(-1)
{  
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

inflowGeneratorBaseFvPatchVectorField::inflowGeneratorBaseFvPatchVectorField
(
    const inflowGeneratorBaseFvPatchVectorField& ptf
)
: fixedValueFvPatchField<vector>(ptf),
  ranGen_(Pstream::myProcNo()),
  Umean_(ptf.Umean_),
  R_(ptf.R_),
  L_(ptf.L_),
  c_(ptf.c_),
  conditioningFactor_(ptf.conditioningFactor_),
  overlap_(ptf.overlap_),
  curTimeIndex_(ptf.curTimeIndex_)
{}

inflowGeneratorBaseFvPatchVectorField::inflowGeneratorBaseFvPatchVectorField
(
    const inflowGeneratorBaseFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
: fixedValueFvPatchField<vector>(ptf, iF),
  ranGen_(Pstream::myProcNo()),
  Umean_(ptf.Umean_),
  R_(ptf.R_),
  L_(ptf.L_),
  c_(ptf.c_),
  conditioningFactor_(ptf.conditioningFactor_),
  overlap_(ptf.overlap_),
  curTimeIndex_(ptf.curTimeIndex_)
{}


void inflowGeneratorBaseFvPatchVectorField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    fixedValueFvPatchField<vector>::autoMap(m);
    Umean_.autoMap(m);
    R_.autoMap(m);
    L_.autoMap(m);
    c_.autoMap(m);
}


void inflowGeneratorBaseFvPatchVectorField::rmap
(
    const fvPatchField<vector>& ptf,
    const labelList& addr
)
{
    fixedValueFvPatchField<vector>::rmap(ptf, addr);
    const inflowGeneratorBaseFvPatchVectorField& tiptf = 
      refCast<const inflowGeneratorBaseFvPatchVectorField >(ptf);
    Umean_.rmap(tiptf.Umean_, addr);
    R_.rmap(tiptf.R_, addr);
    L_.rmap(tiptf.L_, addr);
    c_.rmap(tiptf.c_, addr);
}

vector inflowGeneratorBaseFvPatchVectorField::randomTangentialDeflection(label fi)
{
  vector n=patch().Sf()[fi]; n/=mag(n);
  vector e1=n^vector(1,1,1);
  if (mag(e1)<SMALL) e1=n^vector(0,1,0);
  vector e2=n^e1;
  
  scalar dist=Foam::sqrt(patch().magSf()[fi]);

  return (0.5-ranGen_())*dist*e1 + (0.5-ranGen_())*dist*e2 ;
}

// autoPtr<indexedOctree<treeDataPoint> > inflowGeneratorBaseFvPatchVectorField::buildTree(const pointField& vloc) const
// {
//   if (vloc.size()>3)
//   {
//     treeBoundBox overallBb(vloc);
//     Random rndGen(123456);
//     overallBb = overallBb.extend(rndGen, 1E-4);
//     overallBb.min() -= point(ROOTVSMALL, ROOTVSMALL, ROOTVSMALL);
//     overallBb.max() += point(ROOTVSMALL, ROOTVSMALL, ROOTVSMALL);
// 
//     return autoPtr<indexedOctree<treeDataPoint> > 
//     (
//       new indexedOctree<treeDataPoint>
//       ( 
// 	treeDataPoint( vloc ),
// 	overallBb,                      // overall search domain
// 	8,                              // maxLevel
// 	10,                             // leafsize
// 	3.0                             // duplicity
//       )
//     );
//   }
//   return autoPtr<indexedOctree<treeDataPoint> > ();
// }


void inflowGeneratorBaseFvPatchVectorField::updateCoeffs()
{
//   if (debug>5)
//   {
//     Info<<"Calculating conditioning factor. This will take a long time. Reduce debug level, if inappropriate."<<endl;
//     computeConditioningFactor();
//   }
  
  if (!Lund_.valid())
  {
    tensorField LT(size(), tensor::zero);
    
    LT.replace(tensor::XX, sqrt(R_.component(symmTensor::XX)));
    LT.replace(tensor::YX, R_.component(symmTensor::XY)/(SMALL+LT.component(tensor::XX)));
    LT.replace(tensor::ZX, R_.component(symmTensor::XZ)/(SMALL+LT.component(tensor::XX)));
    LT.replace(tensor::YY, sqrt(R_.component(symmTensor::YY)-sqr(LT.component(tensor::YX))));
    LT.replace(tensor::ZY, (R_.component(symmTensor::YZ) - LT.component(tensor::YX)*LT.component(tensor::ZX) )/(SMALL+LT.component(tensor::YY)));
    LT.replace(tensor::ZZ, sqrt(R_.component(symmTensor::ZZ) - sqr(LT.component(tensor::ZX))-sqr(LT.component(tensor::ZY))));
    
    Lund_.reset(new tensorField(LT));
  }

  
  if (this->updated())
  {
      return;
  }

  if (curTimeIndex_ != this->db().time().timeIndex())
  {
    vectorField fluctuations=continueFluctuationProcess(this->db().time().value());
    
    fluctuations = Lund_() & fluctuations;
    
    if (debug>=3)
    { 
      Pout<<" Lund-transf. fluct.: min/max/avg = "<<min(fluctuations)<<" / "<<max(fluctuations) << " / "<<average(fluctuations)<<endl;
    }
    if (this->db().time().outputTime()) writeStateVisualization(0, fluctuations);
    
    vectorField turbField = Umean_ + fluctuations;
    
    scalar meanflux = gSum(Umean_ & patch().Sf());
    scalar turbflux = gSum(turbField & patch().Sf());
    scalar rescale = meanflux/turbflux;
//     scalar rescale=1.0;
//     turbField.component(vector::X) = pos(turbField&patch().Sf()) * turbField.component(vector::X);
//     scalar finalflux = gSum((rescale*turbField) & patch().Sf());
//     Info<<meanflux<<", "<<turbflux<<", "<<finalflux<<endl;
    Info<<" Inflow generator ["<<patch().name()<<"]: scaling turbulent fluctuations by "<< rescale << " to ensure constant flux across boundary."<<endl;

//     scalar meanflux = gSum(Umean_ & patch().Sf());
//     scalar turbflux = gSum(turbField & patch().Sf());
//     vector addvel = vector((turbflux-meanflux) / gSum(patch().magSf()), 0, 0);
//     scalar finalflux = gSum((turbField+addvel) & patch().Sf());
//     Info<<meanflux<<", "<<turbflux<<", "<<addvel<<", "<<finalflux<<endl;
//     Info<<"adding velocity "<< addvel << " to ensure constant flux across boundary."<<endl;
    
    
    
    fixedValueFvPatchField<vector>::operator==( turbField * rescale /*+addvel*/);
    curTimeIndex_ = this->db().time().timeIndex();
  }

  fixedValueFvPatchField<vector>::updateCoeffs();
}


void inflowGeneratorBaseFvPatchVectorField::write(Ostream& os) const
{
    Umean_.writeEntry("Umean", os);
    os.writeKeyword("uniformConvection") << uniformConvection_ << token::END_STATEMENT << nl;
    R_.writeEntry("R", os);
    L_.writeEntry("L", os);
    c_.writeEntry("c", os);
    os.writeKeyword("overlap") << overlap_ << token::END_STATEMENT << nl;
    
    if (conditioningFactor_.valid())
    {
        conditioningFactor_().writeEntry("conditioningFactor", os);
    }
    
    fixedValueFvPatchField<vector>::write(os);
}

}
