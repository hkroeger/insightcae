#include "structuredsurfacemesh.h"

#include "ivtkoccshape.h"
#include "parameterlisthash.h"
#include "base/tools.h"

#include "BRep_Builder.hxx"
#include "BRepBuilderAPI_Transform.hxx"
#include "Geom_Plane.hxx"
#include "Poly_Triangulation.hxx"
#include "TopoDS_Compound.hxx"
#include "TopoDS_Face.hxx"
#include "TopLoc_Location.hxx"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkUnsignedCharArray.h"


namespace insight {
namespace cad {




defineType(StructuredSurfaceMesh);




size_t StructuredSurfaceMesh::calcHash() const
{
    ParameterListHash h;
    h+=this->type();
    h+=boundaryEdges_;
    h+=trsf_;
    for (const auto& p: patches_)
    {
        h+=p.nu;
        h+=p.nv;
        for (const auto& pt: p.pts)
        {
            h+=pt;
        }
    }
    return h.getHash();
}




StructuredSurfaceMesh::StructuredSurfaceMesh(
    const StructuredSurfaceMesh&o, TreeCloneMap& tcm )
    : Feature(o, tcm),
      boundaryEdges_(o.boundaryEdges_),
      patches_(o.patches_),
      trsf_(o.trsf_)
{}




StructuredSurfaceMesh::StructuredSurfaceMesh(
    const TopoDS_Shape& boundaryEdges,
    const StructuredSurfacePatchList& patches,
    const gp_Trsf& trsf )
    : boundaryEdges_(boundaryEdges),
      patches_(patches),
      trsf_(trsf)
{
    for (const auto& p: patches_)
    {
        insight::assertion(
            (p.nu>0) && (p.nv>0) && (p.pts.size()==size_t((p.nu+1)*(p.nv+1))),
            "inconsistent structured surface patch" );
    }
}




void StructuredSurfaceMesh::build()
{
    ExecTimer t("StructuredSurfaceMesh::build() ["+featureSymbolName()+"]");

    BRep_Builder bb;

    TopoDS_Compound faces;
    bb.MakeCompound(faces);

    for (const auto& p: patches_)
    {
        Handle_Poly_Triangulation tri =
            new Poly_Triangulation( p.pts.size(), 2*p.nu*p.nv, Standard_False );

        for (size_t k=0; k<p.pts.size(); ++k)
        {
            tri->
#if OCC_VERSION_MAJOR<7
                ChangeNodes().ChangeValue(k+1)
#else
                ChangeNode (k+1)
#endif
                = p.pts[k].Transformed(trsf_);
        }

        auto node = [&](int i, int j) { return j*(p.nu+1)+i+1; };
        int l=1;
        for (int j=0; j<p.nv; ++j)
        {
            for (int i=0; i<p.nu; ++i)
            {

                tri->
#if OCC_VERSION_MAJOR<7
                    ChangeTriangles().ChangeValue(l++)
#else
                    ChangeTriangle (l++)
#endif
                    =
                    Poly_Triangle( node(i,j), node(i+1,j), node(i+1,j+1) );

                tri->
#if OCC_VERSION_MAJOR<7
                    ChangeTriangles().ChangeValue(l++)
#else
                    ChangeTriangle (l++)
#endif
                    =
                    Poly_Triangle( node(i,j), node(i+1,j+1), node(i,j+1) );

            }
        }

        // the surface is just a placeholder, as for STL
        TopoDS_Face f;
        bb.MakeFace(f, new Geom_Plane(gp::XOY()), Precision::Confusion());
        bb.UpdateFace(f, tri);
        TopLoc_Location nl;
        nl.Identity();
        f.Location(nl);

        bb.Add(faces, f);
    }
    faces_=faces;

    if (!boundaryEdges_.IsNull())
        transformedBoundaryEdges_ =
            BRepBuilderAPI_Transform(boundaryEdges_, trsf_, true).Shape();
    else
        transformedBoundaryEdges_ = TopoDS_Shape();

    TopoDS_Compound all;
    bb.MakeCompound(all);
    bb.Add(all, faces_);
    if (!transformedBoundaryEdges_.IsNull())
        bb.Add(all, transformedBoundaryEdges_);

    refvalues_["isSTLGeometry"]=1;

    setShape(all);
}




void StructuredSurfaceMesh::replaceDependency(const DependencyReplacement&)
{}

void StructuredSurfaceMesh::addDependencies(DependencyList&) const
{}




VTKActorList StructuredSurfaceMesh::createVTKActors() const
{
    shape(); // trigger build

    VTKActorList actors;

    auto addOCCShapeActor = [&](const TopoDS_Shape& s)
    {
        auto src = vtkSmartPointer<ivtkOCCShape>::New();
        src->SetShape( s );

        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
        actor->GetMapper()->SetInputConnection(src->GetOutputPort());
        actors.push_back(actor);
    };

    // boundary edges: displayed in all representations like other geometry
    if (!transformedBoundaryEdges_.IsNull())
        addOCCShapeActor(transformedBoundaryEdges_);

    // surfaces: shaded in surface representation, invisible in wireframe
    addOCCShapeActor(faces_);

    // interior mesh lines: thin and dimmed.
    // Colored by direct scalars, so that they are not affected
    // by the color, which the viewer assigns to the entity.
    {
        auto points = vtkSmartPointer<vtkPoints>::New();
        auto lines = vtkSmartPointer<vtkCellArray>::New();
        auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
        colors->SetNumberOfComponents(4);
        colors->SetName("colors");
        const unsigned char dimmed[4] = { 64, 64, 64, 90 };

        auto addLine = [&](const std::vector<gp_Pnt>& pl)
        {
            std::vector<vtkIdType> ids;
            for (const auto& p: pl)
            {
                auto tp = p.Transformed(trsf_);
                ids.push_back(points->InsertNextPoint(tp.X(), tp.Y(), tp.Z()));
            }
            lines->InsertNextCell(ids.size(), ids.data());
            colors->InsertNextTypedTuple(dimmed);
        };

        for (const auto& p: patches_)
        {
            for (int i=1; i<p.nu; ++i)
            {
                std::vector<gp_Pnt> pl;
                for (int j=0; j<=p.nv; ++j) pl.push_back(p.pt(i,j));
                addLine(pl);
            }
            for (int j=1; j<p.nv; ++j)
            {
                std::vector<gp_Pnt> pl;
                for (int i=0; i<=p.nu; ++i) pl.push_back(p.pt(i,j));
                addLine(pl);
            }
        }

        if (lines->GetNumberOfCells()>0)
        {
            auto pd = vtkSmartPointer<vtkPolyData>::New();
            pd->SetPoints(points);
            pd->SetLines(lines);
            pd->GetCellData()->SetScalars(colors);

            auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputData(pd);
            mapper->SetScalarModeToUseCellData();
            mapper->SetColorModeToDirectScalars();
            mapper->ScalarVisibilityOn();

            auto actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetLineWidth(1);
            actors.push_back(actor);
        }
    }

    return actors;
}




}
}
