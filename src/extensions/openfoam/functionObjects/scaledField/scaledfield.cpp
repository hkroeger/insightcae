#include "scaledfield.h"
#include "uniof.h"
#include "addToRunTimeSelectionTable.H"
#include "calculatedFvPatchFields.H"

namespace Foam
{

scaledField::scaledField(
        const word& name,
        const Time& runTime,
        const dictionary& dict
        )
  : functionObject(name),
    mesh_(
      runTime.lookupObject<fvMesh>
      (
          polyMesh::defaultRegion
      )),
    baseField_
    (
      IOobject(
       word(dict.lookup("baseField")),
       runTime.timeName(),
       mesh_,
       IOobject::MUST_READ,
       IOobject::AUTO_WRITE
       ),
      mesh_
     ),
    multiplier_(Function1<scalar>::New("multiplier", dict)),
//    resultField_
//    (
//      IOobject(
//       word(dict.lookup("resultFieldName")),
//       runTime.timeName(),
//       mesh_,
//       IOobject::MUST_READ,
//       IOobject::AUTO_WRITE
//       ),
//      mesh_
//     )
   resultFieldName_(dict.lookup("resultFieldName"))
{
}


bool scaledField::execute()
{
  if (mesh_.foundObject<volVectorField>(resultFieldName_))
  {
    volVectorField& resultField ( mesh_.lookupObjectRef<volVectorField>(resultFieldName_) );
    resultField = multiplier_->value(mesh_.time().value()) * baseField_;
  }
  if (mesh_.foundObject<surfaceScalarField>(resultFieldName_))
  {
    surfaceScalarField& resultFluxField ( mesh_.lookupObjectRef<surfaceScalarField>(resultFieldName_) );
    resultFluxField = fvc::flux(multiplier_->value(mesh_.time().value()) * baseField_);
  }
  return true;
}


bool scaledField::write()
{
  return true;
}


defineTypeNameAndDebug(scaledField, 0);

addToRunTimeSelectionTable
(
    functionObject,
    scaledField,
    dictionary
);

}
