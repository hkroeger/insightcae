
#include "providefields.h"
#include "uniof.h"
#include "addToRunTimeSelectionTable.H"
#include "calculatedFvPatchFields.H"

namespace Foam
{

provideFields::provideFields(
        const word& name,
        const Time& runTime,
        const dictionary& dict
        )
  : functionObject(name)
{
  wordList readFieldNames(dict.lookup("readFields"));

  const fvMesh& mesh =
      runTime.lookupObject<fvMesh>
      (
          polyMesh::defaultRegion
      );

  forAll(readFieldNames, i)
  {
    IOobject ioo(
          readFieldNames[i],
          runTime.timeName(),
          mesh,
          IOobject::MUST_READ,
          IOobject::AUTO_WRITE
          );

    if (UNIOF_HEADEROK(ioo, volScalarField))
    {
      scalarFields_.append(new volScalarField(ioo, mesh));
    }
    else if (UNIOF_HEADEROK(ioo, volVectorField))
    {
      vectorFields_.append(new volVectorField(ioo, mesh));
    }
    else if (UNIOF_HEADEROK(ioo, volTensorField))
    {
      tensorFields_.append(new volTensorField(ioo, mesh));
    }
    else if (UNIOF_HEADEROK(ioo, volSymmTensorField))
    {
      symmTensorFields_.append(new volSymmTensorField(ioo, mesh));
    }
    else
    {
      FatalErrorIn("provideFields::provideFields()")
          << "Did not find field "<<readFieldNames[i]
          << " (must be scalar, vector, tensor or symmTensor field)"
          << abort(FatalError);
    }
  }

  if (dict.found("createScalarFields"))
  {
    const dictionary& csf = dict.subDict("createScalarFields");
    wordList fields = csf.toc();
    forAll(fields, i)
    {
      word fieldName = fields[i];
      const dictionary& propDict = csf.subDict(fieldName);
      scalar value = readScalar(propDict.lookup("value"));
      dimensionSet dims(propDict.lookup("dimensions"));

      Info << "Creating and storing field "<< fieldName
           << " with value " << value << endl;

      scalarFields_.append
          (
            new volScalarField
            (
              IOobject
              (
                fieldName,
                runTime.timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::AUTO_WRITE
              ),
              mesh,
              dimensioned<scalar>("", dims, value ),
              calculatedFvPatchField<scalar>::typeName
            )
           );
    }
  }

}


bool provideFields::execute()
{
  return true;
}


bool provideFields::write()
{
  return true;
}


defineTypeNameAndDebug(provideFields, 0);

addToRunTimeSelectionTable
(
    functionObject,
    provideFields,
    dictionary
);

}
