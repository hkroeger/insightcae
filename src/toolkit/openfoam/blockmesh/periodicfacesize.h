#ifndef INSIGHT_BMD_PERIODICFACESIZE_H
#define INSIGHT_BMD_PERIODICFACESIZE_H

#include "openfoam/blockmesh/blockmeshprediction.h"

#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace insight {
namespace bmd {




/**
 * @brief The PeriodicFaceSizeCheck class
 * compares the sizes of the cell faces on two periodic boundaries, which are not
 * necessarily conformal (e.g. cyclicAMI or stitched copies of a sector mesh).
 *
 * The opposite face of a cell face on one side is the face on the other side,
 * whose centre is nearest to its (transformed) centre.
 * If a face is much smaller than its opposite face (slivers),
 * stitching rotated copies of the mesh (stitchMesh) fails.
 */
class PeriodicFaceSizeCheck
{
public:
    typedef std::function<V3(const V3&)> PointFunction;

    struct Result
    {
        double minAreaRatio = 1.;   //!< min. of |S_f|/|S_opposite| over the faces of both sides
        double violation = 0.;      //!< sum over the faces of max(0, 1-ratio/threshold)
        int nViolating = 0;         //!< number of faces with ratio < threshold
        double maxShortfall = 0.;   //!< max. of 1-ratio/threshold
        int nBelowLimit = 0;        //!< number of faces with ratio < limit
        int nFaces = 0;
        bool withReference = false; //!< thresholds from reference ratios

        std::string summary(double limit) const;
    };

private:
    std::vector<std::pair<int,int> > faces_[2]; // (block, hex face) of side A and B
    std::set<int> blocks_;
    PointFunction T_;

public:
    /**
     * @param labelA, labelB
     * geometry labels of the projected block faces on both sides (BlockingModel::projectedFaceLabels)
     * @param T
     * transformation from side A onto side B
     */
    PeriodicFaceSizeCheck(
        const MeshPredictor& mp,
        const std::string& labelA, const std::string& labelB,
        const PointFunction& T );

    /**
     * both sides have faces
     */
    bool active() const;

    /**
     * any of the blocks has a face on one of the periodic boundaries
     */
    bool involves(const std::set<int>& blocks) const;

    /**
     * area ratios |S_f|/|S_opposite| of all faces of both sides (NaN: degenerate opposite face)
     * of the current (up-to-date) predicted grid of mp,
     * which has to be built from the same blocking (and resolution) as the one given to the constructor.
     * The faces are always in the same order.
     */
    std::vector<double> areaRatios(const MeshPredictor& mp) const;

    /**
     * compare the area ratios with a threshold per face: the limit or, if reference ratios
     * (areaRatios of a reference state) are given, the smaller of the limit and the reference ratio.
     * With a reference, faces which are already below the limit in the reference state, may not get smaller.
     */
    Result evaluate(
        const MeshPredictor& mp, double limit,
        const std::vector<double>* reference = nullptr ) const;
};




} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_PERIODICFACESIZE_H
