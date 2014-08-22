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

#ifndef modalTurbulenceFvPatchVectorField_H
#define modalTurbulenceFvPatchVectorField_H

#include "boostRandomGen.H"
#include "inflowGeneratorBaseFvPatchVectorField.H"

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



class modalTurbulenceFvPatchVectorField
:
    public inflowGeneratorBaseFvPatchVectorField
{
public:
  struct mode
  {
    scalar k, phi, s;
    vector d, sigma;
    scalarField q;
    
    bool operator!=(const mode& other) const;
  };

protected:    
  List<mode> modes_;
  scalar tau_;
  
  /**
   * create and store the modes
   */
  void createModes();
  
  void writeStateVisualization
  (  
    int i,
    const vectorField& u,
    const vectorField* uMean=NULL,
    const symmTensorField* uPrime2Mean=NULL
  ) const;
    
public:
    //- Runtime type information
    TypeName("modalTurbulence");


    // Constructors

        //- Construct from patch and internal field
        modalTurbulenceFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        modalTurbulenceFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given modalTurbulenceFvPatchVectorField
        //  onto a new patch
        modalTurbulenceFvPatchVectorField
        (
            const modalTurbulenceFvPatchVectorField&,
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        modalTurbulenceFvPatchVectorField
        (
            const modalTurbulenceFvPatchVectorField&
        );

        //- Construct as copy setting internal field reference
        modalTurbulenceFvPatchVectorField
        (
            const modalTurbulenceFvPatchVectorField&,
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
	
        //- Construct and return a clone
        virtual tmp<fvPatchVectorField> clone() const
        {
            return tmp<fvPatchVectorField>
            (
                new modalTurbulenceFvPatchVectorField
                (
                    *this
                )
            );
        }
        
        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchVectorField> clone
        (
            const DimensionedField<vector, volMesh>& iF
        ) const
        {
            return tmp<fvPatchVectorField>
            (
                new modalTurbulenceFvPatchVectorField
                (
                    *this,
                    iF
                )
            );
        }


        // Evaluation functions

        virtual tmp<vectorField> continueFluctuationProcess(scalar t, ProcessStepInfo *info=NULL);

	//- Write
        virtual void write(Ostream&) const;
};

Ostream& operator<<(Ostream& os, const modalTurbulenceFvPatchVectorField::mode&);
Istream& operator>>(Istream& os, modalTurbulenceFvPatchVectorField::mode&);

}

#endif