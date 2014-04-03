
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

inflowGeneratorBaseFvPatchVectorField::inflowGeneratorBaseFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchField<vector>(p, iF),
    ranGen_(1),
    Umean_(),
    R_(),
    L_(),
    c_(),
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
    ranGen_(1),
    Umean_(ptf.Umean_),
    R_(ptf.R_),
    L_(ptf.L_),
    c_(ptf.c_),
    conditioningFactor_(ptf.conditioningFactor_),
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
    ranGen_(1),
    Umean_("Umean", dict, size()),
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
  ranGen_(1),
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
  ranGen_(1),
  Umean_(ptf.Umean_),
  R_(ptf.R_),
  L_(ptf.L_),
  c_(ptf.c_),
  conditioningFactor_(ptf.conditioningFactor_),
  overlap_(ptf.overlap_),
  curTimeIndex_(ptf.curTimeIndex_)
{}


vector inflowGeneratorBaseFvPatchVectorField::randomTangentialDeflection(label fi)
{
  vector n=patch().Sf()[fi]; n/=mag(n);
  vector e1=n^vector(1,1,1);
  if (mag(e1)<SMALL) e1=n^vector(0,1,0);
  vector e2=n^e1;
  
  scalar dist=Foam::sqrt(patch().magSf()[fi]);

  return (0.5-ranGen_())*dist*e1 + (0.5-ranGen_())*dist*e2 ;
}

autoPtr<indexedOctree<treeDataPoint> > inflowGeneratorBaseFvPatchVectorField::buildTree(const pointField& vloc) const
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


void inflowGeneratorBaseFvPatchVectorField::updateCoeffs()
{
//   if (!conditioningFactor_.valid())
//     computeConditioningFactor();
  
  if (this->updated())
  {
      return;
  }

  if (curTimeIndex_ != this->db().time().timeIndex())
  {
      fixedValueFvPatchField<vector>::operator==(Umean_ + continueFluctuationProcess(this->db().time().value()));
      curTimeIndex_ = this->db().time().timeIndex();
  }

  fixedValueFvPatchField<vector>::updateCoeffs();
}


void inflowGeneratorBaseFvPatchVectorField::write(Ostream& os) const
{
    Umean_.writeEntry("Umean", os);
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