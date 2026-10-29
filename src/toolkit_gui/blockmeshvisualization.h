#ifndef INSIGHT_BLOCKMESHVISUALIZATION_H
#define INSIGHT_BLOCKMESHVISUALIZATION_H

#include "toolkit_gui_export.h"

#include "TopoDS_Edge.hxx"
#include "TopoDS_Compound.hxx"
#include <array>
#include "gp_Trsf.hxx"
#include "base/parameters/spatialtransformationparameter.h"
#include "openfoam/blockmesh.h"
#include "cadfeatures/structuredsurfacemesh.h"
#include "openfoam/blockmesh/blockmeshprediction.h"

#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

namespace insight {

class CADParameterSetModelVisualizer;

namespace bmd
{


/**
 * @brief The blockMeshVisualization class
 * converts the blocks of a blockMeshBlocking into a displayable representation.
 * For each block, its edges are created. Curved edges (arcs, splines) are reproduced,
 * all other edges are drawn as straight lines.
 * Additionally, a preview of the mesh on all faces of each block is created,
 * which takes the grading into account. The resolution of the preview is scaled down,
 * if required, to limit the total number of displayed faces.
 * Projections of vertices, edges and faces onto geometries are applied as in blockMesh.
 * This is the common basis for all visualizations of blockMesh data.
 */
class TOOLKIT_GUI_EXPORT blockMeshVisualization
{
public:
    /**
     * @brief TransformFunction
     * maps the points of the blocking (blockMesh coordinates) to the displayed location
     */
    typedef std::function<arma::mat(const arma::mat&)> TransformFunction;
    static TransformFunction NoTransformFunction;

    typedef boost::variant<double,gp_Trsf,insight::SpatialTransformation> ScaleOrTransform;

    struct BlockVisualization
    {
        TopoDS_Compound edges;
        cad::StructuredSurfacePatchList meshPreview;

        /**
         * faces (in OpenFOAM hex face order), which are not outward-pointing.
         * blockMesh rejects such blocks (see Foam::blockDescriptor::check)
         */
        std::vector<int> inwardFaces;

        /**
         * all faces point inwards
         */
        bool insideOut = false;

        inline bool isInvalid() const { return !inwardFaces.empty(); }

        /**
         * the displayed edges in OpenFOAM hex edge order
         */
        std::array<TopoDS_Edge,12> displayEdges;

        /**
         * mean of the displayed corner locations
         */
        arma::mat centre;
    };

private:
    /**
     * @brief The EdgeKeyLess struct
     * orders point pairs independently of their direction
     */
    struct EdgeKeyLess
    {
        bool operator()(
            const std::pair<Point,Point>& e1,
            const std::pair<Point,Point>& e2 ) const;
    };

    struct ProjectionSurface;

    const bmd::blockMeshBlocking& bm_;
    TransformFunction tf_;
    boost::filesystem::path geometryDir_;

    /**
     * the geometries for projection, loaded on demand.
     * Null, if the geometry could not be loaded.
     */
    std::map<std::string, std::shared_ptr<ProjectionSurface> > surfaces_;

    /**
     * edges are shared between adjacent blocks,
     * so each edge geometry is created only once
     */
    std::map<std::pair<Point,Point>, TopoDS_Edge, EdgeKeyLess> edges_;

    std::vector<BlockVisualization> blocks_;
    double previewResolutionFactor_;

    const ProjectionSurface* projectionSurface(const std::string& label);
    const ProjectionSurface* edgeProjectionSurface(const arma::mat& p0, const arma::mat& p1);

    /**
     * @brief vertexPosition
     * location of the vertex, projected, if requested
     */
    gp_Pnt vertexPosition(const Point& p);

    static std::vector<gp_Pnt> projectedLine(
        const gp_Pnt& x0, const gp_Pnt& x1,
        const ProjectionSurface& surf,
        const std::vector<double>& lambda );

    static void projectFace(
        cad::StructuredSurfacePatch& p,
        const ProjectionSurface& surf );

    typedef std::function<gp_Pnt(const gp_Pnt&)> PointMapping;

    /**
     * @brief buildEdge
     * creates the edge between the vertices p0 and p1.
     * The curve is evaluated in blockMesh coordinates and its defining points are mapped by map.
     */
    TopoDS_Edge buildEdge(
        const arma::mat& p0, const arma::mat& p1,
        const PointMapping& map );

    /**
     * @brief edge
     * the (cached) edge between p0 and p1 in blockMesh coordinates
     */
    TopoDS_Edge edge( const arma::mat& p0, const arma::mat& p1 );

    gp_Pnt transformed(const gp_Pnt& x) const;

    /**
     * @brief edgePoints
     * distributes the points of n cells with expansion ratio grading
     * along the edge from p0 to p1.
     */
    std::vector<gp_Pnt> edgePoints(
        const arma::mat& p0, const arma::mat& p1,
        int n, double grading );

public:
    /**
     * @param tf
     * transformation of the blocking points into the displayed locations.
     * Mesh points and projections are computed in blockMesh coordinates,
     * the results are transformed.
     * @param maxPreviewFaces
     * upper limit of the number of displayed mesh faces. 0 disables the mesh preview.
     * @param geometryDir
     * directory, in which relative file names of the geometries for projection are searched
     * (directly and in constant/triSurface, like blockMesh does)
     */
    blockMeshVisualization(
        const bmd::blockMeshBlocking& bm,
        TransformFunction tf = NoTransformFunction,
        int maxPreviewFaces = 50000,
        const boost::filesystem::path& geometryDir = boost::filesystem::path() );

    inline const std::vector<BlockVisualization>& blocks() const { return blocks_; }

    /**
     * @brief previewResolutionFactor
     * factor, by which the block resolutions were scaled in the mesh preview
     */
    inline double previewResolutionFactor() const { return previewResolutionFactor_; }

    /**
     * @brief hasInvalidBlocks
     * true, if blockMesh would reject at least one block as inside-out
     * or as having inward-pointing faces
     */
    bool hasInvalidBlocks() const;

    /**
     * @brief blockProblemsReport
     * description of the invalid blocks (empty, if there are none)
     */
    std::string blockProblemsReport() const;

    /**
     * @brief blockProblemDescription
     * description of the problem of block i (empty, if the block is valid)
     */
    std::string blockProblemDescription(size_t i) const;

    /**
     * @brief problemEdges
     * the edges of the faces of block i, which are not outward-pointing
     * (all edges, if the block is inside-out). Displayed coordinates.
     */
    TopoDS_Shape problemEdges(size_t i) const;

    /**
     * @brief addToVisualizer
     * adds one feature per block. Invalid blocks are shown in red,
     * annotated (outline of the offending faces and caption)
     * and reported in a warning.
     * @param repr
     * Surface: shaded mesh preview with edges.
     * Wireframe: only block edges and mesh lines (e.g. for domains, which enclose other geometry)
     */
    void addToVisualizer(
        CADParameterSetModelVisualizer& vz,
        const std::string& prefix,
        const ScaleOrTransform& s_or_t = 1.0,
        insight::DatasetRepresentation repr = insight::Surface ) const;
};


/**
 * @brief addBlockingToVisualizer
 * shortcut: create the visualization of the blocking and add it to the visualizer
 */
TOOLKIT_GUI_EXPORT void addBlockingToVisualizer(
    CADParameterSetModelVisualizer& vz,
    const bmd::blockMeshBlocking& bm,
    const std::string& prefix,
    const blockMeshVisualization::ScaleOrTransform& s_or_t = 1.0,
    insight::DatasetRepresentation repr = insight::Surface );


/**
 * @brief createNonOrthogonalFacesDataset
 * polydata of all internal faces of the predicted mesh with a non-orthogonality
 * above the threshold, with cell data "nonOrthogonality" [deg] and "block"
 */
TOOLKIT_GUI_EXPORT vtkSmartPointer<vtkPolyData> createNonOrthogonalFacesDataset(
    const bmd::MeshPredictor& mp,
    double thresholdDeg );


/**
 * @brief addBlockingQualityToVisualizer
 * predict the mesh of the blocking (with reduced resolution), evaluate the
 * non-orthogonality and add the faces above the threshold as dataset
 * "<prefix>/quality/nonOrthogonalFaces" to the visualizer.
 * @return statistics of the predicted mesh
 */
TOOLKIT_GUI_EXPORT bmd::NonOrthogonalityStatistics addBlockingQualityToVisualizer(
    CADParameterSetModelVisualizer& vz,
    const bmd::blockMeshBlocking& bm,
    const std::string& prefix,
    int maxCells = 100000,
    double thresholdDeg = 30. );


}

} // namespace insight


#endif // INSIGHT_BLOCKMESHVISUALIZATION_H
