#include "blockmeshsmoother.h"

#include "base/linearalgebra.h"
#include "base/exception.h"
#include "base/actionprogress.h"

#include <chrono>

#include <iostream>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <memory>
#include <tuple>

#include "boost/format.hpp"

namespace insight {
namespace bmd {


namespace {

const int hexEdgeVerts[12][2] = {
    {0,1}, {3,2}, {7,6}, {4,5},
    {0,3}, {1,2}, {5,6}, {4,7},
    {0,4}, {1,5}, {2,6}, {3,7}
};

inline std::pair<int,int> edgeKey(int a, int b)
{
    return std::make_pair(std::min(a,b), std::max(a,b));
}

// normalized chord length parameter of the points of a polyline
std::vector<double> chordParameter(const std::vector<V3>& pts)
{
    std::vector<double> t(pts.size(), 0.);
    for (size_t i=1; i<pts.size(); ++i)
        t[i] = t[i-1] + norm(pts[i]-pts[i-1]);
    if (t.back()>0.)
        for (auto& ti: t) ti/=t.back();
    return t;
}

/**
 * check, that the points of a spline proceed monotonically along its chord
 * with a minimum spacing (fraction of the uniform spacing).
 * Otherwise the Catmull-Rom spline may backtrack and fold the cells near it,
 * which the reduced-resolution prediction may not resolve.
 */
bool monotonicAlongChord(const std::vector<V3>& pts, double minGapRel=0.25)
{
    V3 c = pts.back() - pts.front();
    double L2 = magSqr(c);
    if (L2<=0.) return true;
    double minGap = minGapRel/double(pts.size()-1);
    double sPrev = 0.;
    for (size_t i=1; i<pts.size(); ++i)
    {
        double s = dot(pts[i]-pts.front(), c)/L2;
        if (s-sPrev < minGap) return false;
        sPrev = s;
    }
    return true;
}

const double inadmissible = 1e30;

// block edges may not shrink below this fraction of their original length
// (prevents collapsing edges, which removes the bad faces from the objective)
const double minEdgeLengthRatio = 0.25;

}




BlockMeshSmoother::BlockMeshSmoother(MeshPredictor& mp, const SmoothingParameters& par)
: mp_(mp), par_(par)
{}




V3 BlockMeshSmoother::Constraint::project(const V3& p, const BlockingModel& m) const
{
    if (label.empty())
        return p - (dot(n, p)-d)*n;
    else
        return m.surfaceProjections.at(label)(p);
}




V3 BlockMeshSmoother::projectOnto(const ConstraintSet& cs, const V3& p0) const
{
    if (cs.empty()) return p0;

    // alternating projections: converges to the intersection of two transversal surfaces
    V3 p = p0;
    for (int it=0; it<100; ++it)
    {
        V3 pPrev = p;
        for (int c: cs)
            p = constraints_[c].project(p, mp_.model);
        if (cs.size()==1 || norm(p-pPrev) < 1e-3*tol_) break;
    }
    return p;
}




BlockMeshSmoother::ConstraintSet BlockMeshSmoother::constraintsOfEdge(int va, int vb) const
{
    auto i = edgeConstraints_.find(edgeKey(va, vb));
    if (i==edgeConstraints_.end()) return ConstraintSet();
    return i->second;
}




void BlockMeshSmoother::classify()
{
    auto& m = mp_.model;
    const size_t nv = m.vertices.size();

    fixedVertex_.assign(nv, false);
    vertexConstraints_.assign(nv, {});
    blocksOfVertex_.assign(nv, {});
    boundaryEdges_.clear();
    edgeConstraints_.clear();
    fixedEdges_.clear();
    constraints_.clear();

    V3 lo(DBL_MAX, DBL_MAX, DBL_MAX), hi(-DBL_MAX, -DBL_MAX, -DBL_MAX);
    for (const auto& v: m.vertices)
    {
        for (int k=0; k<3; ++k)
        {
            lo(k)=std::min(lo(k), v(k));
            hi(k)=std::max(hi(k), v(k));
        }
    }
    tol_ = 1e-6*norm(hi-lo);

    auto addConstraint = [&](Constraint c) -> int
    {
        for (size_t i=0; i<constraints_.size(); ++i)
        {
            const auto& o = constraints_[i];
            if (!c.label.empty())
            {
                if (o.label==c.label) return int(i);
            }
            else if (o.label.empty())
            {
                double s = dot(o.n, c.n);
                if ( std::fabs(s) > 1.-1e-9 && std::fabs(o.d - (s>0. ? c.d : -c.d)) < tol_ )
                    return int(i);
            }
        }
        constraints_.push_back(c);
        return int(constraints_.size()-1);
    };

    // surface, along which the vertices and edges of a boundary face may slide (-1: none)
    auto faceConstraint = [&](int bi, int f) -> int
    {
        if (!par_.slideOnBoundaries) return -1;

        const auto& b = m.blocks[bi];
        std::array<int,4> c;
        for (int k=0; k<4; ++k)
            c[k] = b.v[MeshPredictor::hexFaces[f][k]];

        auto key=c;
        std::sort(key.begin(), key.end());
        auto pl = m.projectedFaceLabels.find(key);
        if (pl!=m.projectedFaceLabels.end())
        {
            if (!m.surfaceProjections.count(pl->second)) return -1;
            Constraint cs;
            cs.label = pl->second;
            return addConstraint(cs);
        }

        // unprojected: planar faces only (incl. their edges)
        V3 n(0,0,0), ctr(0,0,0);
        for (int k=0; k<4; ++k)
        {
            n += cross(m.vertices[c[k]], m.vertices[c[(k+1)%4]]);
            ctr += 0.25*m.vertices[c[k]];
        }
        double A = norm(n);
        if (A < tol_*tol_) return -1;
        n /= A;
        double d = dot(n, ctr);

        for (int k=0; k<4; ++k)
        {
            int va=c[k], vb=c[(k+1)%4];
            if (std::fabs(dot(n, m.vertices[va])-d) > tol_) return -1;
            if (va==vb) continue;
            int e = m.findEdge(va, vb);
            if (e>=0)
            {
                if (m.edges[e].kind==BlockingModel::EdgeData::Fixed) return -1;
                for (const auto& p: m.edgePoints(e))
                    if (std::fabs(dot(n, p)-d) > tol_) return -1;
            }
        }

        Constraint cs;
        cs.n=n;
        cs.d=d;
        return addConstraint(cs);
    };

    for (size_t bi=0; bi<m.blocks.size(); ++bi)
    {
        const auto& b = m.blocks[bi];
        for (int c=0; c<8; ++c)
            blocksOfVertex_[b.v[c]].insert(int(bi));

        for (int f=0; f<6; ++f)
        {
            int fi = mp_.faceInterface(int(bi), f);
            if (fi >= 0) continue;

            // boundary faces slide along their surface, if it is known.
            // Degenerate or unmatched faces, other boundary faces: keep fixed
            int fc = (fi==-1) ? faceConstraint(int(bi), f) : -1;

            for (int k=0; k<4; ++k)
            {
                int va = b.v[MeshPredictor::hexFaces[f][k]];
                int vb = b.v[MeshPredictor::hexFaces[f][(k+1)%4]];

                if (fc<0) fixedVertex_[va] = true;
                else vertexConstraints_[va].insert(fc);

                if (va!=vb)
                {
                    auto ek = edgeKey(va, vb);
                    boundaryEdges_.insert(ek);
                    if (fc<0) fixedEdges_.insert(ek);
                    else edgeConstraints_[ek].insert(fc);
                }
            }
        }
    }

    for (int v: m.projectedVertices)
    {
        auto l = m.projectedVertexLabels.find(v);
        if ( par_.slideOnBoundaries
             && l!=m.projectedVertexLabels.end()
             && m.surfaceProjections.count(l->second) )
        {
            Constraint cs;
            cs.label = l->second;
            vertexConstraints_[v].insert(addConstraint(cs));
        }
        else
            fixedVertex_[v] = true;
    }

    for (const auto& e: m.edges)
    {
        auto ek = edgeKey(e.v0, e.v1);
        if (e.kind==BlockingModel::EdgeData::Fixed)
        {
            fixedVertex_[e.v0] = fixedVertex_[e.v1] = true;
            fixedEdges_.insert(ek);
        }
        else if (!e.geometryLabel.empty())
        {
            if (par_.slideOnBoundaries && m.surfaceProjections.count(e.geometryLabel))
            {
                Constraint cs;
                cs.label = e.geometryLabel;
                int ci = addConstraint(cs);
                vertexConstraints_[e.v0].insert(ci);
                vertexConstraints_[e.v1].insert(ci);
                edgeConstraints_[ek].insert(ci);
            }
            else
            {
                fixedVertex_[e.v0] = fixedVertex_[e.v1] = true;
                fixedEdges_.insert(ek);
            }
        }
    }

    // only sliding along one surface or along the intersection of two surfaces
    for (size_t v=0; v<nv; ++v)
        if (vertexConstraints_[v].size()>2)
            fixedVertex_[v] = true;
}




void BlockMeshSmoother::convertEdges()
{
    auto& m = mp_.model;

    int nConvLines=0, nConvArcs=0;
    for (const auto& b: m.blocks)
    {
        for (int e=0; e<12; ++e)
        {
            int va = b.v[hexEdgeVerts[e][0]], vb = b.v[hexEdgeVerts[e][1]];
            if (va==vb) continue;
            bool boundary = boundaryEdges_.count(edgeKey(va, vb));

            int ei = m.findEdge(va, vb);
            if (ei<0)
            {
                if (boundary) continue;

                // straight edge => low-order spline
                BlockingModel::EdgeData ed;
                ed.v0=va; ed.v1=vb;
                ed.kind=BlockingModel::EdgeData::Spline;
                ed.generated=true;
                int n=par_.nSupportPointsConverted;
                for (int i=1; i<=n; ++i)
                {
                    double t=double(i)/double(n+1);
                    ed.intermediate.push_back( m.vertices[va] + t*(m.vertices[vb]-m.vertices[va]) );
                }
                m.edgeIndex[edgeKey(va,vb)] = int(m.edges.size());
                m.edges.push_back(ed);
                nConvLines++;
            }
            else if (
                m.edges[ei].kind==BlockingModel::EdgeData::Arc
                && ( !boundary || !fixedVertex_[va] || !fixedVertex_[vb] ) )
            {
                // arc => spline through sampled arc points
                auto& ed = m.edges[ei];
                EdgeCurve arc = m.curve(ed.v0, ed.v1);
                const int n=7;
                ed.intermediate.clear();
                for (int i=1; i<=n; ++i)
                    ed.intermediate.push_back( arc.position(double(i)/double(n+1)) );
                ed.kind=BlockingModel::EdgeData::Spline;
                nConvArcs++;
            }
        }
    }

    edgesOfVertex_.assign(m.vertices.size(), {});
    for (size_t ei=0; ei<m.edges.size(); ++ei)
    {
        edgesOfVertex_[m.edges[ei].v0].insert(int(ei));
        edgesOfVertex_[m.edges[ei].v1].insert(int(ei));
    }

    std::cout<<"block smoother: converted "<<nConvLines<<" straight edges and "
             <<nConvArcs<<" arcs into splines"<<std::endl;
}




void BlockMeshSmoother::findPeriodicPairs()
{
    auto& m = mp_.model;
    const size_t nv = m.vertices.size(), ne = m.edges.size();

    slaveVertex_.assign(nv, -1);
    masterVertex_.assign(nv, -1);
    transformOfVertex_.assign(nv, -1);
    slaveEdge_.assign(ne, -1);
    masterEdge_.assign(ne, -1);
    slaveReversed_.assign(ne, false);

    if (!par_.slideOnBoundaries || par_.periodicTransforms.empty()) return;

    std::set<int> bv;
    for (const auto& ek: boundaryEdges_)
    {
        bv.insert(ek.first);
        bv.insert(ek.second);
    }

    int nPairs=0;
    for (size_t ti=0; ti<par_.periodicTransforms.size(); ++ti)
    {
        const auto& T = par_.periodicTransforms[ti];
        for (int v: bv)
        {
            if (slaveVertex_[v]>=0 || masterVertex_[v]>=0) continue;
            V3 tv = T(m.vertices[v]);
            for (int w: bv)
            {
                if ( w!=v && slaveVertex_[w]<0 && masterVertex_[w]<0
                     && norm(m.vertices[w]-tv) < 10.*tol_ )
                {
                    slaveVertex_[v]=w;
                    masterVertex_[w]=v;
                    transformOfVertex_[v]=int(ti);
                    nPairs++;
                    break;
                }
            }
        }
    }

    auto fixPair = [&](int v)
    {
        fixedVertex_[v]=true;
        if (slaveVertex_[v]>=0) fixedVertex_[slaveVertex_[v]]=true;
        if (masterVertex_[v]>=0) fixedVertex_[masterVertex_[v]]=true;
    };

    for (size_t v=0; v<nv; ++v)
    {
        int s=slaveVertex_[v];
        if (s>=0 && (fixedVertex_[v] || fixedVertex_[s]))
        {
            fixPair(int(v));
        }
    }

    // pair the edges between paired vertices
    int nEdgePairs=0, nMismatch=0;
    for (size_t e=0; e<ne; ++e)
    {
        const auto& ed = m.edges[e];
        int sa=slaveVertex_[ed.v0], sb=slaveVertex_[ed.v1];
        if ( sa<0 || sb<0 || transformOfVertex_[ed.v0]!=transformOfVertex_[ed.v1] ) continue;

        const auto& T = par_.periodicTransforms[transformOfVertex_[ed.v0]];
        int e2 = m.findEdge(sa, sb);
        bool match = false;
        if (e2>=0 && m.edges[e2].kind==ed.kind)
        {
            auto P = m.edgePoints(int(e)), Q = m.edgePoints(e2);
            bool rev = (m.edges[e2].v0!=sa);
            if (rev) std::reverse(Q.begin(), Q.end());
            if (P.size()==Q.size())
            {
                match=true;
                for (size_t i=0; i<P.size(); ++i)
                    if (norm(T(P[i])-Q[i]) > 10.*tol_) match=false;
            }
            if (match)
            {
                slaveEdge_[e]=e2;
                masterEdge_[e2]=int(e);
                slaveReversed_[e2]=rev;
                nEdgePairs++;
            }
        }
        if (!match)
        {
            fixPair(ed.v0);
            fixPair(ed.v1);
            nMismatch++;
        }
    }
    // explicit edge on the slave side only
    for (size_t e=0; e<ne; ++e)
    {
        const auto& ed = m.edges[e];
        int ma=masterVertex_[ed.v0], mb=masterVertex_[ed.v1];
        if (ma>=0 && mb>=0 && masterEdge_[e]<0 && m.findEdge(ma, mb)<0)
        {
            fixPair(ed.v0);
            fixPair(ed.v1);
            nMismatch++;
        }
    }

    std::cout<<"block smoother: "<<nPairs<<" periodic vertex pairs, "
             <<nEdgePairs<<" periodic edge pairs, "
             <<nMismatch<<" non-matching edges (end points kept fixed)"<<std::endl;
}




bool BlockMeshSmoother::isMovableEdge(int e) const
{
    const auto& ed = mp_.model.edges[e];
    auto ek = edgeKey(ed.v0, ed.v1);
    return
        ed.kind==BlockingModel::EdgeData::Spline
        && ed.v0!=ed.v1
        && !ed.intermediate.empty()
        && masterEdge_[e]<0
        && !fixedEdges_.count(ek)
        && constraintsOfEdge(ed.v0, ed.v1).size()<=1;
}




std::set<int> BlockMeshSmoother::blocksOfEdge(int e) const
{
    const auto& ed = mp_.model.edges[e];
    std::set<int> res;
    const auto& b0 = blocksOfVertex_[ed.v0];
    const auto& b1 = blocksOfVertex_[ed.v1];
    std::set_intersection(
        b0.begin(), b0.end(), b1.begin(), b1.end(),
        std::inserter(res, res.begin()) );
    return res;
}




double BlockMeshSmoother::penalizedObjective(const std::set<int>& blocks) const
{
    return penalizedObjective(mp_, blocks);
}




double BlockMeshSmoother::penalizedObjective(MeshPredictor& mp, const std::set<int>& blocks) const
{
    int nInvalid=0;
    double o = mp.objective(blocks, par_.p, nInvalid);
    if (nInvalid>0) o += 1e3*double(1+nInvalid);
    return o;
}




void BlockMeshSmoother::syncCheck(const std::set<int>& blocks)
{
    auto& cm = check_->model;
    const auto& m = mp_.model;
    cm.vertices = m.vertices;
    for (size_t e=0; e<m.edges.size(); ++e)
        cm.edges[e].intermediate = m.edges[e].intermediate;
    for (int bi: blocks) check_->updateBlock(bi);
}




bool BlockMeshSmoother::acceptMove(
    const std::set<int>& blocks, double fCheckBefore,
    const std::function<void()>& revert )
{
    if (!check_) return true;

    syncCheck(blocks);
    if (penalizedObjective(*check_, blocks) <= fCheckBefore)
        return true;

    // the improvement on the optimizer grid is not confirmed on the finer grid
    // (e.g. folding between the coarse grid lines): reject
    revert();
    syncCheck(blocks);
    nRejectedByCheck_++;
    return false;
}




double BlockMeshSmoother::minAdjacentEdgeLength(int v) const
{
    const auto& m = mp_.model;
    double h = DBL_MAX;
    for (int bi: blocksOfVertex_[v])
    {
        const auto& b = m.blocks[bi];
        for (int e=0; e<12; ++e)
        {
            int va = b.v[hexEdgeVerts[e][0]], vb = b.v[hexEdgeVerts[e][1]];
            int other = -1;
            if (va==v && vb!=v) other=vb;
            else if (vb==v && va!=v) other=va;
            if (other>=0)
            {
                double l = norm(m.vertices[other]-m.vertices[v]);
                if (l>0.) h=std::min(h, l);
            }
        }
    }
    return h;
}




void BlockMeshSmoother::updateSlaveEdge(int e)
{
    auto& m = mp_.model;
    int me = masterEdge_[e];
    const auto& T = par_.periodicTransforms[transformOfVertex_[m.edges[me].v0]];
    const auto& mip = m.edges[me].intermediate;
    auto& ip = m.edges[e].intermediate;
    size_t n=ip.size();
    for (size_t i=0; i<n; ++i)
        ip[i] = T( mip[ slaveReversed_[e] ? n-1-i : i ] );
}




bool BlockMeshSmoother::optimizeVertex(int v)
{
    auto& m = mp_.model;

    // periodic partner follows
    const int sv = slaveVertex_[v];
    const SmoothingParameters::PointFunction* T =
        sv>=0 ? &par_.periodicTransforms[transformOfVertex_[v]] : nullptr;

    std::set<int> blocks = blocksOfVertex_[v];
    std::set<int> edges = edgesOfVertex_[v];
    if (sv>=0)
    {
        blocks.insert(blocksOfVertex_[sv].begin(), blocksOfVertex_[sv].end());
        edges.insert(edgesOfVertex_[sv].begin(), edgesOfVertex_[sv].end());
    }

    double h = minAdjacentEdgeLength(v);
    if (h==DBL_MAX) return false;

    const V3 p0 = m.vertices[v];
    const V3 ps0 = sv>=0 ? m.vertices[sv] : V3();

    // attached splines follow the vertex (and are kept on their boundary surfaces),
    // splines paired with a master edge are transformed copies
    struct Attached { int e; std::vector<V3> pts0; std::vector<double> t; ConstraintSet cs; };
    std::vector<Attached> att;
    std::vector<int> slaveEdges;
    std::vector<int> monotonicEdges;
    for (int e: edges)
    {
        auto& ed = m.edges[e];
        if (ed.kind!=BlockingModel::EdgeData::Spline || ed.intermediate.empty()) continue;
        bool checkMonotonic = monotonicAlongChord(m.edgePoints(e));
        if (checkMonotonic) monotonicEdges.push_back(e);
        if (masterEdge_[e]>=0)
        {
            slaveEdges.push_back(e);
            continue;
        }
        auto t = chordParameter(m.edgePoints(e));
        Attached a;
        a.e=e;
        a.pts0=ed.intermediate;
        a.t.assign(t.begin()+1, t.end()-1);
        a.cs=constraintsOfEdge(ed.v0, ed.v1);
        att.push_back(a);
    }

    // block edges at the moved vertices and their original lengths
    std::vector<std::tuple<int,int,double> > adjacentEdges;
    for (int mv: { v, sv })
    {
        if (mv<0) continue;
        std::set<int> others;
        for (int bi: blocksOfVertex_[mv])
        {
            const auto& b = m.blocks[bi];
            for (int e=0; e<12; ++e)
            {
                int va = b.v[hexEdgeVerts[e][0]], vb = b.v[hexEdgeVerts[e][1]];
                if (va==mv && vb!=mv) others.insert(vb);
                else if (vb==mv && va!=mv) others.insert(va);
            }
        }
        for (int o: others)
            adjacentEdges.push_back(std::make_tuple(mv, o, norm(m.vertices[o]-m.vertices[mv])));
    }

    auto apply = [&](const arma::mat& x)
    {
        V3 q = projectOnto(vertexConstraints_[v], p0 + h*V3(x));
        m.vertices[v] = q;
        V3 d = q - p0, ds(0,0,0);
        if (sv>=0)
        {
            m.vertices[sv] = (*T)(q);
            ds = m.vertices[sv] - ps0;
        }
        auto disp = [&](int vi) { return vi==v ? d : ( vi==sv ? ds : V3(0,0,0) ); };

        for (const auto& a: att)
        {
            auto& ed = m.edges[a.e];
            V3 d0=disp(ed.v0), d1=disp(ed.v1);
            for (size_t i=0; i<ed.intermediate.size(); ++i)
                ed.intermediate[i] = projectOnto( a.cs, a.pts0[i] + (1.-a.t[i])*d0 + a.t[i]*d1 );
        }
        for (int e: slaveEdges)
            updateSlaveEdge(e);

        for (int bi: blocks) mp_.updateBlock(bi);
    };

    std::function<double(const arma::mat&)> obj =
        [&](const arma::mat& x)
        {
            apply(x);
            for (const auto& ae: adjacentEdges)
                if ( norm(m.vertices[std::get<0>(ae)]-m.vertices[std::get<1>(ae)])
                     < minEdgeLengthRatio*std::get<2>(ae) )
                    return inadmissible;
            for (int e: monotonicEdges)
                if (!monotonicAlongChord(m.edgePoints(e)))
                    return inadmissible;
            return penalizedObjective(blocks);
        };

    const double fCheck0 = check_ ? penalizedObjective(*check_, blocks) : 0.;

    arma::mat x0 = arma::zeros(3);
    double f0 = obj(x0);

    arma::mat x;
    try
    {
        x = nonlinearMinimizeND(
            obj, x0, 1e-3,
            arma::ones(3)*par_.vertexStepRel,
            par_.maxIterLocal );
    }
    catch (const std::exception&)
    {
        apply(x0);
        return false;
    }

    double f1 = obj(x);
    if (f1 < f0*(1.-1e-9))
        return acceptMove(blocks, fCheck0, [&]() { apply(x0); });

    apply(x0);
    return false;
}




bool BlockMeshSmoother::optimizeEdge(int e)
{
    auto& m = mp_.model;
    auto& ed = m.edges[e];
    std::set<int> blocks = blocksOfEdge(e);
    const int se = slaveEdge_[e];
    if (se>=0)
    {
        auto sb = blocksOfEdge(se);
        blocks.insert(sb.begin(), sb.end());
    }
    if (blocks.empty()) return false;

    double L = norm(m.vertices[ed.v1]-m.vertices[ed.v0]);
    if (L<=0.) return false;

    const std::vector<V3> pts0 = ed.intermediate;
    auto t = chordParameter(m.edgePoints(e));
    const bool checkMonotonic = monotonicAlongChord(m.edgePoints(e));
    const ConstraintSet cs = constraintsOfEdge(ed.v0, ed.v1);

    auto apply = [&](const arma::mat& x)
    {
        V3 a1 = L*V3(arma::mat(x.rows(0,2))), a2 = L*V3(arma::mat(x.rows(3,5)));
        for (size_t i=0; i<pts0.size(); ++i)
        {
            double ti=t[i+1];
            ed.intermediate[i] = projectOnto( cs,
                pts0[i] + a1*std::sin(M_PI*ti) + a2*std::sin(2.*M_PI*ti) );
        }
        if (se>=0) updateSlaveEdge(se);
        for (int bi: blocks) mp_.updateBlock(bi);
    };

    std::function<double(const arma::mat&)> obj =
        [&](const arma::mat& x)
        {
            apply(x);
            if (checkMonotonic && !monotonicAlongChord(m.edgePoints(e)))
                return inadmissible;
            return penalizedObjective(blocks);
        };

    const double fCheck0 = check_ ? penalizedObjective(*check_, blocks) : 0.;

    arma::mat x0 = arma::zeros(6);
    double f0 = obj(x0);

    arma::mat x;
    try
    {
        x = nonlinearMinimizeND(
            obj, x0, 1e-3,
            arma::ones(6)*par_.edgeStepRel,
            par_.maxIterLocal );
    }
    catch (const std::exception&)
    {
        apply(x0);
        return false;
    }

    double f1 = obj(x);
    if (f1 < f0*(1.-1e-9))
        return acceptMove(blocks, fCheck0, [&]() { apply(x0); });

    apply(x0);
    return false;
}




NonOrthogonalityStatistics BlockMeshSmoother::run(ActionProgress& progress)
{
    typedef std::chrono::steady_clock Clock;
    auto seconds = [](Clock::duration d) { return std::chrono::duration<double>(d).count(); };
    auto formatTime = [](double s)
    {
        if (s<120.) return str(boost::format("%.0f s")%s);
        else if (s<7200.) return str(boost::format("%.1f min")%(s/60.));
        else return str(boost::format("%.1f h")%(s/3600.));
    };

    progress.message("classifying vertices and edges, converting interior edges");
    classify();
    convertEdges();
    findPeriodicPairs();
    mp_.updateAll();

    check_.reset();
    nRejectedByCheck_=0;
    if (par_.checkMaxCells > par_.maxCells)
    {
        progress.message("setting up the mesh prediction for checking the local modifications");
        check_.reset(new MeshPredictor(mp_.model, par_.checkMaxCells));
        if (check_->resolutionFactor() <= mp_.resolutionFactor())
            check_.reset(); // no finer than the optimizer grid
    }

    const auto& m = mp_.model;

    std::vector<int> movableVertices, movableEdges;
    int nSliding=0;
    for (size_t v=0; v<m.vertices.size(); ++v)
    {
        if (!fixedVertex_[v] && masterVertex_[v]<0)
        {
            movableVertices.push_back(int(v));
            if (!vertexConstraints_[v].empty()) nSliding++;
        }
    }
    for (size_t e=0; e<m.edges.size(); ++e)
        if (isMovableEdge(int(e))) movableEdges.push_back(int(e));

    const int nItems = int(movableVertices.size()+movableEdges.size());
    progress.setNSteps(std::max(1, par_.nSweeps*nItems));

    progress.message(str(boost::format(
        "%d movable vertices (%d of them on boundaries), %d movable edges, resolution factor %g")
                         % movableVertices.size() % nSliding % movableEdges.size()
                         % mp_.resolutionFactor()));
    progress.message("initial "+mp_.statistics().summary());

    const double f0 = penalizedObjective({});
    double f = f0;

    const int reportInterval = std::max(1, nItems/50);
    auto tStart = Clock::now();
    int nDone = 0;
    bool stopped = false;

    for (int sweep=0; sweep<par_.nSweeps && !stopped; ++sweep)
    {
        auto tSweep = Clock::now();
        int nImproved=0, item=0;

        auto processItem = [&](const std::string& kind, int idx, int nOfKind, bool improved)
        {
            if (improved) nImproved++;
            item++; nDone++;

            if ( item%reportInterval==0 || item==nItems )
            {
                double tPerItem = seconds(Clock::now()-tStart)/double(nDone);
                double remaining = tPerItem*double(par_.nSweeps*nItems-nDone);
                progress.stepUp(str(boost::format(
                    "sweep %d/%d: %s %d/%d, %d improved, %.2g s/item, max. remaining %s")
                    % (sweep+1) % par_.nSweeps % kind % idx % nOfKind
                    % nImproved % tPerItem % formatTime(remaining) ));
            }
            else
            {
                progress.stepUp();
            }

            stopped = progress.stopIsDemanded();
        };

        for (size_t i=0; i<movableVertices.size() && !stopped; ++i)
            processItem("vertex", int(i+1), int(movableVertices.size()), optimizeVertex(movableVertices[i]));

        for (size_t i=0; i<movableEdges.size() && !stopped; ++i)
            processItem("edge", int(i+1), int(movableEdges.size()), optimizeEdge(movableEdges[i]));

        double fn = penalizedObjective({});
        progress.message(str(boost::format(
            "sweep %d/%d finished after %s: objective %g -> %g (start %g), %d items improved, %d rejected by the check")
                       % (sweep+1) % par_.nSweeps % formatTime(seconds(Clock::now()-tSweep))
                       % f % fn % f0 % nImproved % nRejectedByCheck_ ));
        nRejectedByCheck_=0;
        progress.message(mp_.statistics().summary());

        bool converged = (f-fn) < par_.relTol*f;
        f=fn;
        if (converged)
        {
            progress.message("converged");
            break;
        }
    }

    if (stopped)
        progress.message("stopped by user, keeping the current state");

    progress.completed();

    return mp_.statistics();
}




} // namespace bmd
} // namespace insight
