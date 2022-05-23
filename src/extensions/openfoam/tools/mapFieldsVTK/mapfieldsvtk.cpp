
#include "fvCFD.H"

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
    vtkProbeFilter* ipf
)
{
    // Objects of field type
    IOobjectList fields(objects.lookupClass<GeoField>());

    auto *validPts = ipf->GetValidPoints();
    auto *ipd = ipf->GetOutput();
    auto *ippd = ipd->GetPointData();
    forAllConstIters(fields, fieldIter)
    {
        if (ippd->HasArray(fieldIter.val()->name().c_str()))
        {
            auto *src = ippd->GetArray(fieldIter.val()->name().c_str());

            if (src->GetNumberOfComponents()!=pTraits<typename GeoField::value_type>::nComponents)
            {
                FatalErrorIn("SetFields") << "different number of components for field " << fieldIter.val()->name()
                                          <<": VTK:"<<label(src->GetNumberOfComponents())
                                         <<", OpenFOAM:"<<pTraits<typename GeoField::value_type>::nComponents
                                        << abort(FatalError);
            }

            GeoField fld(*fieldIter(), mesh);

            Info<< "    Setting " << fld.name() << endl;
            for (vtkIdType i=0; i<validPts->GetNumberOfValues(); ++i)
            {
                label cellI = validPts->GetValue(i);
                copyValue<typename GeoField::value_type>(
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

# include "setRootCase.H"
# include "createTime.H"
# include "createMesh.H"

  string vtkFile(UNIOF_ADDARG(args, 0));

  auto r = vtkSmartPointer<vtkGenericDataObjectReader>::New();
  r->SetFileName(vtkFile.c_str());
  r->Update();

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

  IOobjectList objects(mesh, runTime.timeName());

  ReadAndSetFields<volScalarField>(mesh, objects, ip);
  ReadAndSetFields<volVectorField>(mesh, objects, ip);
  ReadAndSetFields<volSymmTensorField>(mesh, objects, ip);
  ReadAndSetFields<volTensorField>(mesh, objects, ip);


  return 0;
}
