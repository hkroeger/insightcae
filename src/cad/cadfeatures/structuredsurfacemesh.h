#ifndef INSIGHT_CAD_STRUCTUREDSURFACEMESH_H
#define INSIGHT_CAD_STRUCTUREDSURFACEMESH_H

#include "cadfeature.h"

#include "gp_Trsf.hxx"
#include "gp_Pnt.hxx"

namespace insight {
namespace cad {




/**
 * @brief The StructuredSurfacePatch struct
 * a structured quad mesh on a surface with nu x nv cells.
 * The (nu+1)*(nv+1) points are stored row by row,
 * i.e. point (i,j) is pts[j*(nu+1)+i].
 */
struct StructuredSurfacePatch
{
    int nu, nv;
    std::vector<gp_Pnt> pts;

    inline const gp_Pnt& pt(int i, int j) const
    {
        return pts[j*(nu+1)+i];
    }
};

typedef std::vector<StructuredSurfacePatch> StructuredSurfacePatchList;




/**
 * @brief The StructuredSurfaceMesh class
 * displays structured surface meshes (e.g. the preview of a blockMesh)
 * together with some boundary edges.
 * The patch surfaces are created directly from their triangulation (as in STL).
 * The interior mesh lines are displayed thin and dimmed,
 * the boundary edges like any other geometry.
 * The result can only be used for display.
 */
class StructuredSurfaceMesh
    : public Feature
{
    TopoDS_Shape boundaryEdges_;
    StructuredSurfacePatchList patches_;
    gp_Trsf trsf_;

    TopoDS_Shape transformedBoundaryEdges_, faces_;

    StructuredSurfaceMesh(const StructuredSurfaceMesh&o, TreeCloneMap& tcm);

    StructuredSurfaceMesh(
        const TopoDS_Shape& boundaryEdges,
        const StructuredSurfacePatchList& patches,
        const gp_Trsf& trsf = gp_Trsf() );

    size_t calcHash() const override;
    void build() override;

public:
    declareType("StructuredSurfaceMesh");

    CREATE_FUNCTION(StructuredSurfaceMesh);
    CLONEABLE(StructuredSurfaceMesh);

    void replaceDependency(const DependencyReplacement& repl) override;
    void addDependencies(DependencyList& dl) const override;

    VTKActorList createVTKActors() const override;
};




}
}

#endif // INSIGHT_CAD_STRUCTUREDSURFACEMESH_H
