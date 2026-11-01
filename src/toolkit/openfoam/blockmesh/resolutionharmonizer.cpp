#include "resolutionharmonizer.h"

#include "openfoam/blockmesh.h"
#include "openfoam/blockmesh/gradinganalyzer.h"
#include "base/exception.h"

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <numeric>
#include <sstream>
#include <tuple>

#include "boost/format.hpp"

namespace insight {
namespace bmd {


namespace {

// OpenFOAM edge order
const int hexEdgeVerts[12][2] = {
    {0,1}, {3,2}, {7,6}, {4,5},
    {0,3}, {1,2}, {5,6}, {4,7},
    {0,4}, {1,5}, {2,6}, {3,7}
};

// faces at the start (side 0) and end (side 1) of each direction
const int hexDirFaces[3][2][4] = {
    { {0,3,7,4}, {1,2,6,5} },
    { {0,1,5,4}, {3,2,6,7} },
    { {0,1,2,3}, {4,5,6,7} }
};

struct UnionFind
{
    std::vector<int> parent;
    UnionFind(int n) : parent(n) { std::iota(parent.begin(), parent.end(), 0); }
    int find(int i) { while (parent[i]!=i) i = parent[i] = parent[parent[i]]; return i; }
    void unite(int a, int b) { parent[find(a)] = find(b); }
};

}




double ResolutionHarmonizer::Report::maxJump() const
{
    return jumps.empty() ? 1. : jumps.front().ratio;
}




double ResolutionHarmonizer::Report::jumpPercentile(double q) const
{
    if (jumps.empty()) return 1.;
    // jumps are sorted in descending order
    size_t i = size_t( (1.-q)*double(jumps.size()-1) + 0.5 );
    return jumps[std::min(i, jumps.size()-1)].ratio;
}




std::string ResolutionHarmonizer::Report::summary(int nWorst) const
{
    std::ostringstream os;
    os << nGroups << " direction groups, " << nCells << " cells";
    os << str(boost::format(", cell size ratios across block interfaces: max %.3g, 95%% %.3g, median %.3g (%d interface edges)")
              % maxJump() % jumpPercentile(0.95) % jumpPercentile(0.5) % jumps.size());
    for (int i=0; i<std::min(nWorst, int(jumps.size())); ++i)
    {
        const auto& j = jumps[i];
        os << str(boost::format("\n  ratio %.3g at (%g %g %g): block %d dir %d / block %d dir %d")
                  % j.ratio % j.location.x % j.location.y % j.location.z
                  % j.block % j.dir % j.otherBlock % j.otherDir );
    }
    if (!gradingMismatches.empty())
    {
        os << "\n" << gradingMismatches.size() << " edges with different gradings in adjacent blocks:";
        for (const auto& g: gradingMismatches)
            os << str(boost::format("\n  edge %d-%d at (%g %g %g): grading %g in block %d, %g in block %d")
                      % g.v0 % g.v1 % g.location.x % g.location.y % g.location.z
                      % g.grading % g.block % g.otherGrading % g.otherBlock );
    }
    if (!inconsistentGroups.empty())
    {
        os << "\n" << inconsistentGroups.size() << " direction groups with inconsistent cell counts:";
        for (const auto& g: inconsistentGroups)
        {
            os << "\n  group " << g.group << ":";
            for (int n: g.resolutions) os << " " << n;
        }
    }
    return os.str();
}




ResolutionHarmonizer::ResolutionHarmonizer(
    const blockMeshBlocking& bmd,
    const std::vector<PointFunction>& periodicTransforms )
: model_(bmd)
{
    V3 lo(DBL_MAX, DBL_MAX, DBL_MAX), hi(-DBL_MAX, -DBL_MAX, -DBL_MAX);
    for (const auto& v: model_.vertices)
        for (int k=0; k<3; ++k)
        {
            lo(k) = std::min(lo(k), v(k));
            hi(k) = std::max(hi(k), v(k));
        }
    tol_ = 1e-6*std::max(norm(hi-lo), DBL_MIN);

    findGroups();

    for (size_t b=0; b<model_.blocks.size(); ++b)
        for (int d=0; d<3; ++d)
            for (int s=0; s<2; ++s)
            {
                auto key = faceKey(int(b), d, s);
                if (key.size()>=3) faces_[key].push_back({int(b), d, s});
            }

    findPeriodicFaces(periodicTransforms);
}




void ResolutionHarmonizer::findGroups()
{
    const int nb = int(model_.blocks.size());
    UnionFind uf(3*nb);

    std::map<std::pair<int,int>, int> edgeNode;
    for (int b=0; b<nb; ++b)
    {
        for (int e=0; e<12; ++e)
        {
            int a = edgeVertex(b, e, 0), c = edgeVertex(b, e, 1);
            if (a==c) continue; // collapsed edges do not connect directions
            auto key = std::make_pair(std::min(a,c), std::max(a,c));
            auto i = edgeNode.find(key);
            if (i==edgeNode.end()) edgeNode[key] = 3*b+e/4;
            else uf.unite(3*b+e/4, i->second);
        }
    }

    std::map<int,int> groupIds;
    group_.resize(3*nb);
    for (int i=0; i<3*nb; ++i)
    {
        int r = uf.find(i);
        auto g = groupIds.find(r);
        if (g==groupIds.end()) g = groupIds.insert({r, int(groupIds.size())}).first;
        group_[i] = g->second;
    }
    nGroups_ = int(groupIds.size());

    resolution_.assign(nGroups_, 0);
    fixed_.assign(nGroups_, false);
    for (int b=0; b<nb; ++b)
        for (int d=0; d<3; ++d)
            if (resolution_[groupOf(b,d)]==0)
                resolution_[groupOf(b,d)] = model_.blocks[b].n[d];
}




void ResolutionHarmonizer::findPeriodicFaces(const std::vector<PointFunction>& periodicTransforms)
{
    if (periodicTransforms.empty()) return;

    std::vector<FaceKey> boundaryFaces;
    std::set<int> boundaryVertices;
    for (const auto& f: faces_)
    {
        if (f.second.size()==1)
        {
            boundaryFaces.push_back(f.first);
            boundaryVertices.insert(f.first.begin(), f.first.end());
        }
    }

    auto findVertex = [&](const V3& p) -> int
    {
        for (int v: boundaryVertices)
            if (norm(model_.vertices[v]-p) < tol_) return v;
        return -1;
    };

    for (const auto& T: periodicTransforms)
    {
        for (const auto& f: boundaryFaces)
        {
            std::map<int,int> vmap;
            FaceKey img;
            for (int v: f)
            {
                int w = findVertex(T(model_.vertices[v]));
                if (w<0) break;
                vmap[v] = w;
                img.push_back(w);
            }
            if (img.size()!=f.size()) continue;
            std::sort(img.begin(), img.end());
            auto i = faces_.find(img);
            if (i==faces_.end() || i->second.size()!=1 || img==f) continue;

            std::map<int,int> inv;
            for (const auto& vm: vmap) inv[vm.second] = vm.first;
            periodicImages_[f].push_back({img, vmap});
            periodicImages_[img].push_back({f, inv});
        }
    }
}




int ResolutionHarmonizer::edgeVertex(int block, int edge, int side) const
{
    return model_.blocks[block].v[hexEdgeVerts[edge][side]];
}




ResolutionHarmonizer::FaceKey ResolutionHarmonizer::faceKey(int block, int dir, int side) const
{
    std::set<int> vs;
    for (int k=0; k<4; ++k)
        vs.insert(model_.blocks[block].v[hexDirFaces[dir][side][k]]);
    return FaceKey(vs.begin(), vs.end());
}




std::vector<ResolutionHarmonizer::Continuation>
ResolutionHarmonizer::continuations(int block, int edge, int side) const
{
    std::vector<Continuation> res;

    const int dir = edge/4;
    const int w = edgeVertex(block, edge, side);
    auto key = faceKey(block, dir, side);
    if (key.size()<3) return res;

    auto addFrom = [&](const FaceKey& k, int vertex)
    {
        auto i = faces_.find(k);
        if (i==faces_.end()) return;
        for (const auto& fs: i->second)
        {
            if (fs.block==block) continue;
            for (int j=0; j<4; ++j)
            {
                int e2 = 4*fs.dir+j;
                if ( edgeVertex(fs.block, e2, fs.side)==vertex
                     && edgeVertex(fs.block, e2, 0)!=edgeVertex(fs.block, e2, 1) )
                    res.push_back({fs.block, e2, fs.side, vertex});
            }
        }
    };

    addFrom(key, w);

    auto pi = periodicImages_.find(key);
    if (pi!=periodicImages_.end())
        for (const auto& img: pi->second)
            addFrom(img.image, img.vertexMap.at(w));

    return res;
}




double ResolutionHarmonizer::cellSize(int block, int edge, int side, int n) const
{
    EdgeCurve c = model_.curve(edgeVertex(block, edge, 0), edgeVertex(block, edge, 1));
    std::vector<V3> pts;
    std::vector<double> div;
    lineDivide(c, n, model_.blocks[block].grading[edge], pts, div);
    return side==0 ? norm(pts[1]-pts[0]) : norm(pts[n]-pts[n-1]);
}




double ResolutionHarmonizer::edgeLength(int block, int edge) const
{
    EdgeCurve c = model_.curve(edgeVertex(block, edge, 0), edgeVertex(block, edge, 1));
    const int ns = 64;
    double L = 0.;
    V3 p0 = c.position(0.);
    for (int i=1; i<=ns; ++i)
    {
        V3 p1 = c.position(double(i)/double(ns));
        L += norm(p1-p0);
        p0 = p1;
    }
    return L;
}




void ResolutionHarmonizer::fixGroupsOfBlocks(const std::set<int>& blocks)
{
    for (int b: blocks)
        for (int d=0; d<3; ++d)
            fixed_[groupOf(b,d)] = true;
}




std::string ResolutionHarmonizer::harmonize(const std::function<double(const V3&)>& weight)
{
    std::ostringstream log;
    std::vector<bool> determined = fixed_;
    const int nb = int(model_.blocks.size());

    // condition: the cell of edge e of block b at the given side shall have the size delta
    struct Constraint
    {
        int block, edge, side;
        double delta, weight;
        int n0;
        V3 location;
    };
    typedef std::tuple<int,int,int,int,int> ConstraintKey;

    for (int wave=0; ; ++wave)
    {
        std::map<int, std::vector<Constraint> > constraints;
        std::set<ConstraintKey> known;

        for (int b=0; b<nb; ++b)
        {
            for (int d=0; d<3; ++d)
            {
                const int G = groupOf(b, d);
                if (determined[G]) continue;

                for (int j=0; j<4; ++j)
                {
                    const int e = 4*d+j;
                    const int a = edgeVertex(b, e, 0), c1 = edgeVertex(b, e, 1);
                    if (a==c1) continue;

                    const double R = model_.blocks[b].grading[e]; // last/first cell
                    const double L = edgeLength(b, e);

                    for (int side=0; side<2; ++side)
                    {
                        for (const auto& c: continuations(b, e, side))
                        {
                            const int G2 = groupOf(c.block, c.edge/4);
                            if (!determined[G2]) continue;

                            // the same pair of edges may be found from several blocks
                            const int ca = edgeVertex(c.block, c.edge, 0), cb = edgeVertex(c.block, c.edge, 1);
                            ConstraintKey key(
                                std::min(a,c1), std::max(a,c1), edgeVertex(b, e, side),
                                std::min(ca,cb), std::max(ca,cb) );
                            if (!known.insert(key).second) continue;

                            const double delta = cellSize(c.block, c.edge, c.side, resolution_[G2]);
                            if (delta<=0.) continue;

                            // smallest cell of this edge, if the cell at the shared vertex has size delta
                            double first = side==0 ? delta : delta/R;
                            double smallest = R>=1. ? first : first*R;

                            int n0 = 1;
                            try
                            {
                                n0 = std::max(1, GradingAnalyzer(R).calc_n(smallest, L));
                            }
                            catch (const std::exception&)
                            {
                                continue;
                            }

                            Constraint cs;
                            cs.block = b; cs.edge = e; cs.side = side;
                            cs.delta = delta;
                            cs.location = model_.vertices[edgeVertex(b, e, side)];
                            cs.weight = std::max(0., weight(cs.location));
                            cs.n0 = n0;
                            constraints[G].push_back(cs);
                        }
                    }
                }
            }
        }

        if (constraints.empty()) break;

        for (auto& gc: constraints)
        {
            const int G = gc.first;
            auto& cs = gc.second;

            double W = 0.;
            for (const auto& c: cs) W += c.weight;
            if (!(W>0.)) for (auto& c: cs) c.weight = 1.;

            // weighted squared log mismatch of the cell sizes (unimodal in n)
            auto F = [&](int n)
            {
                double f = 0.;
                for (const auto& c: cs)
                    if (c.weight>0.)
                    {
                        double mm = std::log(cellSize(c.block, c.edge, c.side, n)/c.delta);
                        f += c.weight*mm*mm;
                    }
                return f;
            };

            int lo = INT_MAX, hi = 1;
            for (const auto& c: cs)
            {
                lo = std::min(lo, c.n0-1);
                hi = std::max(hi, c.n0+1);
            }
            lo = std::max(1, lo);

            // smallest n in [lo,hi], for which F does not decrease any more
            while (lo<hi)
            {
                int m = lo + (hi-lo)/2;
                if (F(m+1) < F(m)) lo = m+1;
                else hi = m;
            }
            const int n = lo;

            if (resolution_[G]!=n)
            {
                double rmin = DBL_MAX, rmax = 0., wmax = -1.;
                V3 loc;
                for (const auto& c: cs)
                {
                    if (c.weight<=0.) continue;
                    double r = cellSize(c.block, c.edge, c.side, n)/c.delta;
                    rmin = std::min(rmin, r);
                    rmax = std::max(rmax, r);
                    if (c.weight>wmax) { wmax = c.weight; loc = c.location; }
                }
                log << str(boost::format(
                               "group %d (block %d dir %d): %d -> %d cells, %d matching locations"
                               " (most important at (%g %g %g)), cell size ratios %.3g...%.3g\n")
                           % G % cs.front().block % (cs.front().edge/4) % resolution_[G] % n
                           % cs.size() % loc.x % loc.y % loc.z % rmin % rmax );
            }
            resolution_[G] = n;
            determined[G] = true;
        }
    }

    int nUndetermined = int(std::count(determined.begin(), determined.end(), false));
    if (nUndetermined)
        log << nUndetermined << " direction groups are not connected to prescribed groups and keep their cell counts\n";

    return log.str();
}




ResolutionHarmonizer::Report ResolutionHarmonizer::check() const
{
    Report r;
    r.nGroups = nGroups_;
    const int nb = int(model_.blocks.size());

    // cell counts within the groups
    std::vector<std::set<int> > res(nGroups_);
    for (int b=0; b<nb; ++b)
        for (int d=0; d<3; ++d)
            res[groupOf(b,d)].insert(model_.blocks[b].n[d]);
    for (int G=0; G<nGroups_; ++G)
        if (res[G].size()>1)
            r.inconsistentGroups.push_back({G, res[G]});

    for (int b=0; b<nb; ++b)
    {
        long nc=1;
        for (int d=0; d<3; ++d) nc *= resolution_[groupOf(b,d)];
        r.nCells += nc;
    }

    // gradings of shared edges
    std::map<std::pair<int,int>, std::pair<int,double> > edgeGrading;
    for (int b=0; b<nb; ++b)
    {
        for (int e=0; e<12; ++e)
        {
            int a = edgeVertex(b, e, 0), c = edgeVertex(b, e, 1);
            if (a==c) continue;
            double g = model_.blocks[b].grading[e];
            if (a>c) { std::swap(a,c); g = 1./g; }
            auto key = std::make_pair(a,c);
            auto i = edgeGrading.find(key);
            if (i==edgeGrading.end())
                edgeGrading[key] = {b, g};
            else if (std::fabs(std::log(g/i->second.second)) > 1e-6)
                r.gradingMismatches.push_back({
                    a, c, 0.5*(model_.vertices[a]+model_.vertices[c]),
                    i->second.first, b, i->second.second, g });
        }
    }

    // cell size jumps
    typedef std::tuple<int,int,int> EdgeEnd;
    std::set<std::pair<EdgeEnd,EdgeEnd> > done;
    for (int b=0; b<nb; ++b)
    {
        for (int e=0; e<12; ++e)
        {
            if (edgeVertex(b, e, 0)==edgeVertex(b, e, 1)) continue;
            for (int side=0; side<2; ++side)
            {
                for (const auto& c: continuations(b, e, side))
                {
                    EdgeEnd e1(b, e, side), e2(c.block, c.edge, c.side);
                    auto key = std::make_pair(std::min(e1,e2), std::max(e1,e2));
                    if (!done.insert(key).second) continue;

                    double s1 = cellSize(b, e, side, resolution_[groupOf(b, e/4)]);
                    double s2 = cellSize(c.block, c.edge, c.side, resolution_[groupOf(c.block, c.edge/4)]);
                    if (s1<=0. || s2<=0.) continue;
                    r.jumps.push_back({
                        std::max(s1,s2)/std::min(s1,s2),
                        model_.vertices[edgeVertex(b, e, side)],
                        b, e/4, c.block, c.edge/4 });
                }
            }
        }
    }
    std::sort(r.jumps.begin(), r.jumps.end(),
              [](const Report::Jump& a, const Report::Jump& b) { return a.ratio > b.ratio; });

    return r;
}




void ResolutionHarmonizer::apply(blockMeshBlocking& bmd) const
{
    auto& blocks = bmd.allBlocks();
    insight::assertion(
        blocks.size()==model_.blocks.size(),
        "the blocking does not match the analysed blocking" );
    for (size_t b=0; b<blocks.size(); ++b)
        for (int d=0; d<3; ++d)
            blocks[b].setResolution(d, resolution_[groupOf(int(b), d)]);
}




} // namespace bmd
} // namespace insight
