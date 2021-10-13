/*---------------------------------------------------------------------------*\


\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "IFstream.H"
#include "pointToPointPlanarInterpolation.H"

#include "uniof.h"

using namespace Foam;


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
void expand(
    const fvMesh& mesh,
    const IOobject& fieldHeader,
    const string& patchName,
    const scalar& timeValue,
    const string& mapMethod = "planarInterpolation",
    const scalar perturb = 1e-5
    )
{
  const Time& runTime=mesh.time();
  auto fieldName=fieldHeader.name();
  label pI = mesh.boundaryMesh().findPatchID(patchName);
  if (pI<0)
  {
    FatalErrorIn("main")
        << "Patch "<<patchName<<" is not existing!"
        <<abort(FatalError);
  }

  GeometricField<Type, fvPatchField, volMesh> field
      (fieldHeader, mesh);
  fvPatchField<Type>& pf = field.boundaryFieldRef()[pI];

  const polyPatch& patch = mesh.boundary()[pI].patch();

  // Reread values and interpolate
  fileName samplePointsFile
  (
      runTime.path()
     /runTime.caseConstant()
     /"boundaryData"
     /patchName
     /"points"
  );
  Info<<"Reading points from "<<samplePointsFile<<endl;

  pointField samplePoints((IFstream(samplePointsFile)()));

  // tbd: run-time selection
  bool nearestOnly =
  (
     !mapMethod.empty()
   && mapMethod != "planarInterpolation"
  );

  pointToPointPlanarInterpolation mapper
  (
      samplePoints,
      patch.faceCentres(),
      perturb,
      nearestOnly
  );

  const fileName samplePointsDir = samplePointsFile.path();
  auto sampleTimes = Time::findTimes(samplePointsDir);

  label lo = -1;
  label hi = -1;

  bool foundTime = mapper.findTime
      (
          sampleTimes,
          0,
          timeValue,
          lo,
          hi
      );

  if (!foundTime)
  {
      FatalErrorInFunction
          << "Cannot find starting sampling values for current time "
          << timeValue << nl
          << "Have sampling values for times "
          << pointToPointPlanarInterpolation::timeNames(sampleTimes) << nl
          << "In directory "
          <<  runTime.constant()/"boundaryData"/patch.name()
          << "\n    on patch " << patch.name()
          << " of field " << fieldName
          << exit(FatalError);
  }

  Field<Type> startSampledValues, endSampledValues;

  if (lo>=0)
  {
    // Reread values and interpolate
    fileName valsFile
    (
        runTime.path()
       /runTime.caseConstant()
       /"boundaryData"
       /patch.name()
       /sampleTimes[lo].name()
       /fieldName
    );

    Info<<"Reading lo time values from "<<valsFile<<endl;

    Field<Type> vals;
    IFstream(valsFile)() >> vals;

    startSampledValues = mapper.interpolate(vals);
  }

  if (hi>=0)
  {
    // Reread values and interpolate
    fileName valsFile
    (
        runTime.path()
       /runTime.caseConstant()
       /"boundaryData"
       /patch.name()
       /sampleTimes[hi].name()
       /fieldName
    );
    Info<<"Reading hi time values from "<<valsFile<<endl;

    Field<Type> vals;
    IFstream(valsFile)() >> vals;

    endSampledValues = mapper.interpolate(vals);
  }

  if (hi>=0 && lo>=0)
  {
    scalar start = sampleTimes[lo].value();
    scalar end = sampleTimes[hi].value();

    scalar s = (timeValue - start)/(end - start);

    pf == ((1 - s)*startSampledValues + s*endSampledValues);
  }
  else if (hi<0 && lo>=0)
  {
    pf == startSampledValues;
  }
  else if (hi>=0 && lo<0)
  {
    pf == endSampledValues;
  }
  else
  {
    FatalErrorIn("main")
        << "internal error: unhandled combination"
        << abort(FatalError);
  }

  field.write();
}





int main(int argc, char *argv[])
{

  argList::validArgs.append("field name");
  argList::validArgs.append("patch name");
  argList::validArgs.append("time value");

#include "setRootCase.H"
#include "createTime.H"
#include "createMesh.H"

  string fieldName(UNIOF_ADDARG(args,0));
  string patchName(UNIOF_ADDARG(args,1));
  scalar timeValue(readScalar(IStringStream(UNIOF_ADDARG(args,2))()));


  IOobject ioo
  (
      fieldName,
      runTime.timeName(),
      mesh,
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
  );

  if (UNIOF_HEADEROK(ioo, volVectorField))
  {
    expand<vector>(mesh, ioo, patchName, timeValue);
  }
  else if (UNIOF_HEADEROK(ioo, volSymmTensorField))
  {
    expand<symmTensor>(mesh, ioo, patchName, timeValue);
  }
  else if (UNIOF_HEADEROK(ioo, volTensorField))
  {
    expand<tensor>(mesh, ioo, patchName, timeValue);
  }
  else if (UNIOF_HEADEROK(ioo, volScalarField))
  {
    expand<scalar>(mesh, ioo, patchName, timeValue);
  }

  return 0;
}
