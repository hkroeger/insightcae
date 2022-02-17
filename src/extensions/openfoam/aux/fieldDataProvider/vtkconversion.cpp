#include "vtkconversion.h"

namespace Foam {

void VTKMeshToOF(
        vtkDataSet *ds,
        pointField &pts, faceList &faces )
{
    pts.resize(ds->GetNumberOfPoints());
    faces.resize(ds->GetNumberOfCells());

    for (size_t i=0; i<ds->GetNumberOfPoints(); ++i)
    {
        auto* x = ds->GetPoint(i);
        pts[i]=point(x[0], x[1], x[2]);
    }

    for (size_t i=0; i<ds->GetNumberOfCells(); ++i)
    {
        auto pts=vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(i, pts);

        face f( pts->GetNumberOfIds() );
        for (int k=0; k<pts->GetNumberOfIds();++k)
            f[k]=pts->GetId(k);
        faces[i]=f;
    }
}

void OFPrimitivePatchToVTK(const pointField &pts, const faceList &faces, vtkPolyData *ds)
{
    setPoints<vtkPolyData>(pts, ds);

    auto cells = vtkSmartPointer<vtkCellArray>::New();
    forAll(faces, fi)
    {
        auto& f = faces[fi];

        cells->InsertNextCell(f.size());
        forAll(f, vi)
        {
            cells->InsertCellPoint(f[vi]);
        }
    }
    ds->SetPolys(cells);
}



} // namespace Foam
