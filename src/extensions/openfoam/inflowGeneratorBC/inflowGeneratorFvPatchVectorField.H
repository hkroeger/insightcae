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

#ifndef inflowGeneratorFvPatchVectorField_H
#define inflowGeneratorFvPatchVectorField_H

#include "boost/shared_ptr.hpp"
#include "Random.H"
#include "inflowGeneratorBaseFvPatchVectorField.H"

#include "turbulentStructure.H"
#include "hatSpot.H"
#include "gaussianSpot.H"
#include "decayingTurbulenceSpot.H"
#include "SLList.H"

#include "indexedOctree.H"
#include "treeDataPoint.H"
#include "treeDataFace.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class inflowGeneratorFvPatchVectorField Declaration
\*---------------------------------------------------------------------------*/


template<class TurbulentStructure>
class inflowGeneratorFvPatchVectorField
: public inflowGeneratorBaseFvPatchVectorField
{
    
protected:
  
    autoPtr<globalPatch> globalPatch_;
  
    /**
     * not a linked list: each vorton needs to have an index for tree seach
     */
    Field<TurbulentStructure> vortons_; 
    
    boost::shared_ptr<scalarField> tau_;
    boost::shared_ptr<scalarField> crTimes_;
    
    /**
     * Additional parameters for the selected turbulent structure
     */
    typename TurbulentStructure::StructureParameters structureParameters_;
        
    void computeTau();
    
    virtual void writeStateVisualization
    (  
      int i,
      const vectorField& fluctuations,
      const vectorField* uMean=NULL,
      const symmTensorField* uPrime2Mean=NULL
    ) const;
    

    scalar computeMinOverlap(const autoPtr<indexedOctree<treeDataPoint> >& tree, const TurbulentStructure& snew) const;
    
    void induceInNeighbours
    (
      vectorField& fluctuations, 
      TurbulentStructure& v, 
      const typename TurbulentStructure::StructureParameters& sp,
      label faceI, 
      labelList& visited,
      label& depth
    ) const;

    tmp<Field<TurbulentStructure> > filterVortons
    (
	const inflowGeneratorFvPatchVectorField<TurbulentStructure>& ptf,
	const fvPatchFieldMapper& mapper,
	const Field<TurbulentStructure>& vlist
    ) const;    
    
public:

    //- Runtime type information
    TypeName("inflowGenerator");


    // Constructors

        //- Construct from patch and internal field
        inflowGeneratorFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        inflowGeneratorFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given inflowGeneratorFvPatchVectorField
        //  onto a new patch
        inflowGeneratorFvPatchVectorField
        (
            const inflowGeneratorFvPatchVectorField&,
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        inflowGeneratorFvPatchVectorField
        (
            const inflowGeneratorFvPatchVectorField&
        );

        //- Construct and return a clone
        virtual tmp<fvPatchField<vector> > clone() const
        {
            return tmp<fvPatchField<vector> >
            (
                new inflowGeneratorFvPatchVectorField(*this)
            );
        }

        //- Construct as copy setting internal field reference
        inflowGeneratorFvPatchVectorField
        (
            const inflowGeneratorFvPatchVectorField&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchField<vector> > clone
        (
            const DimensionedField<vector, volMesh>& iF
        ) const
        {
            return tmp<fvPatchField<vector> >
            (
                new inflowGeneratorFvPatchVectorField(*this, iF)
            );
        }


    // Member functions



    // Mapping functions

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
	
	// choose a random location x inside face fi, such that PDF of x is P(x)=1/A_f
	point randomFacePosition(label fi);


        // Evaluation functions

        virtual tmp<vectorField> continueFluctuationProcess(scalar t, ProcessStepInfo *info=NULL);
	
        //- Write
        virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


#include "inflowGeneratorFvPatchVectorField.C"

#endif

// ************************************************************************* //
