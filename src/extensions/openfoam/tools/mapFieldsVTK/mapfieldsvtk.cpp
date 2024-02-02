
#include "fvCFD.H"

#include <limits>
#include "vtkSmartPointer.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkProbeFilter.h"
#include "vtkIdTypeArray.h"

#include "uniof.h"
#include "vtkconversion.h"


#include "volFields.H"
#include "surfaceFields.H"
#include "IOobjectList.H"

using namespace Foam;



template<class T>
void copyValue(double *vtkCmpts, T& ofCmpts)
{
    for (label i=0; i<pTraits<T>::nComponents; ++i)
    {
        setComponent(ofCmpts, i)=vtkCmpts[i];
    }
}

template<class GeoField>
void ReadAndSetFields
(
    const fvMesh& mesh,
    const IOobjectList& objects,
    vtkProbeFilter* ipf,
    const HashTable<word, word>& fieldMatching
)
{
    // Objects of field type
    IOobjectList fields(objects.
#if OF_VERSION>=060505
                        lookupClass<GeoField>()
#else
                        lookupClass(GeoField::typeName)
#endif
                        );

    auto *validPts = ipf->GetValidPoints();
    auto *ipd = ipf->GetOutput();
    auto *ippd = ipd->GetPointData();
#if OF_VERSION>=060505
    forAllConstIters(
#else
    forAllConstIter(
                IOobjectList,
#endif
                fields,
                fieldIter )
    {
        word targetName = fieldIter()->name();
        word sourceName = targetName;
        if (fieldMatching.found(targetName))
        {
            sourceName = fieldMatching[targetName];
        }

        if (ippd->HasArray(sourceName.c_str()))
        {
            auto *src = ippd->GetArray(sourceName.c_str());

            if (src->GetNumberOfComponents()!=pTraits<typename GeoField::value_type>::nComponents)
            {
                FatalErrorIn("SetFields") << "different number of components for field " << fieldIter()->name()
                                          <<": VTK:"<<label(src->GetNumberOfComponents())
                                         <<", OpenFOAM:"<<pTraits<typename GeoField::value_type>::nComponents
                                        << abort(FatalError);
            }

            GeoField fld(*fieldIter(), mesh);

            Info<< "    Setting " << fld.name() << " from " << sourceName << endl;
            for (vtkIdType i=0; i<validPts->GetNumberOfValues(); ++i)
            {
                label cellI = validPts->GetValue(i);
                copyValue(
                            src->GetTuple(cellI),
                            fld[cellI]
                            );
            }

            fld.write();
        }
    }
}



int main(int argc, char *argv[])
{
  argList::validArgs.append("VTK file");
  argList::validOptions.insert("fieldMatching", "map of extra corresponding source and target fields ( (<target field> <source field name>) ... )");

# include "setRootCase.H"
# include "createTime.H"
# include "createMesh.H"

  string vtkFile(UNIOF_ADDARG(args, 0));

  HashTable<word, word> fieldMatching;
  if (UNIOF_OPTIONFOUND(args, "fieldMatching"))
  {
      fieldMatching = HashTable<word, word>(
                  IStringStream(UNIOF_OPTION(args, "fieldMatching"))()
                  );
  }

  Info << "Reading VTK data." << endl;
  auto r = vtkSmartPointer<vtkGenericDataObjectReader>::New();
  r->SetFileName(vtkFile.c_str());
  r->Update();

  Info << "Performing interpolation." << endl;
  auto targ = vtkSmartPointer<vtkPolyData>::New();
  setPoints<vtkPolyData>(mesh.C().internalField(), targ);


  // Gaussian kernel
//    auto gaussianKernel = vtkSmartPointer<vtkGaussianKernel>::New();
//    gaussianKernel->SetSharpness(2.0);
//    gaussianKernel->SetRadius(12.0);

//    auto ip=vtkSmartPointer<vtkPointInterpolator>::New();
  auto ip=vtkSmartPointer<vtkProbeFilter>::New();
  ip->SetInputData(targ);
  ip->SetSourceData(r->GetOutput());

  ip->Update();

  Info << " Found correspondence for " << label(ip->GetValidPoints()->GetNumberOfValues()) << " out of " << mesh.C().internalField().size() << " cells." << endl;

  IOobjectList objects(mesh, runTime.timeName());

  ReadAndSetFields<volScalarField>(mesh, objects, ip, fieldMatching);
  ReadAndSetFields<volVectorField>(mesh, objects, ip, fieldMatching);
  ReadAndSetFields<volSymmTensorField>(mesh, objects, ip, fieldMatching);
  ReadAndSetFields<volTensorField>(mesh, objects, ip, fieldMatching);


  return 0;
}
