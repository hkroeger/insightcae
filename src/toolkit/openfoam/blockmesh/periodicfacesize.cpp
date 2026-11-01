#include "periodicfacesize.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iterator>
#include <limits>

#include "base/exception.h"

#include "boost/format.hpp"
#include "boost/geometry.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/index/rtree.hpp"

namespace insight {
namespace bmd {


namespace {

// relative tolerance of the comparison with the thresholds
const double relTol = 1e-6;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<double, 3, bg::cs::cartesian> RPoint;
typedef std::pair<RPoint, size_t> RValue;
typedef bgi::rtree<RValue, bgi::quadratic<16> > RTree;

struct FaceSet
{
    std::vector<V3> centres;
    std::vector<double> areas;
};

}




std::string PeriodicFaceSizeCheck::Result::summary(double limit) const
{
    std::string s = str(boost::format(
        "min. area ratio of opposite periodic faces %.3g, %d of %d faces below %g")
               % minAreaRatio % nBelowLimit % nFaces % limit );
    if (withReference)
        s += str(boost::format(" (%d faces smaller than in the reference state or the limit, by max. %.2g%%)")
                 % nViolating % (100.*maxShortfall) );
    return s;
}




PeriodicFaceSizeCheck::PeriodicFaceSizeCheck(
    const MeshPredictor& mp,
    const std::string& labelA, const std::string& labelB,
    const PointFunction& T )
: T_(T)
{
    const auto& m = mp.model;
    for (size_t bi=0; bi<m.blocks.size(); ++bi)
    {
        const auto& b = m.blocks[bi];
        for (int f=0; f<6; ++f)
        {
            if (mp.faceInterface(int(bi), f)!=-1) continue; // no boundary face

            std::array<int,4> key;
            for (int k=0; k<4; ++k)
                key[k] = b.v[MeshPredictor::hexFaces[f][k]];
            std::sort(key.begin(), key.end());
            auto l = m.projectedFaceLabels.find(key);
            if (l==m.projectedFaceLabels.end()) continue;

            int side = l->second==labelA ? 0 : (l->second==labelB ? 1 : -1);
            if (side>=0)
            {
                faces_[side].push_back({int(bi), f});
                blocks_.insert(int(bi));
            }
        }
    }
}




bool PeriodicFaceSizeCheck::active() const
{
    return !faces_[0].empty() && !faces_[1].empty();
}




bool PeriodicFaceSizeCheck::involves(const std::set<int>& blocks) const
{
    for (int bi: blocks)
        if (blocks_.count(bi)) return true;
    return false;
}




std::vector<double> PeriodicFaceSizeCheck::areaRatios(const MeshPredictor& mp) const
{
    std::vector<double> ratios;
    if (!active()) return ratios;

    // cell faces of both sides, side A transformed onto side B
    FaceSet fs[2];
    for (int side=0; side<2; ++side)
    {
        for (const auto& bf: faces_[side])
        {
            const auto& g = mp.grids()[bf.first];
            const int f = bf.second, fam = f/2;
            const bool hi = (f%2)==1;
            const int n[3] = { g.ni, g.nj, g.nk };

            // index ranges of the faces of family fam on block face f
            int lo[3] = { 0, 0, 0 }, up[3] = { n[0], n[1], n[2] };
            lo[fam] = hi ? n[fam] : 0;
            up[fam] = lo[fam]+1;

            for (int k=lo[2]; k<up[2]; ++k)
                for (int j=lo[1]; j<up[1]; ++j)
                    for (int i=lo[0]; i<up[0]; ++i)
                    {
                        int fl = g.faceLabel(fam, i, j, k);
                        const V3& c = g.faceCentres[fam][fl];
                        fs[side].centres.push_back(side==0 ? T_(c) : c);
                        fs[side].areas.push_back(norm(g.faceAreas[fam][fl]));
                    }
        }
    }

    RTree trees[2];
    for (int side=0; side<2; ++side)
    {
        std::vector<RValue> vals;
        vals.reserve(fs[side].centres.size());
        for (size_t i=0; i<fs[side].centres.size(); ++i)
        {
            const V3& c = fs[side].centres[i];
            vals.push_back({ RPoint(c.x, c.y, c.z), i });
        }
        trees[side] = RTree(vals.begin(), vals.end()); // packing construction
    }

    ratios.reserve(fs[0].areas.size()+fs[1].areas.size());
    for (int side=0; side<2; ++side)
    {
        const auto& own = fs[side];
        const auto& other = fs[1-side];
        for (size_t i=0; i<own.centres.size(); ++i)
        {
            const V3& c = own.centres[i];
            std::vector<RValue> nearest;
            trees[1-side].query(
                bgi::nearest(RPoint(c.x, c.y, c.z), 1), std::back_inserter(nearest) );

            double Aopp = nearest.empty() ? 0. : other.areas[nearest.front().second];
            ratios.push_back(
                Aopp>0. ? own.areas[i]/Aopp
                        : std::numeric_limits<double>::quiet_NaN() ); // degenerate opposite face
        }
    }

    return ratios;
}




PeriodicFaceSizeCheck::Result PeriodicFaceSizeCheck::evaluate(
    const MeshPredictor& mp, double limit,
    const std::vector<double>* reference ) const
{
    Result res;
    res.withReference = (reference!=nullptr);

    auto ratios = areaRatios(mp);
    insight::assertion(
        !reference || reference->size()==ratios.size(),
        "reference ratios do not match the faces of the periodic boundaries" );

    res.minAreaRatio = DBL_MAX;
    for (size_t i=0; i<ratios.size(); ++i)
    {
        const double r = ratios[i];
        if (std::isnan(r)) continue;

        res.minAreaRatio = std::min(res.minAreaRatio, r);
        res.nFaces++;
        if (limit>0.)
        {
            if (r<limit) res.nBelowLimit++;

            double t = limit;
            if (reference && !std::isnan((*reference)[i]))
                t = std::min(t, (*reference)[i]);
            if (r<t*(1.-relTol)) // ignore round-off
            {
                res.nViolating++;
                res.violation += 1.-r/t;
                res.maxShortfall = std::max(res.maxShortfall, 1.-r/t);
            }
        }
    }
    if (res.nFaces==0) res.minAreaRatio = 1.;

    return res;
}




} // namespace bmd
} // namespace insight
