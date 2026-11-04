#include "blockmeshcadgeometry.h"

#include "cadfeature.h"
#include "cadfeatures/stl.h"
#include "occbndbox.h"

#include "BRepBuilderAPI_Copy.hxx"
#include "BRepTools.hxx"
#include "BRepMesh_IncrementalMesh.hxx"
#include "BRep_Tool.hxx"
#include "TopExp_Explorer.hxx"
#include "TopoDS.hxx"
#include "Poly_Triangulation.hxx"

#include "vtkPoints.h"
#include "vtkCellArray.h"

namespace insight {
namespace bmd {


namespace {

/**
 * the existing triangulations of all faces of shape s
 */
vtkSmartPointer<vtkPolyData> triangulationToVTK(const TopoDS_Shape& s)
{
    auto pts = vtkSmartPointer<vtkPoints>::New();
    auto cells = vtkSmartPointer<vtkCellArray>::New();

    for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next())
    {
        const auto& f=TopoDS::Face(ex.Current());
        TopLoc_Location loc;
        auto mesh = BRep_Tool::Triangulation(f, loc);
        insight::assertion(!mesh.IsNull(), "face has no triangulation");

        auto trsf = loc.Transformation();
        vtkIdType ofs = pts->GetNumberOfPoints();
        for (int i=1; i<=mesh->NbNodes(); ++i)
        {
            auto p = mesh->
#if OCC_VERSION_MAJOR<7
                     Nodes().Value(i)
#else
                     Node(i)
#endif
                         .Transformed(trsf);
            pts->InsertNextPoint(p.X(), p.Y(), p.Z());
        }
        for (int i=1; i<=mesh->NbTriangles(); ++i)
        {
            int n1, n2, n3;
            mesh->
#if OCC_VERSION_MAJOR<7
                Triangles().Value(i)
#else
                Triangle(i)
#endif
                .Get(n1, n2, n3);
            if (f.Orientation()==TopAbs_REVERSED) std::swap(n2, n3); // like StlAPI_Writer
            vtkIdType vtx[]={ofs+n1-1, ofs+n2-1, ofs+n3-1};
            cells->InsertNextCell(3, vtx);
        }
    }

    auto pd = vtkSmartPointer<vtkPolyData>::New();
    pd->SetPoints(pts);
    pd->SetPolys(cells);
    return pd;
}

}



CADGeometry::CADGeometry(
    const std::string& label,
    const boost::filesystem::path& fileName,
    cad::FeaturePtr feature,
    double tolerance )
    : Geometry(label, fileName),
      feature_(feature),
      tolerance_(tolerance),
      surface_(std::make_shared<Surface>())
{
    insight::assertion(bool(feature_), "geometry \"%s\": no feature given", label.c_str());
}


vtkSmartPointer<vtkPolyData> CADGeometry::surface() const
{
    std::lock_guard<std::mutex> l(surface_->mutex);

    if (!surface_->surface)
    {
        insight::CurrentExceptionContext ex(
            "triangulating the geometry \"%s\" for projection", c_str() );

        // the shapes may be shared with other threads (feature cache)
        std::lock_guard<std::recursive_mutex> lock(cad::shapeModificationMutex());

        if (dynamic_cast<const cad::STL*>(feature_.get()))
        {
            // already triangulated
            surface_->surface = feature_->triangulationToVTK();
        }
        else
        {
            // like Feature::exportSTL: triangulate an independent copy
            // with the requested absolute deflection
            // (an existing triangulation of the shape, e.g. for display, might be coarser)
            BRepBuilderAPI_Copy aCopy( feature_->shape(), Standard_False );
            TopoDS_Shape os=aCopy.Shape();
            BRepTools::Clean( os );
            BRepMesh_IncrementalMesh binc(os, tolerance_);

            surface_->surface = triangulationToVTK(os);
        }
    }

    return surface_->surface;
}


} // namespace bmd
} // namespace insight
