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

#include "inflowGeneratorBaseFvPatchVectorField.H" 

#include "transform.H"
#include "transformField.H"
#include "volFields.H"
#include "ListListOps.H"
#include "PstreamReduceOps.H"
#include "addToRunTimeSelectionTable.H"
#include "globalMeshData.H"
#include "globalIndex.H"

#include "base/vtktools.h"

#include <vector>
#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;

namespace Foam 
{
  
autoPtr<PrimitivePatch<face, List, pointField> > globalPatch::createGlobalPatch(const polyPatch& patch)
{
  const polyMesh& mesh=patch.boundaryMesh().mesh();
  
  IOobject addrHeader
  (
      "pointProcAddressing",
      mesh.facesInstance()/mesh.meshSubDir,
      mesh,
      IOobject::MUST_READ
  );
  
  if (addrHeader.headerOk())
  {
    // There is a pointProcAddressing file so use it to get labels
    // on the original mesh
    labelIOList gpi(addrHeader);
  
    typedef Tuple2<point,label> piTuple;
    typedef HashTable<piTuple,label> pointHashTable;
    
    pointHashTable usedGlobalPts;
    forAll(patch.localPoints(), lpI)
    {
      label gpt=gpi[patch.meshPoints()[lpI]];
      usedGlobalPts.insert( gpt, piTuple(patch.localPoints()[lpI], -1) );
    }
    
    Pstream::mapCombineGather(usedGlobalPts, eqOp<piTuple>());
        
    if (Pstream::master())
    {
      label idx=0;
      forAllIter(pointHashTable, usedGlobalPts, pi)
      {
	pi().second()=idx++;
      }
    }

    Pstream::mapCombineScatter(usedGlobalPts);

    pointField globalPoints(usedGlobalPts.size());
    forAllIter(pointHashTable, usedGlobalPts, pi)
    {
      globalPoints[pi().second()] = pi().first();
    }
    
    List<faceList> allfaces(Pstream::nProcs());
    faceList& myfaces=allfaces[Pstream::myProcNo()];
    myfaces.resize(patch.size());
    
    forAll(patch, fI)
    {
      const face& f = patch[fI];
      face gf(f.size());
      forAll(gf, vI)
      {
	gf[vI] = usedGlobalPts[ gpi[ f[vI] ] ].second();
      }
      myfaces[fI]=gf;
    }
    
    Pstream::gatherList(allfaces);
    Pstream::scatterList(allfaces);
    
    faceList globalFaces =
	ListListOps::combine<faceList>
	(
	  allfaces, accessOp<faceList>()
	);
    
    return autoPtr<PrimitivePatch<face, List, pointField> >
    (
      new PrimitivePatch<face, List, pointField>(globalFaces, globalPoints)
    );
  }
  else
  {
    return autoPtr<PrimitivePatch<face, List, pointField> >
    (
      new PrimitivePatch<face, List, pointField>(patch.localFaces(), patch.localPoints())
    );
  }
  
}

  
globalPatch::globalPatch(const polyPatch& patch)
: PrimitivePatch<face, List, pointField>(createGlobalPatch(patch)()),
  procOfs_(Pstream::nProcs(), -1)
{
  labelList nfaces(Pstream::nProcs());
  nfaces[Pstream::myProcNo()]=patch.size();

  Pstream::gatherList(nfaces);
  Pstream::scatterList(nfaces);
  
  procOfs_[0]=0;
  nfaces_=nfaces[0];
  for(label j=1; j<Pstream::nProcs(); j++)
  {
    nfaces_+=nfaces[j];
    procOfs_[j]=procOfs_[j-1] + nfaces[j-1];
  }
}


defineTypeNameAndDebug(inflowGeneratorBaseFvPatchVectorField, 0);


void inflowGeneratorBaseFvPatchVectorField::computeConditioningFactor(int writeInterval)
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
    vectorField u=continueFluctuationProcess(t, &info);
    N_total+=info.n_generated;
    V += gSum( (-patch().Sf()&Umean()) * dt );
    
    scalar alpha = scalar(i - 1)/scalar(i);
    scalar beta = 1.0/scalar(i);
    
//     uPrime2Mean += sqr(uMean);
    uMean = alpha*uMean + beta*u;
//     N = alpha*N + beta*scalar(info.n_induced);
    uPrime2Mean = alpha*uPrime2Mean + beta*sqr(u) /*- sqr(uMean)*/; //uMean shoudl be zero
    
    Info<<"i="<<i<<": Averages: uMean="
	<<gSum(uMean*patch().magSf())/gSum(patch().magSf())
	<<" \t R^2="
	<<gSum(uPrime2Mean*patch().magSf())/gSum(patch().magSf())
	/*<< "\t N="<<N*/<<"\t N_tot="<<N_total<<"\t V="<<V<< endl;
		    
    if (i%writeInterval==0) writeStateVisualization(i, u, &uMean, &uPrime2Mean);
  }
  
  FatalErrorIn("computeConditioningFactor") << "STOP" << abort(FatalError);
}

inflowGeneratorBaseFvPatchVectorField::inflowGeneratorBaseFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchField<vector>(p, iF),
    ranGen_(Pstream::myProcNo()),
//     Umean_(p.size(), vector::zero),
//     R_(p.size(), symmTensor::zero),
//     L_(p.size(), symmTensor::zero),
//     c_(p.size(), 16),
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
//     Umean_(ptf.Umean_, mapper),
//     R_(ptf.R_, mapper),
//     L_(ptf.L_, mapper),
//     c_(ptf.c_, mapper),
    Umean_(ptf.Umean_().clone()),
    R_(ptf.R_().clone()),
    L_(ptf.L_().clone()),
    c_(ptf.c_().clone()),
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
//     Umean_("Umean", dict, size()),
    uniformConvection_(dict.lookupOrDefault<Switch>("uniformConvection", false)),
//     R_("R", dict, size()),
//     L_("L", dict, size()),
//     c_("c", dict, size()),
    Umean_(FieldDataProvider<vector>::New(dict.lookup("Umean"))),
    R_(FieldDataProvider<symmTensor>::New(dict.lookup("R"))),
    L_(FieldDataProvider<symmTensor>::New(dict.lookup("L"))),
    c_(FieldDataProvider<scalar>::New(dict.lookup("c"))),
    curTimeIndex_(-1)
{  
}

inflowGeneratorBaseFvPatchVectorField::inflowGeneratorBaseFvPatchVectorField
(
    const inflowGeneratorBaseFvPatchVectorField& ptf
)
: fixedValueFvPatchField<vector>(ptf),
  ranGen_(Pstream::myProcNo()),
//   Umean_(ptf.Umean_),
//   R_(ptf.R_),
//   L_(ptf.L_),
//   c_(ptf.c_),
    Umean_(ptf.Umean_().clone()),
    R_(ptf.R_().clone()),
    L_(ptf.L_().clone()),
    c_(ptf.c_().clone()),
  curTimeIndex_(ptf.curTimeIndex_)
{}

inflowGeneratorBaseFvPatchVectorField::inflowGeneratorBaseFvPatchVectorField
(
    const inflowGeneratorBaseFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
: fixedValueFvPatchField<vector>(ptf, iF),
  ranGen_(Pstream::myProcNo()),
//   Umean_(ptf.Umean_),
//   R_(ptf.R_),
//   L_(ptf.L_),
//   c_(ptf.c_),
    Umean_(ptf.Umean_().clone()),
    R_(ptf.R_().clone()),
    L_(ptf.L_().clone()),
    c_(ptf.c_().clone()),
  curTimeIndex_(ptf.curTimeIndex_)
{}


void inflowGeneratorBaseFvPatchVectorField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    fixedValueFvPatchField<vector>::autoMap(m);
//     Umean_.autoMap(m);
//     R_.autoMap(m);
//     L_.autoMap(m);
//     c_.autoMap(m);
}


void inflowGeneratorBaseFvPatchVectorField::rmap
(
    const fvPatchField<vector>& ptf,
    const labelList& addr
)
{
    fixedValueFvPatchField<vector>::rmap(ptf, addr);
//     const inflowGeneratorBaseFvPatchVectorField& tiptf = 
//       refCast<const inflowGeneratorBaseFvPatchVectorField >(ptf);
//     Umean_.rmap(tiptf.Umean_, addr);
//     R_.rmap(tiptf.R_, addr);
//     L_.rmap(tiptf.L_, addr);
//     c_.rmap(tiptf.c_, addr);
}

void inflowGeneratorBaseFvPatchVectorField::setParameters(const vectorField& umean, const symmTensorField& R, const symmTensorField& L)
{
  FatalErrorIn("inflowGeneratorBaseFvPatchVectorField::setParameters()")
  <<"yet unsupported"<<abort(FatalError);
  
//   Umean_=umean;
//   R_=R;
//   L_=L;
}


tmp<scalarField> inflowGeneratorBaseFvPatchVectorField::edgeLengths(bool maxL) const
{
  tmp<scalarField> res(new scalarField(size(), maxL?0.0:GREAT));
  scalarField& delta_edge = res();
  
  const polyPatch& ppatch = patch().patch();

  forAll(ppatch.edges(), ei)
  {
    const edge& e = ppatch.edges()[ei];
    scalar this_edge_len=e.mag(ppatch.localPoints());
    forAll(ppatch.edgeFaces()[ei], j)
    {
      label fi = ppatch.edgeFaces()[ei][j];
      if (maxL)
	delta_edge[fi] = max(delta_edge[fi], this_edge_len);
      else
	delta_edge[fi] = min(delta_edge[fi], this_edge_len);
    }
  }
  
  return res;
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


void inflowGeneratorBaseFvPatchVectorField::updateCoeffs()
{
  
  if (this->updated())
  {
      return;
  }

  if (curTimeIndex_ != this->db().time().timeIndex())
  {
    vectorField fluctuations=continueFluctuationProcess(this->db().time().value());
    
    if (this->db().time().outputTime()) writeStateVisualization(0, fluctuations);
    
    vectorField Um(Umean());
    
    vectorField turbField = Um + fluctuations;
    
    scalar meanflux = gSum(Um & patch().Sf());
    scalar turbflux = gSum(turbField & patch().Sf());
    scalar rescale = meanflux/turbflux;
    Info<<" Inflow generator ["<<patch().name()<<"]: scaling turbulent fluctuations by "<< rescale << " to ensure constant flux across boundary."<<endl;
    
    fixedValueFvPatchField<vector>::operator==( turbField * rescale );
    curTimeIndex_ = this->db().time().timeIndex();
  }

  fixedValueFvPatchField<vector>::updateCoeffs();
}


void inflowGeneratorBaseFvPatchVectorField::write(Ostream& os) const
{
    os.writeKeyword("uniformConvection") << uniformConvection_ << token::END_STATEMENT << nl;
    Umean_().writeEntry("Umean", os);
    R_().writeEntry("R", os);
    L_().writeEntry("L", os);
    c_().writeEntry("c", os);
        
    fixedValueFvPatchField<vector>::write(os);
}

}
