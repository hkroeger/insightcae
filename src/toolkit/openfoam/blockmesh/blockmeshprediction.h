#ifndef INSIGHT_BMD_BLOCKMESHPREDICTION_H
#define INSIGHT_BMD_BLOCKMESHPREDICTION_H

#include "openfoam/blockmesh/point.h"

#include <array>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <functional>
#include <cmath>

namespace insight {
namespace bmd {

class blockMeshBlocking;

/**
 * @brief The V3 struct
 * lightweight 3-vector for the inner loops of the mesh prediction
 */
struct V3
{
    double x=0., y=0., z=0.;

    V3() = default;
    V3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    explicit V3(const arma::mat& m) : x(m(0)), y(m(1)), z(m(2)) {}

    inline double operator()(int i) const { return i==0 ? x : (i==1 ? y : z); }
    inline double& operator()(int i) { return i==0 ? x : (i==1 ? y : z); }

    inline arma::mat toArma() const { arma::mat m(3,1); m(0)=x; m(1)=y; m(2)=z; return m; }

    inline V3& operator+=(const V3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    inline V3& operator-=(const V3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }
    inline V3& operator*=(double s) { x*=s; y*=s; z*=s; return *this; }
    inline V3& operator/=(double s) { x/=s; y/=s; z/=s; return *this; }
};

inline V3 operator+(V3 a, const V3& b) { return a+=b; }
inline V3 operator-(V3 a, const V3& b) { return a-=b; }
inline V3 operator-(const V3& a) { return V3(-a.x, -a.y, -a.z); }
inline V3 operator*(V3 a, double s) { return a*=s; }
inline V3 operator*(double s, V3 a) { return a*=s; }
inline V3 operator/(V3 a, double s) { return a/=s; }

inline double dot(const V3& a, const V3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline V3 cross(const V3& a, const V3& b)
{
    return V3( a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x );
}
inline double magSqr(const V3& a) { return dot(a,a); }
inline double norm(const V3& a) { return std::sqrt(magSqr(a)); }


/**
 * projection of a point onto a geometry surface (nearest point on the surface)
 */
typedef std::function<V3(const V3&)> SurfaceProjection;




/**
 * @brief The EdgeCurve class
 * shape of a block edge, evaluated in the same way as OpenFOAM-v2112 blockMesh does
 * (spline: Catmull-Rom with chord length parametrization, arc: circular arc through three points)
 */
class EdgeCurve
{
public:
    enum Type { Line, CatmullRom, Arc };

private:
    Type type_;
    std::vector<V3> pts_;           // Line: 2 points, CatmullRom: all points incl. ends
    std::vector<double> param_;     // CatmullRom: normalized cumulative chord length

    V3 centre_, e1_, e2_;           // Arc
    double radius_, angle_;

    V3 catmullRomPosition(int segment, double mu) const;

public:
    static EdgeCurve line(const V3& p0, const V3& p1);
    static EdgeCurve spline(const std::vector<V3>& allPoints);
    static EdgeCurve arc(const V3& p0, const V3& pm, const V3& p1);

    V3 position(double mu) const;
};


/**
 * @brief lineDivide
 * division of an edge with the given expansion ratio (last cell/first cell),
 * as done in OpenFOAM-v2112 blockMesh (grading applied to the curve parameter)
 */
void lineDivide(
    const EdgeCurve& c, int nDiv, double expRatio,
    std::vector<V3>& points, std::vector<double>& divisions );




/**
 * @brief The BlockingModel struct
 * indexed representation of the vertices, blocks and edges of a blockMeshBlocking.
 * Vertices are merged by coordinates as blockMesh does with the written labels.
 */
struct BlockingModel
{
    std::vector<V3> vertices;
    std::vector<arma::mat> originalVertices; // coordinates as in the blocking

    struct BlockData
    {
        std::array<int,8> v;    // vertex indices, in the order written into the blockMeshDict
        std::array<int,3> n;    // resolution
        std::array<double,12> grading; // expansion ratios in OpenFOAM edge order
    };
    std::vector<BlockData> blocks;

    struct EdgeData
    {
        enum Kind {
            Spline,     // spline, intermediate points
            Arc,        // arc, intermediate[0] is the mid point
            Fixed       // projected or unsupported edge, evaluated as straight line
        };
        int v0, v1;
        Kind kind;
        std::vector<V3> intermediate;
        std::string splineKeyword, geometryLabel;
        bool generated = false; // created by smoother from a straight (implicit) block edge
        int originalIndex = -1; // index in blockMeshBlocking::allEdges()
    };
    std::vector<EdgeData> edges;
    std::map<std::pair<int,int>, int> edgeIndex; // (min vertex, max vertex) => edge

    std::set<int> projectedVertices;
    std::map<int, std::string> projectedVertexLabels;              // vertex => geometry label
    std::map<std::array<int,4>, std::string> projectedFaceLabels;  // sorted corner vertices => geometry label
    std::map<std::string, SurfaceProjection> surfaceProjections;   // geometry label => projection (if known)

    BlockingModel(const blockMeshBlocking& bmd);

    int findEdge(int a, int b) const;

    /**
     * all points of edge e (incl. end points) in direction v0 -> v1
     */
    std::vector<V3> edgePoints(int e) const;

    /**
     * curve between vertices a and b in direction a -> b
     */
    EdgeCurve curve(int a, int b) const;
};




struct NonOrthogonalityStatistics
{
    double maxDeg = 0.;
    double avgDeg = 0.;     //!< as reported by checkMesh: acos(mean of cosines)
    int nFaces = 0;
    int nAbove70 = 0;
    int nInvalidCells = 0;  //!< cells with negative volume or incorrectly oriented face pyramids
    int nCells = 0;

    std::string summary() const;
};




/**
 * @brief The MeshPredictor class
 * predicts the cells, which blockMesh creates from a blocking (with reduced resolution),
 * and computes the face non-orthogonality as checkMesh does.
 *
 * Limitations: projections of faces are only considered, if a projection function for the geometry is known
 * (blockMeshBlocking::setSurfaceProjection, not for collapsed blocks), projected edges are
 * evaluated through their support points (projectSpline) or as straight lines.
 */
class MeshPredictor
{
public:
    // OpenFOAM hex face order
    static const int hexFaces[6][4];

    struct BlockGrid
    {
        int ni=0, nj=0, nk=0;
        std::vector<V3> points;
        std::vector<V3> cellCentres;
        std::vector<double> cellVolumes;

        // face centres and area vectors of all faces (incl. block boundary faces),
        // area vectors oriented along the positive index direction
        // family 0: faces normal to i, 1: normal to j, 2: normal to k
        std::vector<V3> faceCentres[3], faceAreas[3];

        // cached quality of the internal faces (for the current objective exponent)
        double sumThetaP = 0.;
        double maxTheta = 0.;
        int nInvalid = 0;

        // scratch buffers
        std::vector<V3> edgePoints[12];
        std::vector<double> edgeWeights[12];

        inline int pointLabel(int i, int j, int k) const { return i + (ni+1)*(j + (nj+1)*k); }
        inline int cellLabel(int i, int j, int k) const { return i + ni*(j + nj*k); }

        inline int faceLabel(int family, int i, int j, int k) const
        {
            switch (family)
            {
            case 0: return i + (ni+1)*(j + nj*k);
            case 1: return i + ni*(j + (nj+1)*k);
            default: return i + ni*(j + nj*k);
            }
        }
    };

    struct Interface
    {
        int blockA, faceA, blockB, faceB;
        double signA;   // orientation factor for area vectors from facePoints to point from A to B
        int familyA;    // face family of the interface faces in block A
        double signOutA;// orientation factor of the cached face areas of A to point from A to B
        std::vector<int> faceLabelsA; // labels of the interface faces in A's face family arrays
        // pairs of cells (in A and B) adjacent to the face cells, and the face points (labels in A's grid)
        std::vector<std::pair<int,int> > cellPairs;
        std::vector<std::array<int,4> > facePoints;
    };

    BlockingModel model;

private:
    double resolutionFactor_;
    double objectiveExponent_ = 8.;
    std::vector<BlockGrid> grids_;
    std::vector<Interface> interfaces_;
    std::vector<std::array<int,6> > faceInterface_; // >=0: interface, -1: boundary, -2: degenerate/unmatched
    std::vector<std::vector<int> > blockInterfaces_;
    std::vector<std::array<const SurfaceProjection*,6> > faceProjection_; // projection of block faces (nullptr: none)

    /**
     * correction of the block points for projected faces, like OpenFOAM block::createPoints
     */
    void applyFaceProjections(int bi);

    void init(int maxCells);
    void setupInterfaces();

    double thetaPow(double theta) const;

    /**
     * sum of theta^p over the internal faces, max. theta, invalid cells of a block
     * (requires up-to-date geometry)
     */
    void computeBlockQuality(int bi);

public:
    MeshPredictor(const blockMeshBlocking& bmd, int maxCells=100000);
    MeshPredictor(const BlockingModel& model, int maxCells=100000);

    inline double resolutionFactor() const { return resolutionFactor_; }
    inline const std::vector<BlockGrid>& grids() const { return grids_; }

    /**
     * face type: >=0 interface index, -1 boundary face, -2 degenerate or unmatched face
     */
    inline int faceInterface(int block, int face) const { return faceInterface_[block][face]; }

    /**
     * recompute points, cell centres and volumes of a block
     * (after modification of model vertices or edges)
     */
    void updateBlock(int bi);
    void updateAll();

    /**
     * visit all faces (internal faces of the given blocks and the interfaces of these blocks).
     * If blocks is empty, all faces are visited. Each face is visited once.
     */
    void forEachFace(
        const std::set<int>& blocks,
        const std::function<void(const std::array<V3,4>& facePoints, double thetaDeg, int block)>& f
        ) const;

    /**
     * p-norm of the non-orthogonality angles (in degrees) of the faces visited by forEachFace
     */
    double objective(const std::set<int>& blocks, double p, int& nInvalidCells);

    /**
     * set the exponent of the objective. The per-block sums are cached for this exponent.
     */
    void setObjectiveExponent(double p);

    NonOrthogonalityStatistics statistics() const;
};




/**
 * geometric face centre and area vector, computed like OpenFOAM's primitiveMesh
 */
void faceCentreAndArea(const std::array<const V3*,4>& p, V3& fc, V3& S);

/**
 * face non-orthogonality angle in degrees, like checkMesh
 */
double nonOrthogonalityDeg(const V3& d, const V3& S);


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_BLOCKMESHPREDICTION_H
