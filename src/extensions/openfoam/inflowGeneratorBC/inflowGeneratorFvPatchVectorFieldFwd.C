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

\*---------------------------------------------------------------------------*/

#include "inflowGeneratorFvPatchVectorField.H"
#include "fixedValueFvPatchFields.H"
#include "transform.H"
#include "transformField.H"
#include "volFields.H"
#include "typeInfo.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#define makeInflowGeneratorFvPatchField(spotType)                   	\
                                                                        \
typedef inflowGeneratorFvPatchVectorField<spotType>                     \
inflowGeneratorFvPatchVectorField##spotType;                            \
                                                                        \
defineTemplateTypeNameAndDebugWithName(                                 \
    inflowGeneratorFvPatchVectorField##spotType,                        \
    "inflowGenerator<"#spotType">", 0)                                  \
                                                                        \
addToRunTimeSelectionTable                                              \
(                                                                       \
    fvPatchVectorField,                                                 \
    inflowGeneratorFvPatchVectorField##spotType,                        \
    patch                                                               \
);                                                                      \
                                                                        \
addToRunTimeSelectionTable                                              \
(                                                                       \
    fvPatchVectorField,                                                 \
    inflowGeneratorFvPatchVectorField##spotType,                        \
    dictionary                                                          \
);                                                                      \
                                                                        \
addToRunTimeSelectionTable                                              \
(                                                                       \
    fvPatchVectorField,                                                 \
    inflowGeneratorFvPatchVectorField##spotType,                        \
    patchMapper                                                         \
)                                                                       \
    

typedef inflowGeneratorFvPatchVectorField<hatSpot> inflowGeneratorFvPatchVectorFieldhatSpot;
defineTemplateTypeNameAndDebugWithName(inflowGeneratorFvPatchVectorFieldhatSpot, "inflowGenerator<hatSpot>", 0); 
addToRunTimeSelectionTable                                          
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldhatSpot,                        
    patch                                                               
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldhatSpot,                        
    dictionary                                                          
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldhatSpot,                        
    patchMapper                                                         
);

//makeInflowGeneratorFvPatchField(hatSpot);

typedef inflowGeneratorFvPatchVectorField<homogeneousTurbulenceVorton> inflowGeneratorFvPatchVectorFieldhomogeneousTurbulence;
defineTemplateTypeNameAndDebugWithName(inflowGeneratorFvPatchVectorFieldhomogeneousTurbulence, "inflowGenerator<homogeneousTurbulence>", 0); 
addToRunTimeSelectionTable                                          
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldhomogeneousTurbulence,                        
    patch                                                               
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldhomogeneousTurbulence,                        
    dictionary                                                          
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldhomogeneousTurbulence,                        
    patchMapper                                                         
);

// typedef inflowGeneratorFvPatchVectorField<homogeneousTurbulenceVorton> inflowGeneratorFvPatchVectorFieldhomogeneousTurbulenceVorton;
// defineTemplateTypeNameAndDebugWithName(inflowGeneratorFvPatchVectorFieldhomogeneousTurbulenceVorton, "inflowGenerator<homogeneousTurbulenceVorton>", 0); 
// addToRunTimeSelectionTable                                          
// (                                                                       
//     fvPatchVectorField,                                                 
//     inflowGeneratorFvPatchVectorFieldhomogeneousTurbulenceVorton,                        
//     patch                                                               
// );                                                                      
//                                                                         
// addToRunTimeSelectionTable                                              
// (                                                                       
//     fvPatchVectorField,                                                 
//     inflowGeneratorFvPatchVectorFieldhomogeneousTurbulenceVorton,                        
//     dictionary                                                          
// );                                                                      
//                                                                         
// addToRunTimeSelectionTable                                              
// (                                                                       
//     fvPatchVectorField,                                                 
//     inflowGeneratorFvPatchVectorFieldhomogeneousTurbulenceVorton,                        
//     patchMapper                                                         
// );

//makeInflowGeneratorFvPatchField(homogeneousTurbulenceVorton);

} // End namespace Foam

// ************************************************************************* //
