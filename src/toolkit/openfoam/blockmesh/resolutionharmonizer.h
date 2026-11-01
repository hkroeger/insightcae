#ifndef INSIGHT_BMD_RESOLUTIONHARMONIZER_H
#define INSIGHT_BMD_RESOLUTIONHARMONIZER_H

#include "openfoam/blockmesh/blockmeshprediction.h"

#include <functional>
#include <set>
#include <string>
#include <vector>

namespace insight {
namespace bmd {

class blockMeshBlocking;




/**
 * @brief The ResolutionHarmonizer class
 * determines the cell counts of a blocking from prescribed counts in some blocks,
 * the prescribed gradings and the condition, that the cell sizes do not jump across
 * block interfaces. Also checks the consistency of the cell counts and gradings.
 *
 * The block directions are grouped: all directions, which are connected by shared edges,
 * need the same cell count (direction group). The groups of selected blocks are fixed,
 * the cell counts of the other groups are determined in waves, starting from the fixed groups:
 * the edges of a group, which continue (across a block face) into an edge of a determined group,
 * yield conditions for the cell count of the group: the cell size at the shared vertex shall match.
 * Usually, the conditions contradict each other (e.g. different neighbour cell sizes at different interfaces).
 * The selected cell count minimizes the sum of the squared logarithmic cell size mismatches,
 * weighted by a function of the location of the shared vertex.
 */
class ResolutionHarmonizer
{
public:
    typedef std::function<V3(const V3&)> PointFunction;

    struct Report
    {
        struct Jump
        {
            double ratio;       //!< larger/smaller cell size (>=1)
            V3 location;
            int block, dir, otherBlock, otherDir;
        };
        std::vector<Jump> jumps;        //!< sorted, largest first

        struct GradingMismatch
        {
            int v0, v1;                 //!< vertex indices of the edge
            V3 location;                //!< edge mid point
            int block, otherBlock;
            double grading, otherGrading; //!< expansion ratios in direction v0 -> v1
        };
        std::vector<GradingMismatch> gradingMismatches;

        struct InconsistentGroup
        {
            int group;
            std::set<int> resolutions;
        };
        std::vector<InconsistentGroup> inconsistentGroups;

        int nGroups = 0;
        long nCells = 0;

        double maxJump() const;
        double jumpPercentile(double q) const;
        std::string summary(int nWorst = 10) const;
    };

private:
    BlockingModel model_;
    double tol_;

    std::vector<int> group_;            // group of node 3*block+dir
    int nGroups_ = 0;
    std::vector<int> resolution_;       // per group
    std::vector<bool> fixed_;           // per group

    struct FaceSide { int block, dir, side; }; // side: 0 start face, 1 end face of direction
    typedef std::vector<int> FaceKey;
    std::map<FaceKey, std::vector<FaceSide> > faces_;

    // periodic face pairs: face => (image face, vertex map)
    struct PeriodicImage { FaceKey image; std::map<int,int> vertexMap; };
    std::map<FaceKey, std::vector<PeriodicImage> > periodicImages_;

    struct Continuation { int block, edge, side, vertex; };

    FaceKey faceKey(int block, int dir, int side) const;
    std::vector<Continuation> continuations(int block, int edge, int side) const;
    double cellSize(int block, int edge, int side, int n) const;
    double edgeLength(int block, int edge) const;
    int edgeVertex(int block, int edge, int side) const;

    void findGroups();
    void findPeriodicFaces(const std::vector<PointFunction>& periodicTransforms);

public:
    ResolutionHarmonizer(
        const blockMeshBlocking& bmd,
        const std::vector<PointFunction>& periodicTransforms = {} );

    inline const BlockingModel& model() const { return model_; }
    inline int nGroups() const { return nGroups_; }
    inline int groupOf(int block, int dir) const { return group_[3*block+dir]; }
    inline int resolution(int group) const { return resolution_[group]; }
    inline bool isFixed(int group) const { return fixed_[group]; }

    /**
     * prescribe the cell counts of the groups of all directions of the given blocks
     */
    void fixGroupsOfBlocks(const std::set<int>& blocks);

    /**
     * determine the cell counts of all non-fixed groups.
     * @param weight
     * importance of matching the cell sizes at a location (>=0, higher is more important).
     * If all weights of a group are zero, all its locations are weighted equally.
     * @return description of the changes
     */
    std::string harmonize(const std::function<double(const V3&)>& weight);

    /**
     * check the current cell counts and gradings
     */
    Report check() const;

    /**
     * write the cell counts into the blocking (same blocking as given in the constructor)
     */
    void apply(blockMeshBlocking& bmd) const;
};




} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_RESOLUTIONHARMONIZER_H
