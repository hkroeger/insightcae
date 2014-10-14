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

typedef inflowGeneratorFvPatchVectorField<gaussianSpot> inflowGeneratorFvPatchVectorFieldgaussianSpot;
defineTemplateTypeNameAndDebugWithName(inflowGeneratorFvPatchVectorFieldgaussianSpot, "inflowGenerator<gaussianSpot>", 0); 
addToRunTimeSelectionTable                                          
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldgaussianSpot,                        
    patch                                                               
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldgaussianSpot,                        
    dictionary                                                          
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldgaussianSpot,                        
    patchMapper                                                         
);

typedef inflowGeneratorFvPatchVectorField<decayingTurbulenceSpot> inflowGeneratorFvPatchVectorFielddecayingTurbulenceSpot;
defineTemplateTypeNameAndDebugWithName(inflowGeneratorFvPatchVectorFielddecayingTurbulenceSpot, "inflowGenerator<decayingTurbulenceSpot>", 0); 
addToRunTimeSelectionTable                                          
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFielddecayingTurbulenceSpot,                        
    patch                                                               
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFielddecayingTurbulenceSpot,                        
    dictionary                                                          
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFielddecayingTurbulenceSpot,                        
    patchMapper                                                         
);

typedef inflowGeneratorFvPatchVectorField<anisotropicVorton> inflowGeneratorFvPatchVectorFieldanisotropicVorton;
defineTemplateTypeNameAndDebugWithName(inflowGeneratorFvPatchVectorFieldanisotropicVorton, "inflowGenerator<anisotropicVorton>", 0); 
addToRunTimeSelectionTable                                          
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldanisotropicVorton,                        
    patch                                                               
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldanisotropicVorton,                        
    dictionary                                                          
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFieldanisotropicVorton,                        
    patchMapper                                                         
);


typedef inflowGeneratorFvPatchVectorField<decayingTurbulenceVorton> inflowGeneratorFvPatchVectorFielddecayingTurbulenceVorton;
defineTemplateTypeNameAndDebugWithName(inflowGeneratorFvPatchVectorFielddecayingTurbulenceVorton, "inflowGenerator<decayingTurbulenceVorton>", 0); 
addToRunTimeSelectionTable                                          
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFielddecayingTurbulenceVorton,                        
    patch                                                               
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFielddecayingTurbulenceVorton,                        
    dictionary                                                          
);                                                                      
                                                                        
addToRunTimeSelectionTable                                              
(                                                                       
    fvPatchVectorField,                                                 
    inflowGeneratorFvPatchVectorFielddecayingTurbulenceVorton,                        
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
