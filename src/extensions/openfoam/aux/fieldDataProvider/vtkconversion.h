#ifndef FOAM_VTKCONVERSION_H
#define FOAM_VTKCONVERSION_H

#include "fvCFD.H"

#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"

namespace Foam {

template<class T>
void copyFieldToVTKArray(
        const Field<T>& field,
        vtkDoubleArray* data
        )
{
    data->SetNumberOfComponents(pTraits<T>::nComponents);
    data->SetNumberOfTuples(field.size());
    for (size_t j=0;j<field.size();++j)
    {
        auto& d=field[j];
        double t[pTraits<T>::nComponents];
        for (int k=0; k<pTraits<T>::nComponents; ++k)
        {
            t[k]=component(d, k);
        }
        data->SetTuple(j, t);
    }
}

template<class T>
void addFieldToVTK(const std::pair<std::string, const Field<T>&>& f, vtkDataSetAttributes* dsa)
{
    auto arr = vtkSmartPointer<vtkDoubleArray>::New();
    arr->SetName(f.first.c_str());
    copyFieldToVTKArray<T>(f.second, arr);
    dsa->AddArray(arr);
}


template<class T>
vtkDoubleArray* operator<<(vtkDoubleArray* data, const Field<T>& field)
{
    copyFieldToVTKArray(field, data);
    return data;
}


template<class T>
void setPoints(
        const pointField& points,
        T* vtkDS
        )
{
    auto vtkPts = vtkSmartPointer<vtkPoints>::New();

    vtkPts->SetNumberOfPoints(points.size());
    forAll(points, j)
    {
        auto& p=points[j];
        vtkPts->SetPoint(j, p.x(), p.y(), p.z());
    }

    vtkDS->SetPoints(vtkPts);
}


template<class T>
vtkSmartPointer<vtkPolyData>
pointCloudFromPointData(
        const pointField& points,
        const Field<T>& pointData
        )
{
    if (points.size()!=pointData.size())
    {
        FatalErrorIn("pointCloudFromPointData")
                << "incompatible size of points list and data array"
                << abort(FatalError);
    }

    auto ds = vtkSmartPointer<vtkPolyData>::New();
    setPoints<vtkPolyData>(points, ds);

    auto data = vtkSmartPointer<vtkDoubleArray>::New();
    data << pointData;
    ds->GetPointData()->AddArray(data);

    return ds;
}


template<class T, class PatchType>
vtkSmartPointer<vtkPolyData>
pointCloudFromCellData(
        const PatchType& mesh,
        const Field<T>& cellData
        )
{
    if (mesh.size()!=cellData.size())
    {
        FatalErrorIn("pointCloudFromCellData")
                << "incompatible size of cell list and data array"
                << abort(FatalError);
    }

    auto ds = vtkSmartPointer<vtkPolyData>::New();
    setPoints<vtkPolyData>(mesh.faceCentres(), ds);

    auto data = vtkSmartPointer<vtkDoubleArray>::New();
    data << cellData;
    ds->GetPointData()->AddArray(data);

    return ds;
}


template<class T>
tmp<Field<T> >
VTKArrayToField(
        vtkDataArray* arr
        )
{
    if (arr->GetNumberOfComponents()!=pTraits<T>::nComponents)
    {
        FatalErrorIn("VTKArrayToField")
                << "incompatible number of components"
                << abort(FatalError);
    }

    tmp<Field<T> > result(new Field<T>(arr->GetNumberOfTuples()));
    for (size_t j=0; j<arr->GetNumberOfTuples(); ++j)
    {
        for (int k=0; k<pTraits<T>::nComponents; ++k)
        {
            auto *src = arr->GetTuple(j);
            T& targ = UNIOF_TMP_NONCONST(result)[j];

            setComponent(targ, k) = src[k];
        }
    }
    return result;
}


void VTKMeshToOF(
        vtkDataSet* ds,
        pointField& pts,
        faceList& faces );

void OFPrimitivePatchToVTK(
        const pointField& pts,
        const faceList& faces,
        vtkPolyData* ds );

} // namespace Foam

#endif // FOAM_VTKCONVERSION_H
