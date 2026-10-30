#include "blockmeshsmoother.h"

#include "base/linearalgebra.h"
#include "base/exception.h"
#include "base/actionprogress.h"

#include <chrono>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <memory>
#include <tuple>

#include "boost/format.hpp"
#include "boost/functional/hash.hpp"

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

// rotation of a by the rotation vector w (Rodrigues)
V3 rotate(const V3& w, const V3& a)
{
    double th = norm(w);
    if (th < 1e-14) return a;
    V3 k = w/th;
    double c = std::cos(th), s = std::sin(th);
    return c*a + s*cross(k, a) + (dot(k, a)*(1.-c))*k;
}

const double inadmissible = 1e30;

// block edges may not shrink below this fraction of their original length
// (prevents collapsing edges, which removes the bad faces from the objective)
const double minEdgeLengthRatio = 0.25;

}




namespace {
const char* modificationFileHeader = "InsightCAE blocking modification 1";
}


void BlockingModification::write(const boost::filesystem::path& file) const
{
    std::ofstream f(file.string());
    insight::assertion(f.good(), "could not open file %s for writing", file.string().c_str());
    f<<modificationFileHeader<<"\n"
     <<"topology "<<topology<<"\n"
     <<"summary "<<summary<<"\n"
     <<std::setprecision(17);
    auto writeMatrix = [&](const std::string& name, const arma::mat& m)
    {
        f<<name<<" "<<m.n_rows<<" "<<m.n_cols<<"\n";
        for (arma::uword i=0; i<m.n_rows; ++i)
        {
            for (arma::uword j=0; j<m.n_cols; ++j)
                f<<(j>0?" ":"")<<m(i,j);
            f<<"\n";
        }
    };
    writeMatrix("vertices", vertices);
    writeMatrix("edges", edges);
    insight::assertion(f.good(), "error while writing file %s", file.string().c_str());
}




BlockingModification BlockingModification::read(const boost::filesystem::path& file)
{
    std::ifstream f(file.string());
    insight::assertion(f.good(), "could not open file %s", file.string().c_str());

    std::string line;
    std::getline(f, line);
    insight::assertion(line==modificationFileHeader,
                       "file %s does not contain a blocking modification", file.string().c_str());

    BlockingModification mod;
    auto readField = [&](const std::string& name, std::string& value)
    {
        std::getline(f, line);
        insight::assertion(line.compare(0, name.size()+1, name+" ")==0 || line==name,
                           "expected entry \"%s\" in file %s", name.c_str(), file.string().c_str());
        value = line.size()>name.size() ? line.substr(name.size()+1) : std::string();
    };
    readField("topology", mod.topology);
    readField("summary", mod.summary);

    auto readMatrix = [&](const std::string& name, arma::mat& m, arma::uword nCols)
    {
        std::string n;
        arma::uword r, c;
        f>>n>>r>>c;
        insight::assertion(f.good() && n==name && c==nCols,
                           "invalid entry \"%s\" in file %s", name.c_str(), file.string().c_str());
        m = arma::zeros(r, c);
        for (arma::uword i=0; i<r; ++i)
            for (arma::uword j=0; j<c; ++j)
                f>>m(i,j);
        insight::assertion(!f.fail(), "incomplete entry \"%s\" in file %s", name.c_str(), file.string().c_str());
    };
    readMatrix("vertices", mod.vertices, 7);
    readMatrix("edges", mod.edges, 5);

    return mod;
}




std::string topologyFingerprint(const BlockingModel& m)
{
    size_t h = 0;
    boost::hash_combine(h, m.vertices.size());
    for (const auto& b: m.blocks)
        for (int v: b.v)
            boost::hash_combine(h, v);
    return str(boost::format("v%d-b%d-%016x")
               % m.vertices.size() % m.blocks.size() % uint64_t(h));
}




BlockMeshSmoother::BlockMeshSmoother(MeshPredictor& mp, const SmoothingParameters& par)
: mp_(mp), par_(par)
{}




void BlockMeshSmoother::setup()
{
    if (setupDone_) return;

    topology_ = topologyFingerprint(mp_.model);

    classify();
    convertEdges();
    findPeriodicPairs();
    initShapes();

    faceOrientation0_.resize(mp_.model.blocks.size());
    for (size_t bi=0; bi<mp_.model.blocks.size(); ++bi)
        for (int f=0; f<6; ++f)
            faceOrientation0_[bi][f] = faceOrientation(int(bi), f);
    mp_.updateAll();

    setupDone_ = true;
}




bool BlockMeshSmoother::localFrame(int v, std::array<V3,3>& R, double& h) const
{
    const auto& m = mp_.model;

    std::set<int> others;
    for (int bi: blocksOfVertex_[v])
    {
        const auto& b = m.blocks[bi];
        for (int e=0; e<12; ++e)
        {
            int va = b.v[hexEdgeVerts[e][0]], vb = b.v[hexEdgeVerts[e][1]];
            if (va==v && vb!=v) others.insert(vb);
            else if (vb==v && va!=v) others.insert(va);
        }
    }

    h = DBL_MAX;
    std::vector<V3> dirs;
    for (int o: others) // ordered by vertex index
    {
        V3 d = origVertices_[o]-origVertices_[v];
        double l = norm(d);
        if (l>0.)
        {
            h = std::min(h, l);
            dirs.push_back(d/l);
        }
    }
    if (dirs.empty()) return false;

    R[0] = dirs[0];
    for (size_t i=1; i<dirs.size(); ++i)
    {
        V3 e2 = dirs[i] - dot(dirs[i], R[0])*R[0];
        double l = norm(e2);
        if (l>0.1) // |sin| of the angle to e1
        {
            R[1] = e2/l;
            R[2] = cross(R[0], R[1]);
            return true;
        }
    }
    return false;
}




bool BlockMeshSmoother::apply(const BlockingModification& mod, std::string* message)
{
    setup();

    auto& m = mp_.model;
    auto result = [&](bool ok, const std::string& msg)
    {
        if (message) *message = msg;
        return ok;
    };

    if (mod.topology != topology_)
        return result(false, str(boost::format(
            "the stored modification was computed for a different blocking topology (%s, this blocking: %s)")
                                 % mod.topology % topology_ ));
    insight::assertion(mod.vertices.n_rows==0 || mod.vertices.n_cols==7, "stored vertex modifications: expected 7 columns");
    insight::assertion(mod.edges.n_rows==0 || mod.edges.n_cols==5, "stored edge modifications: expected 5 columns");

    // state for reverting
    const auto vertices0 = m.vertices;
    const auto rotation0 = vertexRotation_;
    std::vector<std::vector<V3> > intermediate0;
    std::vector<V3> bow0;
    for (const auto& ed: m.edges) intermediate0.push_back(ed.intermediate);
    for (const auto& s: shape_) bow0.push_back(s.bow);
    auto revert = [&]()
    {
        m.vertices = vertices0;
        vertexRotation_ = rotation0;
        for (size_t e=0; e<m.edges.size(); ++e) m.edges[e].intermediate = intermediate0[e];
        for (size_t e=0; e<shape_.size(); ++e) shape_[e].bow = bow0[e];
    };

    auto toGlobal = [](const std::array<V3,3>& R, double x, double y, double z)
    {
        return x*R[0] + y*R[1] + z*R[2];
    };

    int nApplied=0, nSkipped=0;
    for (arma::uword r=0; r<mod.vertices.n_rows; ++r)
    {
        int v = int(std::lround(mod.vertices(r,0)));
        std::array<V3,3> R;
        double h;
        if ( v<0 || v>=int(m.vertices.size())
             || fixedVertex_[v] || masterVertex_[v]>=0
             || !localFrame(v, R, h) )
        {
            nSkipped++;
            continue;
        }

        V3 q = projectOnto(
            vertexConstraints_[v],
            origVertices_[v]
                + h*toGlobal(R, mod.vertices(r,1), mod.vertices(r,2), mod.vertices(r,3)) );
        m.vertices[v] = q;
        vertexRotation_[v] = toGlobal(R, mod.vertices(r,4), mod.vertices(r,5), mod.vertices(r,6));

        // periodic partner follows
        int sv = slaveVertex_[v];
        if (sv>=0)
        {
            const auto& T = par_.periodicTransforms[transformOfVertex_[v]];
            V3 Tq = T(q);
            std::array<V3,3> J;
            for (int k=0; k<3; ++k)
            {
                V3 dk(0,0,0); dk(k)=h;
                J[k] = (T(q+dk)-Tq)/h;
            }
            m.vertices[sv] = Tq;
            const V3& w = vertexRotation_[v];
            vertexRotation_[sv] = w.x*J[0] + w.y*J[1] + w.z*J[2];
        }
        nApplied++;
    }

    int nEdgesApplied=0, nEdgesSkipped=0;
    for (arma::uword r=0; r<mod.edges.n_rows; ++r)
    {
        int v0 = int(std::lround(mod.edges(r,0))), v1 = int(std::lround(mod.edges(r,1)));
        int e = (v0>=0 && v1>=0 && v0<int(m.vertices.size()) && v1<int(m.vertices.size())) ?
                    m.findEdge(v0, v1) : -1;
        std::array<V3,3> R;
        double h;
        if (e<0 || !isMovableEdge(e) || !localFrame(v0, R, h))
        {
            nEdgesSkipped++;
            continue;
        }
        double L = norm(origVertices_[v1]-origVertices_[v0]);
        shape_[e].bow = L*toGlobal(R, mod.edges(r,2), mod.edges(r,3), mod.edges(r,4));
        nEdgesApplied++;
    }

    for (size_t e=0; e<m.edges.size(); ++e)
        if (shape_[e].active) regenerateEdge(int(e));
    for (size_t e=0; e<m.edges.size(); ++e)
        if (masterEdge_[e]>=0) updateSlaveEdge(int(e));

    // validity
    std::set<int> allBlocks;
    for (size_t bi=0; bi<m.blocks.size(); ++bi) allBlocks.insert(int(bi));
    std::set<Point> distinct;
    for (const auto& v: m.vertices) distinct.insert(v.toArma());
    if (distinct.size()!=m.vertices.size())
    {
        revert();
        return result(false, "the stored modification moves vertices onto other vertices");
    }
    if (!blockFacesValid(allBlocks))
    {
        revert();
        return result(false, "the stored modification creates inverted blocks");
    }

    mp_.updateAll();

    std::string msg = str(boost::format("applied the stored modification to %d vertices") % nApplied);
    if (nEdgesApplied) msg += str(boost::format(" and %d edges") % nEdgesApplied);
    if (nSkipped || nEdgesSkipped)
        msg += str(boost::format(" (%d vertices and %d edges skipped, since they are fixed in this blocking)")
                   % nSkipped % nEdgesSkipped);
    return result(true, msg);
}




BlockingModification BlockMeshSmoother::extract() const
{
    insight::assertion(setupDone_, "the smoother was not set up yet");

    const auto& m = mp_.model;
    BlockingModification mod;
    mod.topology = topology_;

    std::vector<arma::rowvec> vrows, erows;
    for (size_t v=0; v<m.vertices.size(); ++v)
    {
        if (fixedVertex_[v] || masterVertex_[v]>=0) continue;
        std::array<V3,3> R;
        double h;
        if (!localFrame(int(v), R, h)) continue;

        V3 d = (m.vertices[v]-origVertices_[v])/h;
        const V3& w = vertexRotation_[v];
        if (norm(d)<1e-12 && norm(w)<1e-12) continue;

        vrows.push_back(arma::rowvec({
            double(v),
            dot(d, R[0]), dot(d, R[1]), dot(d, R[2]),
            dot(w, R[0]), dot(w, R[1]), dot(w, R[2]) }));
    }

    for (size_t e=0; e<m.edges.size(); ++e)
    {
        const auto& s = shape_[e];
        if (!s.active || norm(s.bow)==0.) continue;
        const auto& ed = m.edges[e];
        std::array<V3,3> R;
        double h;
        double L = norm(origVertices_[ed.v1]-origVertices_[ed.v0]);
        if (L<=0. || !localFrame(ed.v0, R, h)) continue;
        V3 b = s.bow/L;
        erows.push_back(arma::rowvec({
            double(ed.v0), double(ed.v1),
            dot(b, R[0]), dot(b, R[1]), dot(b, R[2]) }));
    }

    mod.vertices = arma::zeros(vrows.size(), 7);
    for (size_t i=0; i<vrows.size(); ++i) mod.vertices.row(i) = vrows[i];
    mod.edges = arma::zeros(erows.size(), 5);
    for (size_t i=0; i<erows.size(); ++i) mod.edges.row(i) = erows[i];

    return mod;
}




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
        std::vector<V3> facePoints;
        for (int k=0; k<4; ++k)
        {
            int va=c[k], vb=c[(k+1)%4];
            facePoints.push_back(m.vertices[va]);
            int e = (va!=vb) ? m.findEdge(va, vb) : -1;
            if (e>=0)
            {
                if (m.edges[e].kind==BlockingModel::EdgeData::Fixed) return -1;
                auto ep = m.edgePoints(e);
                facePoints.insert(facePoints.end(), ep.begin(), ep.end());
            }
        }

        // in an already known plane? (the normal of small or degenerate faces is sensitive to round-off,
        // nearly parallel planes would restrict the vertices to their intersection)
        for (size_t i=0; i<constraints_.size(); ++i)
        {
            const auto& o = constraints_[i];
            if ( o.label.empty()
                 && std::all_of(facePoints.begin(), facePoints.end(),
                                [&](const V3& p) { return std::fabs(dot(o.n, p)-o.d) <= tol_; }) )
                return int(i);
        }

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

        for (const auto& p: facePoints)
            if (std::fabs(dot(n, p)-d) > tol_) return -1;

        Constraint cs;
        cs.n=n;
        cs.d=d;
        return addConstraint(cs);
    };

    // distinct vertices of a block face
    auto distinctFaceVertices = [&](int bi, int f)
    {
        std::set<int> d;
        for (int k=0; k<4; ++k)
            d.insert(m.blocks[bi].v[MeshPredictor::hexFaces[f][k]]);
        return d;
    };

    // triangular faces of collapsed blocks: on the boundary, if not shared with another block
    std::map<std::set<int>, int> triangleFaceCount;
    for (size_t bi=0; bi<m.blocks.size(); ++bi)
    {
        for (int f=0; f<6; ++f)
        {
            auto d = distinctFaceVertices(int(bi), f);
            if (d.size()==3) triangleFaceCount[d]++;
        }
    }

    // constrain or fix the vertices and edges of a boundary face
    auto applyFaceConstraint = [&](int bi, int f, int fc)
    {
        const auto& b = m.blocks[bi];
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
    };

    // The boundary faces are processed in the order of decreasing area (triangular faces of collapsed blocks last):
    // the plane is defined by the first face, the normal of small faces is sensitive to round-off
    struct BoundaryFace { int bi, f; bool triangle; double area; };
    std::vector<BoundaryFace> boundaryFaces;
    auto faceArea = [&](int bi, int f)
    {
        V3 n(0,0,0);
        for (int k=0; k<4; ++k)
            n += cross( m.vertices[m.blocks[bi].v[MeshPredictor::hexFaces[f][k]]],
                        m.vertices[m.blocks[bi].v[MeshPredictor::hexFaces[f][(k+1)%4]]] );
        return 0.5*norm(n);
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
            // Triangular boundary faces of collapsed blocks as well.
            // Faces collapsed into a line (i.e. the end points of collapsed edges),
            // unmatched faces, triangles between blocks, other boundary faces: keep fixed
            if (fi==-2)
            {
                auto d = distinctFaceVertices(int(bi), f);
                if (d.size()==3 && triangleFaceCount[d]==1)
                {
                    boundaryFaces.push_back({int(bi), f, true, faceArea(int(bi), f)});
                }
                else
                {
                    applyFaceConstraint(int(bi), f, -1);
                }
            }
            else
            {
                boundaryFaces.push_back({int(bi), f, false, faceArea(int(bi), f)});
            }
        }
    }

    std::stable_sort(
        boundaryFaces.begin(), boundaryFaces.end(),
        [](const BoundaryFace& a, const BoundaryFace& b)
        { return a.triangle!=b.triangle ? b.triangle : a.area>b.area; } );
    for (const auto& bf: boundaryFaces)
        applyFaceConstraint(bf.bi, bf.f, faceConstraint(bf.bi, bf.f));

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
        && shape_[e].active
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




void BlockMeshSmoother::initShapes()
{
    const auto& m = mp_.model;

    origVertices_ = m.vertices;
    vertexRotation_.assign(m.vertices.size(), V3(0,0,0));
    shape_.assign(m.edges.size(), EdgeShape());

    for (size_t e=0; e<m.edges.size(); ++e)
    {
        const auto& ed = m.edges[e];
        if ( ed.kind!=BlockingModel::EdgeData::Spline || ed.intermediate.empty()
             || ed.v0==ed.v1 || masterEdge_[e]>=0 || fixedEdges_.count(edgeKey(ed.v0, ed.v1)) )
            continue;

        auto& s = shape_[e];
        s.active = true;
        s.pts0 = ed.intermediate;
        auto t = chordParameter(m.edgePoints(int(e)));
        s.t.assign(t.begin()+1, t.end()-1);

        EdgeCurve c = m.curve(ed.v0, ed.v1);
        const double eps = 1e-3;
        s.tau0 = c.position(eps) - m.vertices[ed.v0];
        s.tau1 = m.vertices[ed.v1] - c.position(1.-eps);
        s.tau0 /= std::max(norm(s.tau0), DBL_MIN);
        s.tau1 /= std::max(norm(s.tau1), DBL_MIN);
    }
}




V3 BlockMeshSmoother::followedPoint(int e, size_t i) const
{
    // the original shape follows the displacements of the end vertices (linear blending)
    const auto& m = mp_.model;
    const auto& ed = m.edges[e];
    const double t = shape_[e].t[i];
    return shape_[e].pts0[i]
        + (1.-t)*(m.vertices[ed.v0]-origVertices_[ed.v0])
        + t*(m.vertices[ed.v1]-origVertices_[ed.v1]);
}




V3 BlockMeshSmoother::deformedPoint(int e, size_t i) const
{
    const auto& m = mp_.model;
    const auto& ed = m.edges[e];
    const auto& s = shape_[e];

    const double t = s.t[i];
    const double L = norm(m.vertices[ed.v1]-m.vertices[ed.v0]);

    // cubic Hermite basis of the end tangents
    const double h10 = t*t*t - 2.*t*t + t, h11 = t*t*t - t*t;

    return followedPoint(e, i)
        + L*( h10*(rotate(vertexRotation_[ed.v0], s.tau0)-s.tau0)
             + h11*(rotate(vertexRotation_[ed.v1], s.tau1)-s.tau1) )
        + std::sin(M_PI*t)*s.bow;
}




void BlockMeshSmoother::regenerateEdge(int e)
{
    auto& ed = mp_.model.edges[e];
    const auto cs = constraintsOfEdge(ed.v0, ed.v1);
    for (size_t i=0; i<ed.intermediate.size(); ++i)
        ed.intermediate[i] = projectOnto(cs, deformedPoint(e, i));
}




double BlockMeshSmoother::edgeDeformationRel(int e) const
{
    const auto& m = mp_.model;
    const auto& ed = m.edges[e];
    const auto& s = shape_[e];
    if (!s.active) return 0.;

    const double L = norm(m.vertices[ed.v1]-m.vertices[ed.v0]);
    if (L<=0.) return 0.;

    double d=0.;
    for (size_t i=0; i<s.pts0.size(); ++i)
        d = std::max(d, norm(deformedPoint(e, i)-followedPoint(e, i)));
    return d/L;
}




double BlockMeshSmoother::faceOrientation(int bi, int f) const
{
    // as blockMesh checks the blocks: face normal times the distance vector of block centre and face centre.
    // Centres from the distinct vertices (collapsed blocks)
    const auto& m = mp_.model;
    const auto& b = m.blocks[bi];

    auto centre = [&](const std::set<int>& vs)
    {
        V3 c(0,0,0);
        for (int v: vs) c += m.vertices[v];
        return c/double(vs.size());
    };

    std::set<int> fv;
    V3 n(0,0,0);
    for (int k=0; k<4; ++k)
    {
        int va = b.v[MeshPredictor::hexFaces[f][k]], vb = b.v[MeshPredictor::hexFaces[f][(k+1)%4]];
        fv.insert(va);
        n += cross(m.vertices[va], m.vertices[vb]);
    }
    if (fv.size()<3) return 0.;

    return dot(n, centre(fv)-centre(std::set<int>(b.v.begin(), b.v.end())));
}




bool BlockMeshSmoother::blockFacesValid(const std::set<int>& blocks) const
{
    for (int bi: blocks)
    {
        for (int f=0; f<6; ++f)
        {
            double o0 = faceOrientation0_[bi][f];
            if (o0!=0. && faceOrientation(bi, f)/o0 < 1e-2)
                return false;
        }
    }
    return true;
}




bool BlockMeshSmoother::admissibleEdge(int e) const
{
    return edgeDeformationRel(e) <= par_.maxBowRel;
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
    const V3 w0 = vertexRotation_[v];

    // splines regenerated from the vertex state,
    // splines paired with a master edge are transformed copies
    std::vector<int> shapedEdges, slaveEdges, monotonicEdges;
    for (int e: edges)
    {
        const auto& ed = m.edges[e];
        if (ed.kind!=BlockingModel::EdgeData::Spline || ed.intermediate.empty()) continue;
        if (monotonicAlongChord(m.edgePoints(e))) monotonicEdges.push_back(e);
        if (masterEdge_[e]>=0) slaveEdges.push_back(e);
        else if (shape_[e].active) shapedEdges.push_back(e);
    }

    // linear part of the periodic transformation (rotates the tangents of the slave vertex)
    std::array<V3,3> J;
    if (sv>=0)
    {
        V3 Tp = (*T)(p0);
        for (int k=0; k<3; ++k)
        {
            V3 dk(0,0,0); dk(k)=h;
            J[k] = ((*T)(p0+dk)-Tp)/h;
        }
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

    // tangent rotations only, if there are splines to rotate
    const int nPar = shapedEdges.empty() ? 3 : 6;
    const double rotScale = par_.rotationStepDeg*M_PI/180.;
    const double maxRot = par_.maxTangentRotationDeg*M_PI/180.;

    auto apply = [&](const arma::mat& x)
    {
        V3 q = projectOnto(vertexConstraints_[v], p0 + h*V3(x(0), x(1), x(2)));
        m.vertices[v] = q;
        if (nPar==6)
            vertexRotation_[v] = w0 + rotScale*V3(x(3), x(4), x(5));
        if (sv>=0)
        {
            m.vertices[sv] = (*T)(q);
            const V3& w = vertexRotation_[v];
            vertexRotation_[sv] = w.x*J[0] + w.y*J[1] + w.z*J[2];
        }

        for (int e: shapedEdges) regenerateEdge(e);
        for (int e: slaveEdges) updateSlaveEdge(e);

        for (int bi: blocks) mp_.updateBlock(bi);
    };

    double fRef = 0.; // scale of the fairness penalty
    auto fairness = [&]()
    {
        return par_.fairnessWeight*fRef*magSqr(vertexRotation_[v])/(maxRot*maxRot);
    };

    std::function<double(const arma::mat&)> obj =
        [&](const arma::mat& x)
        {
            apply(x);
            if (norm(vertexRotation_[v]) > maxRot)
                return inadmissible;
            for (const auto& ae: adjacentEdges)
                if ( norm(m.vertices[std::get<0>(ae)]-m.vertices[std::get<1>(ae)])
                     < minEdgeLengthRatio*std::get<2>(ae) )
                    return inadmissible;
            for (int e: shapedEdges)
                if (!admissibleEdge(e))
                    return inadmissible;
            if (!blockFacesValid(blocks))
                return inadmissible;
            for (int e: monotonicEdges)
                if (!monotonicAlongChord(m.edgePoints(e)))
                    return inadmissible;
            return penalizedObjective(blocks) + fairness();
        };

    const double fCheck0 = check_ ? penalizedObjective(*check_, blocks) : 0.;

    arma::mat x0 = arma::zeros(nPar);
    apply(x0);
    fRef = penalizedObjective(blocks);
    double f0 = obj(x0);

    arma::mat x;
    try
    {
        arma::mat steps = arma::ones(nPar)*par_.vertexStepRel;
        if (nPar==6) steps.rows(3,5).fill(1.);
        x = nonlinearMinimizeND(obj, x0, 1e-3, steps, par_.maxIterLocal);
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

    V3 c = m.vertices[ed.v1]-m.vertices[ed.v0];
    const double L = norm(c);
    if (L<=0.) return false;
    c/=L;

    // bow directions normal to the chord
    V3 a = std::fabs(c.x)<0.9 ? V3(1,0,0) : V3(0,1,0);
    V3 e1 = cross(c, a); e1/=norm(e1);
    V3 e2 = cross(c, e1);

    auto& s = shape_[e];
    const V3 bow0 = s.bow;
    const bool checkMonotonic = monotonicAlongChord(m.edgePoints(e));
    const double maxBow = par_.maxBowRel*L;

    auto apply = [&](const arma::mat& x)
    {
        s.bow = bow0 + L*par_.edgeStepRel*(x(0)*e1 + x(1)*e2);
        regenerateEdge(e);
        if (se>=0) updateSlaveEdge(se);
        for (int bi: blocks) mp_.updateBlock(bi);
    };

    double fRef = 0.;
    std::function<double(const arma::mat&)> obj =
        [&](const arma::mat& x)
        {
            apply(x);
            if (!admissibleEdge(e))
                return inadmissible;
            if (checkMonotonic && !monotonicAlongChord(m.edgePoints(e)))
                return inadmissible;
            return penalizedObjective(blocks)
                   + par_.fairnessWeight*fRef*magSqr(s.bow)/(maxBow*maxBow);
        };

    const double fCheck0 = check_ ? penalizedObjective(*check_, blocks) : 0.;

    arma::mat x0 = arma::zeros(2);
    apply(x0);
    fRef = penalizedObjective(blocks);
    double f0 = obj(x0);

    arma::mat x;
    try
    {
        x = nonlinearMinimizeND(obj, x0, 1e-3, arma::ones(2), par_.maxIterLocal);
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




NonOrthogonalityStatistics BlockMeshSmoother::run(
    ActionProgress& progress,
    const BlockingModification* initial )
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
    setup();

    if (initial && !initial->empty())
    {
        std::string msg;
        bool ok = apply(*initial, &msg);
        progress.message(
            ok ? "warm start: "+msg
               : "warm start not possible ("+msg+"), starting from the unmodified blocking" );
    }

    check_.reset();
    nRejectedByCheck_=0;
    if (par_.checkMaxCells > par_.maxCells)
    {
        progress.message("setting up the mesh prediction for checking the local modifications");
        check_.reset(new MeshPredictor(mp_.model, par_.checkMaxCells, par_.minCellsPerBlockDir));
        if (check_->nCells() <= mp_.nCells())
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
    if (par_.edgeBowModes)
        for (size_t e=0; e<m.edges.size(); ++e)
            if (isMovableEdge(int(e))) movableEdges.push_back(int(e));

    const int nItems = int(movableVertices.size()+movableEdges.size());
    progress.setNSteps(std::max(1, par_.nSweeps*nItems));

    progress.message(str(boost::format(
        "%d movable vertices (%d of them on boundaries), %d movable edges, resolution factor %g, %d cells (min. %d per block and direction)")
                         % movableVertices.size() % nSliding % movableEdges.size()
                         % mp_.resolutionFactor() % mp_.nCells() % par_.minCellsPerBlockDir));
    if (mp_.nCells() > par_.maxCells)
        progress.message(str(boost::format(
            "note: the optimizer mesh has more cells than maxCells=%d to resolve each block with min. %d cells per direction")
                             % par_.maxCells % par_.minCellsPerBlockDir));
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

    // measures of the deformation of the edges ("waviness")
    {
        double maxDef=0., maxRot=0.;
        int nDeformed=0;
        for (size_t e=0; e<m.edges.size(); ++e)
        {
            double d = edgeDeformationRel(int(e));
            maxDef = std::max(maxDef, d);
            if (d > 1e-3) nDeformed++;
        }
        for (const auto& w: vertexRotation_)
            maxRot = std::max(maxRot, norm(w));
        progress.message(str(boost::format(
            "edge deformation: %d edges bent, max. deviation %.3g of the edge length (limit %g), max. tangent rotation %.1f deg (limit %g deg)")
                             % nDeformed % maxDef % par_.maxBowRel
                             % (maxRot*180./M_PI) % par_.maxTangentRotationDeg ));
    }

    progress.completed();

    return mp_.statistics();
}




} // namespace bmd
} // namespace insight
