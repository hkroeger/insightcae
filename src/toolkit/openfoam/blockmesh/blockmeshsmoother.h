#ifndef INSIGHT_BMD_BLOCKMESHSMOOTHER_H
#define INSIGHT_BMD_BLOCKMESHSMOOTHER_H

#include "openfoam/blockmesh/blockmeshprediction.h"

#include <functional>
#include <memory>

namespace insight { class ActionProgress; }

namespace insight {
namespace bmd {




struct SmoothingParameters
{
    int nSweeps = 5;                //!< max. number of sweeps over all movable vertices and edges
    int maxCells = 20000;           //!< max. number of cells of the reduced-resolution mesh used by the optimizer
    int checkMaxCells = 1000000;    //!< resolution of the check (0: no check). Each local modification has to improve the quality at this resolution as well. If the overall quality got worse, the original blocking is restored
    double p = 8.;                  //!< exponent of the soft-max (p-norm) objective
    double vertexStepRel = 0.1;     //!< initial optimizer step of vertex moves, relative to shortest adjacent edge
    double edgeStepRel = 0.05;      //!< initial optimizer step of edge mode amplitudes, relative to edge length
    int maxIterLocal = 60;          //!< max. optimizer iterations per local problem
    double relTol = 1e-3;           //!< stop, if relative improvement of objective per sweep is smaller
    int nSupportPointsConverted = 3;//!< number of support points of splines replacing straight edges

    typedef std::function<V3(const V3&)> PointFunction;

    /**
     * move vertices and edges on boundary faces along the boundary:
     * along planar, unprojected boundary faces and along projection surfaces, for which a projection is given
     * (blockMeshBlocking::setSurfaceProjection).
     * Vertices on more than two such surfaces, on other boundary faces, unknown projection surfaces
     * or non-spline edges stay fixed.
     * Note: matching of cyclic patches is only kept, if their transformation is given in periodicTransforms.
     */
    bool slideOnBoundaries = false;

    /**
     * transformations between periodic (cyclic) boundaries.
     * Each boundary vertex v, for which T(v) coincides with another boundary vertex w,
     * is paired with w: w follows v, edges between paired vertices are transformed accordingly.
     */
    std::vector<PointFunction> periodicTransforms;
};




/**
 * @brief The BlockMeshSmoother class
 * moves interior block vertices and the support points of interior spline edges
 * to minimize the non-orthogonality of the predicted mesh.
 *
 * Fixed are: vertices and edges on boundary faces of the blocking (faces not shared between two blocks),
 * projected vertices, projected edges and their end points.
 * Optionally (SmoothingParameters::slideOnBoundaries), boundary vertices and edges slide along the boundary.
 * Interior straight edges and arcs are converted into splines before optimization.
 *
 * Each sweep solves small local problems: the displacement of each movable vertex (3 parameters,
 * attached splines follow), and the shape of each movable edge, parametrized by smooth modes
 * d(t) = sum_{k=1..2} a_k sin(k pi t) of the support points (6 parameters).
 */
class BlockMeshSmoother
{
    MeshPredictor& mp_;
    SmoothingParameters par_;

    // surface, on which boundary vertices and edges slide
    struct Constraint
    {
        std::string label;      // geometry label, empty for planes
        V3 n;                   // plane: unit normal
        double d = 0.;          // plane: n.x = d
        V3 project(const V3& p, const BlockingModel& m) const;
    };
    std::vector<Constraint> constraints_;
    double tol_ = 0.;           // geometric tolerance (relative to domain size)

    typedef std::set<int> ConstraintSet;
    std::vector<bool> fixedVertex_;
    std::vector<ConstraintSet> vertexConstraints_;
    std::map<std::pair<int,int>, ConstraintSet> edgeConstraints_; // (min,max) vertex pairs, boundary edges only
    std::set<std::pair<int,int> > boundaryEdges_; // (min,max) vertex pairs
    std::set<std::pair<int,int> > fixedEdges_;    // boundary edges, which may not move
    std::vector<std::set<int> > blocksOfVertex_;
    std::vector<std::set<int> > edgesOfVertex_;

    // periodic pairs
    std::vector<int> slaveVertex_, masterVertex_;   // -1: none
    std::vector<int> slaveEdge_, masterEdge_;       // -1: none
    std::vector<int> transformOfVertex_;            // index into periodicTransforms for master vertices
    std::vector<bool> slaveReversed_;               // slave edge runs opposite to its master

    V3 projectOnto(const ConstraintSet& cs, const V3& p) const;
    ConstraintSet constraintsOfEdge(int va, int vb) const;

    void classify();
    void convertEdges();
    void findPeriodicPairs();
    bool isMovableEdge(int e) const;
    std::set<int> blocksOfEdge(int e) const;
    double penalizedObjective(const std::set<int>& blocks) const;
    double penalizedObjective(MeshPredictor& mp, const std::set<int>& blocks) const;

    // finer prediction (check resolution) for verifying each local modification
    std::unique_ptr<MeshPredictor> check_;
    int nRejectedByCheck_ = 0;
    void syncCheck(const std::set<int>& blocks);
    bool acceptMove(const std::set<int>& blocks, double fCheckBefore, const std::function<void()>& revert);
    double minAdjacentEdgeLength(int v) const;
    void updateSlaveEdge(int e);

    bool optimizeVertex(int v);
    bool optimizeEdge(int e);

public:
    BlockMeshSmoother(MeshPredictor& mp, const SmoothingParameters& par);

    /**
     * run the optimization. Progress is reported to the given action,
     * which may be stopped (current, improved state is kept).
     */
    NonOrthogonalityStatistics run(ActionProgress& progress);
};




} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_BLOCKMESHSMOOTHER_H
