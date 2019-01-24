#include "makeFvOption.H"
#include "limitfield.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makeFvOption(limitField, scalar);
makeFvOption(limitField, vector);
makeFvOption(limitField, sphericalTensor);
makeFvOption(limitField, symmTensor);
makeFvOption(limitField, tensor);
