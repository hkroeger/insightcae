#ifndef INSIGHT_BMD_BLOCKMESHSMOOTHER_H
#define INSIGHT_BMD_BLOCKMESHSMOOTHER_H

#include "openfoam/blockmesh/blockmeshprediction.h"
#include "openfoam/blockmesh/periodicfacesize.h"

#include <functional>
#include <memory>
#include <set>
#include <string>

#include "boost/filesystem/path.hpp"

namespace insight { class ActionProgress; }

namespace insight {
namespace bmd {




struct SmoothingParameters
{
    int nSweeps = 5;                //!< max. number of sweeps over all movable vertices and edges
    int maxCells = 20000;           //!< max. number of cells of the reduced-resolution mesh used by the optimizer
    int checkMaxCells = 1000000;    //!< resolution of the check (0: no check). Each local modification has to improve the quality at this resolution as well. If the overall quality got worse, the original blocking is restored
    double p = 8.;                  //!< exponent of the soft-max (p-norm) objective
    int minCellsPerBlockDir = 4;    //!< min. number of cells per block and direction of the reduced-resolution meshes (exceeds maxCells, if required)
    double vertexStepRel = 0.1;     //!< initial optimizer step of vertex moves, relative to shortest adjacent edge
    double rotationStepDeg = 3.;    //!< initial optimizer step of the rotation of the edge tangents at a vertex
    double maxTangentRotationDeg = 15.; //!< max. rotation of the edge tangents at a vertex
    double maxBowRel = 0.1;         //!< max. deviation of a spline from its original shape (after following the vertex moves), relative to the edge length
    double fairnessWeight = 1e-2;   //!< weight of the penalty on tangent rotations and edge bows, relative to the local objective
    bool edgeBowModes = false;      //!< additionally optimize a single bow mode per edge (normal to the chord, 2 DoF)
    double edgeStepRel = 0.02;      //!< initial optimizer step of the edge bow amplitudes, relative to edge length
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
     * labels of projection surfaces, on which the vertices and edges stay fixed,
     * even if a projection is known (it is used by the mesh prediction only)
     */
    std::set<std::string> fixedSurfaces;

    /**
     * transformations between periodic (cyclic) boundaries.
     * Each boundary vertex v, for which T(v) coincides with another boundary vertex w,
     * is paired with w: w follows v, edges between paired vertices are transformed accordingly.
     */
    std::vector<PointFunction> periodicTransforms;

    /**
     * geometry labels (side A, side B) of the projected block faces on the periodic boundaries,
     * one entry per entry of periodicTransforms (which maps side A onto side B).
     * Required for the face size limit (minPeriodicFaceAreaRatio), also for non-conformal periodic boundaries.
     */
    std::vector<std::pair<std::string,std::string> > periodicSurfaces;

    /**
     * min. area of a cell face on a periodic boundary relative to the opposite face on the other side
     * (see PeriodicFaceSizeCheck). Faces may not get smaller than this limit or, if they are already smaller
     * in the unmodified blocking, than there. Prevents slivers, which make stitching of rotated copies fail.
     * 0: no limit
     */
    double minPeriodicFaceAreaRatio = 0.1;
};




/**
 * @brief The BlockingModification struct
 * result of the block smoothing, which can be re-applied to the same blocking or to
 * a topologically identical but geometrically slightly different blocking.
 *
 * Stored are the degrees of freedom of the smoother relative to the unmodified blocking,
 * in a local frame of each vertex (from the directions of its block edges, ordered by vertex index)
 * and relative to the local edge lengths:
 * vertices: one row per moved vertex [vertex index, displacement (3), tangent rotation (3)],
 *  the displacement is relative to the shortest adjacent block edge;
 * edges: one row per bent edge [vertex index 0, vertex index 1, bow amplitude (3)],
 *  the amplitude is in the frame of vertex 0 and relative to the edge length.
 */
struct BlockingModification
{
    std::string topology;   //!< fingerprint of the blocking topology (see topologyFingerprint)
    arma::mat vertices = arma::zeros(0, 7);
    arma::mat edges = arma::zeros(0, 5);
    std::string summary;    //!< quality before and after the optimization (information only)

    inline bool empty() const { return topology.empty(); }

    void write(const boost::filesystem::path& file) const;
    static BlockingModification read(const boost::filesystem::path& file);
};


/**
 * fingerprint of the topology of a blocking: number of vertices and vertex indices of the blocks
 */
std::string topologyFingerprint(const BlockingModel& m);




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
 * Each sweep solves small local problems for each movable vertex: its displacement (3 parameters)
 * and a rotation of the end tangents of all attached splines (3 parameters).
 * The splines are not optimized independently: their shape follows from the original shape,
 * the displacements of their end vertices (linear blending) and the tangent rotations at
 * their ends (cubic Hermite correction). All edges at a vertex rotate together, so the angles
 * between them are kept (no new kinks at the vertices). The deviation from the original shape
 * is bounded (SmoothingParameters::maxBowRel, maxTangentRotationDeg) and penalized (fairnessWeight).
 * Optionally (SmoothingParameters::edgeBowModes), a single bow mode sin(pi t) normal to the chord
 * is added per edge (2 parameters).
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
    bool acceptMove(
        const std::set<int>& blocks, double fCheckBefore, double faceSizeViolationCheckBefore,
        const std::function<void()>& revert );

    // size limit of the faces on the periodic boundaries, for the optimizer and for the check grid,
    // and the area ratios of the unmodified blocking (faces below the limit there may not get smaller)
    std::vector<std::unique_ptr<PeriodicFaceSizeCheck> > faceSize_, faceSizeCheck_;
    std::vector<std::vector<double> > faceSizeRef_, faceSizeCheckRef_;
    void setupFaceSizeChecks(
        const MeshPredictor& mp,
        std::vector<std::unique_ptr<PeriodicFaceSizeCheck> >& checks,
        std::vector<std::vector<double> >& reference ) const;
    /**
     * sum of the violations of the face size limit of the periodic boundaries touched by the blocks
     * (0 if none), on the optimizer grid or on the check grid
     */
    double faceSizeViolation(bool onCheckGrid, const std::set<int>& blocks) const;
    double minAdjacentEdgeLength(int v) const;
    void updateSlaveEdge(int e);

    // shape of the splines in terms of the vertex displacements and tangent rotations
    struct EdgeShape
    {
        bool active = false;        // shape is regenerated (non-slave splines)
        std::vector<V3> pts0;       // original support points
        std::vector<double> t;      // their chord length parameters
        V3 tau0, tau1;              // original unit tangents at both ends (in direction v0 -> v1)
        V3 bow;                     // amplitude of the bow mode
    };
    std::vector<EdgeShape> shape_;
    std::vector<V3> origVertices_;
    std::vector<V3> vertexRotation_;  // rotation vectors of the edge tangents at the vertices

    void initShapes();
    V3 followedPoint(int e, size_t i) const;      // original shape, following the end vertices
    V3 deformedPoint(int e, size_t i) const;      // before projection onto constraints
    void regenerateEdge(int e);
    double edgeDeformationRel(int e) const;
    bool admissibleEdge(int e) const;

    // orientation of the block faces (blockMesh rejects blocks with inward-pointing faces)
    std::vector<std::array<double,6> > faceOrientation0_;
    double faceOrientation(int bi, int f) const;
    bool blockFacesValid(const std::set<int>& blocks) const;

    bool optimizeVertex(int v);
    bool optimizeEdge(int e);

    // classification, conversion of edges and initialization of the shapes (once)
    bool setupDone_ = false;
    std::string topology_;
    void setup();

    /**
     * local frame of vertex v in the unmodified blocking
     * (from the directions to the adjacent vertices, ordered by vertex index)
     * and its length scale (shortest adjacent block edge)
     */
    bool localFrame(int v, std::array<V3,3>& R, double& h) const;

public:
    BlockMeshSmoother(MeshPredictor& mp, const SmoothingParameters& par);

    /**
     * apply a stored modification to the (unmodified) blocking.
     * Stored vertices, which are fixed in this blocking, are skipped.
     * @return false, if the topology does not match or the modified blocks are invalid
     * (blocking is left unmodified). A description of the outcome is put into message.
     */
    bool apply(const BlockingModification& mod, std::string* message = nullptr);

    /**
     * the current modification relative to the unmodified blocking
     */
    BlockingModification extract() const;

    /**
     * run the optimization. Progress is reported to the given action,
     * which may be stopped (current, improved state is kept).
     * @param initial
     * optional: modification, from which the optimization starts (warm start).
     * The result (extract()) is still relative to the unmodified blocking.
     */
    NonOrthogonalityStatistics run(
        ActionProgress& progress,
        const BlockingModification* initial = nullptr );

    /**
     * sizes of the faces on the periodic boundaries in the current state
     * (empty, if there is no face size limit)
     */
    std::string periodicFaceSizeSummary() const;
};




} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_BLOCKMESHSMOOTHER_H
