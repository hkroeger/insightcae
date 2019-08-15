
#include "flatPlateBLMappedFixedValueFvPatchField.H"
#include "addToRunTimeSelectionTable.H"

namespace Foam
{
  
defineTypeName(flatPlateBLMappedFixedValueFvPatchField);

addToRunTimeSelectionTable
(
    fvPatchVectorField,
    flatPlateBLMappedFixedValueFvPatchField,
    patch
);

addToRunTimeSelectionTable
(
    fvPatchVectorField,
    flatPlateBLMappedFixedValueFvPatchField,
    dictionary
);                                                                    
                                                                      
addToRunTimeSelectionTable                                            
(
    fvPatchVectorField,
    flatPlateBLMappedFixedValueFvPatchField,
    patchMapper
);                                                                       

}