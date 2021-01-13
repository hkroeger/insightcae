
#include "rangeweightfield.h"
#include "uniof.h"
#include "addToRunTimeSelectionTable.H"
#include "calculatedFvPatchFields.H"

namespace Foam
{

rangeWeightField::rangeWeightField(
        const word& name,
        const Time& runTime,
        const dictionary& dict
        )
  : functionObject(name),
    mesh_(runTime.lookupObject<fvMesh>
          (
              polyMesh::defaultRegion
          )),
    sourceFieldName_(dict.lookup("sourceFieldName")),
    min_(readScalar(dict.lookup("min"))),
    max_(readScalar(dict.lookup("max"))),
    outputFieldName_(dict.lookup("outputFieldName")),
    multiplyFieldName_(dict.lookupOrDefault<word>("multiplyFieldName", ""))
{
  Info << "Creating weight field " << outputFieldName_ << endl;

  outputScalarField_.reset
  (
    new volScalarField
    (
       IOobject
       (
          outputFieldName_,
          runTime.timeName(),
          mesh_,
          IOobject::NO_READ,
          IOobject::NO_WRITE
       ),
       mesh_,
       dimensionedScalar("", dimless, 0.0),
       zeroGradientFvPatchField<scalar>::typeName
    )
  );

}


bool rangeWeightField::execute()
{
    if (mesh_.foundObject<volScalarField>(sourceFieldName_))
    {
        const volScalarField& sf = mesh_.lookupObject<volScalarField>(sourceFieldName_);

        outputScalarField_->internalField() =
                pos(sf.internalField()-dimensionedScalar("min", sf.dimensions(), min_))
                *
                neg(sf.internalField()-dimensionedScalar("max", sf.dimensions(), max_))
                ;

        if (!multiplyFieldName_.empty())
        {
          if (mesh_.foundObject<volScalarField>(multiplyFieldName_))
          {
            const volScalarField& mf = mesh_.lookupObject<volScalarField>(multiplyFieldName_);
            outputScalarField_->internalField() *= mf.internalField() / dimensionedScalar("", mf.dimensions(), 1.0);
          }
          else
          {
            FatalErrorIn("rangeWeightField::execute()")
                << "Could not look up scalar field "<<multiplyFieldName_<<"!"
                <<abort(FatalError);
          }
        }
    }
    else
    {
        Info<<"Source field "<<sourceFieldName_<<" not found. Setting weight field to zero."<<endl;
        outputScalarField_->internalField() = 0.0;
    }

    outputScalarField_->correctBoundaryConditions();

  return true;
}


bool rangeWeightField::write()
{
  return true;
}


defineTypeNameAndDebug(rangeWeightField, 0);

addToRunTimeSelectionTable
(
    functionObject,
    rangeWeightField,
    dictionary
);

}
