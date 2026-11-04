#include "geometry.h"

#include "base/exception.h"

#include "vtkCellLocator.h"
#include "vtkSTLWriter.h"
#include "vtkTriangleFilter.h"

namespace insight {
namespace bmd {


Geometry::Geometry(
        const std::string& label,
        const boost::filesystem::path& fileName)
    : std::string(label),
      fileName_(fileName),
      locator_(std::make_shared<Locator>())
{}

Geometry::~Geometry()
{}

const boost::filesystem::path& Geometry::fileName() const
{
    return fileName_;
}

vtkSmartPointer<vtkPolyData> Geometry::surface() const
{
    return nullptr;
}

SurfaceProjection Geometry::projection() const
{
    auto surf = surface();
    if (!surf || surf->GetNumberOfCells()==0)
        return SurfaceProjection();

    auto l = locator_;
    return [l,surf](const V3& p)
    {
        // vtkCellLocator is not thread-safe
        std::lock_guard<std::mutex> lock(l->mutex);
        if (!l->initialized)
        {
            l->locator = vtkSmartPointer<vtkCellLocator>::New();
            l->locator->SetDataSet(surf);
            l->locator->BuildLocator();
            l->initialized = true;
        }
        double x[3]={p.x, p.y, p.z}, cp[3];
        vtkIdType cellId;
        int subId;
        double dist2;
        l->locator->FindClosestPoint(x, cp, cellId, subId, dist2);
        if (cellId<0) return p;
        return V3(cp[0], cp[1], cp[2]);
    };
}

void Geometry::writeFile(const boost::filesystem::path& dir) const
{
    auto surf = surface();
    if (!surf) return;

    auto fn = dir/fileName_;
    boost::filesystem::create_directories(fn.parent_path());

    auto ext = fn.extension().string();
    insight::assertion(
        ext==".stl" || ext==".stlb",
        "geometry \"%s\": only STL output is supported (file name %s)",
        c_str(), fn.string().c_str() );

    auto tri = vtkSmartPointer<vtkTriangleFilter>::New();
    tri->SetInputData(surf);

    auto w = vtkSmartPointer<vtkSTLWriter>::New();
    w->SetInputConnection(tri->GetOutputPort());
    w->SetFileName(fn.string().c_str());
    if (ext==".stlb")
        w->SetFileTypeToBinary();
    else
        w->SetFileTypeToASCII();
    w->Write();
}


} // namespace bmd
} // namespace insight
