 
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

SourceFiles
    inflowGeneratorFvPatchVectorField.C

\*---------------------------------------------------------------------------*/

#ifndef inflowGeneratorBaseFvPatchVectorField_H
#define inflowGeneratorBaseFvPatchVectorField_H

#include "boostRandomGen.H"
#include "fixedValueFvPatchFields.H"

#include "SLList.H"
#include "Switch.H"
#include "ListListOps.H"
#include "PstreamReduceOps.H"
#include "Tuple2.H"

#include "indexedOctree.H"
#include "treeDataPoint.H"
#include "treeDataFace.H"

#include "PatchTools.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{


class globalPatch
: public PrimitivePatch<face, List, pointField>
{
  
  labelList procOfs_;
  label nfaces_;
  
  autoPtr< PrimitivePatch<face, List, pointField> > createGlobalPatch(const polyPatch& patch);

public:
  globalPatch(const polyPatch& patch);
  
  inline label toGlobalFaceI(const label& localFaceI) const
  {
    return procOfs_[Pstream::myProcNo()] + localFaceI;
  }
  
  template<class T>
  tmp< Field<T> > extractLocalFaceValues(const Field<T>& gfield) const
  {
    label pi=Pstream::myProcNo();
    label startI=procOfs_[pi];
    label endI=gfield.size()-1;
    if (pi<Pstream::nProcs()-1)
      endI=procOfs_[pi+1]-1;
    
    tmp<Field<T> > tres(new Field<T>(endI-startI+1));
    Field<T>& res=tres();
    
    label j=0;
    for(label i=startI; i<=endI; i++)
    {
      res[j++]=gfield[i];
    }
    
    return tres;
  }
  
  template<class T>
  void insertLocalFaceValues(const Field<T>& lfield, Field<T>& gfield) const
  {
    label pi=Pstream::myProcNo();
    
    label startI=procOfs_[pi];
    label endI=nfaces_-1;
    if (pi<Pstream::nProcs()-1)
      endI=procOfs_[pi+1]-1;
    
    label j=0;
    for(label i=startI; i<=endI; i++)
    {
      gfield[i]=lfield[j++];
    }
  }
  
};



template<class TurbulentStructure, class PatchType>
class RecursiveApply
{
  
protected:
  const PatchType& patch_;
  const scalarField& c_;
  const typename TurbulentStructure::StructureParameters& sp_;
  vectorField& fluctuations_;
  
  /**
   * if applied value falls below this tolerance, recursive search will be stopped
   */
  static const scalar tol_ = 1e-3;
  
  void recursiveapply
  (
    const TurbulentStructure& v, 
    label faceI, 
    labelHashSet& visited,
    label depth
  )
  {
    depth++;
    if (faceI<0) return;
    
    vector u = v.fluctuation(sp_, patch_.faceCentres()[faceI]);
    
//     Pout<<"("<<depth<<")";
//     for(int i=0; i<depth; i++) Pout<<" ";
//       Pout<<"face="<<faceI<<" : u="<<u<<endl;

    if (mag(u)>tol_)
    {
      
      fluctuations_[faceI] += u / sqrt(c_[faceI]);
      visited.insert(faceI);
      
      labelList lneigh=patch_.faceFaces()[faceI];
      
      // visit local neighbours
      forAll(lneigh, j)
      {
	label nfi=lneigh[j];
	if (!visited.found(nfi))
	{
	  recursiveapply(v, nfi, visited, depth);
	}
      }
    }
  }

public:
  RecursiveApply
  (
    const PatchType& patch,
    const scalarField& c,
    const typename TurbulentStructure::StructureParameters& sp,
    vectorField& fluctuations
  )
  : patch_(patch),
    c_(c),
    sp_(sp),
    fluctuations_(fluctuations)
  {
  }
  
  
  label apply
  (
    const TurbulentStructure& v,
    label startfaceI
  )
  {
    labelHashSet visited;
    recursiveapply(v, startfaceI, visited, 0);
    return visited.size();
  }
  
};



class inflowGeneratorBaseFvPatchVectorField
:
    public fixedValueFvPatchField<vector>
{
  
public:
  struct ProcessStepInfo
  {
    /**
     * # structures currently in simulation
     */
    int n_total;

    /**
     * # structures generated in current step
     */
    int n_generated;

    /**
     * # structures deleted in current step
     */
    int n_removed;

//     /**
//      * # structures that induced fluctuations in patch
//      */
//     int n_induced;
  };

protected:
    BoostRandomGen ranGen_;
    
    /**
     * Prescribed mean velocity field on which the turbulent fluctuations will be superimposed.
     */
    Field<vector> Umean_;
    
    /**
     * If set to true, the average of the mean velocity field will be used for spot convection
     * otherwise, the local mean velue will be used
     */
    Switch uniformConvection_;
    
    /**
     * Prescribed reynolds stress tensor
     */
    Field<symmTensor> R_;
    
    /**
     * Prescribed length scale field
     */
    Field<symmTensor> L_;
    
    /**
     * Prescribed excess parameter
     */
    Field<scalar> c_;
    
    autoPtr<tensorField> Lund_;
    
    label curTimeIndex_;

    virtual void writeStateVisualization
    (  
      int i,
      const vectorField& u,
      const vectorField* uMean=NULL,
      const symmTensorField* uPrime2Mean=NULL
    ) const =0;
        
    vector randomTangentialDeflection(label fi);

    typedef autoPtr< indexedOctree<treeDataFace> > FaceSearcher;
    mutable FaceSearcher boundaryTree_;
    
public:
    virtual void computeConditioningFactor();

    //- Runtime type information
    TypeName("inflowGeneratorBase");


    // Constructors

        //- Construct from patch and internal field
        inflowGeneratorBaseFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        inflowGeneratorBaseFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given inflowGeneratorBaseFvPatchVectorField
        //  onto a new patch
        inflowGeneratorBaseFvPatchVectorField
        (
            const inflowGeneratorBaseFvPatchVectorField&,
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        inflowGeneratorBaseFvPatchVectorField
        (
            const inflowGeneratorBaseFvPatchVectorField&
        );

        //- Construct as copy setting internal field reference
        inflowGeneratorBaseFvPatchVectorField
        (
            const inflowGeneratorBaseFvPatchVectorField&,
            const DimensionedField<vector, volMesh>&
        );


	//- Map (and resize as needed) from self given a mapping object
	virtual void autoMap
	(
	    const fvPatchFieldMapper&
	);

	//- Reverse map the given fvPatchField onto this fvPatchField
	virtual void rmap
	(
	    const fvPatchField<vector>&,
	    const labelList&
	);
	
	// Member functions
	inline Field<vector>& Umean() { return Umean_; }
	inline Field<symmTensor>& R() { return R_; }
	inline Field<symmTensor>& L() { return L_; }
	inline const Field<vector>& Umean() const { return Umean_; }
	inline const Field<symmTensor>& R() const { return R_; }
	inline const Field<symmTensor>& L() const { return L_; }

	inline vector averageMeanVelocity() const
	{ return gSum(Umean_*patch().magSf())/gSum(patch().magSf()); }

        // Evaluation functions

        virtual tmp<vectorField> continueFluctuationProcess(scalar t, ProcessStepInfo *info=NULL) =0;
	
	virtual void updateCoeffs();
	
        //- Write
        virtual void write(Ostream&) const;
};

}

#endif