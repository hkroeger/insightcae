#include "blockmeshprediction.h"

#include "openfoam/blockmesh.h"
#include "openfoam/blockmesh/splineedge.h"
#include "openfoam/blockmesh/arcedge.h"

#include "base/exception.h"

#include <cmath>
#include <algorithm>

#include "boost/format.hpp"

namespace insight {
namespace bmd {


namespace {

// OpenFOAM's (double precision) SMALL
const double foamSMALL = 1e-15;
const double foamVSMALL = 1e-300;
const double foamROOTVSMALL = 1e-150;

// edges of a hex in OpenFOAM order
const int hexEdges[12][2] = {
    {0,1}, {3,2}, {7,6}, {4,5},
    {0,3}, {1,2}, {5,6}, {4,7},
    {0,4}, {1,5}, {2,6}, {3,7}
};

// block vertex number from local corner coordinates
inline int cornerIndex(int x, int y, int z)
{
    return 4*z + ( y ? (x ? 2 : 3) : (x ? 1 : 0) );
}

// axis and side of the OpenFOAM hex faces
const int faceAxis[6] = {0, 0, 1, 1, 2, 2};
const int faceSide[6] = {0, 1, 0, 1, 0, 1};

// in-plane axes (ascending) of a face normal to axis a
inline void faceUV(int a, int& u, int& v)
{
    u = (a==0) ? 1 : 0;
    v = (a==2) ? 1 : 2;
}

// factor to orient the area vector of a face given by (u,v)-ordered points along the positive axis direction
inline double uvOrientation(int a)
{
    return a==1 ? -1. : 1.;
}

} // anonymous namespace




const int MeshPredictor::hexFaces[6][4] = {
    {0, 4, 7, 3},   // x-min
    {1, 2, 6, 5},   // x-max
    {0, 1, 5, 4},   // y-min
    {3, 7, 6, 2},   // y-max
    {0, 3, 2, 1},   // z-min
    {4, 5, 6, 7}    // z-max
};




void faceCentreAndArea(const std::array<const V3*,4>& p, V3& fc, V3& S)
{
    // like OpenFOAM primitiveMesh::makeFaceCentresAndAreas
    V3 fCentre = (*p[0] + *p[1] + *p[2] + *p[3]) / 4.;

    V3 sumN, sumAc;
    double sumA = 0.;
    for (int pi=0; pi<4; ++pi)
    {
        const V3& thisPoint = *p[pi];
        const V3& nextPoint = *p[(pi+1)%4];

        V3 c = thisPoint + nextPoint + fCentre;
        V3 n = cross(nextPoint - thisPoint, fCentre - thisPoint);
        double a = norm(n);

        sumN += n;
        sumA += a;
        sumAc += a*c;
    }

    if (sumA < foamROOTVSMALL)
        fc = fCentre;
    else
        fc = (1./3.)*sumAc/sumA;

    S = 0.5*sumN;
}




double nonOrthogonalityDeg(const V3& d, const V3& S)
{
    double c = dot(d, S) / (norm(d)*norm(S) + foamROOTVSMALL);
    c = std::min(1., std::max(-1., c));
    return std::acos(c)*180./M_PI;
}




EdgeCurve EdgeCurve::line(const V3& p0, const V3& p1)
{
    EdgeCurve c;
    c.type_ = Line;
    c.pts_ = {p0, p1};
    return c;
}




EdgeCurve EdgeCurve::spline(const std::vector<V3>& allPoints)
{
    insight::assertion(
        allPoints.size()>=2,
        "a spline requires at least two points");

    EdgeCurve c;
    c.type_ = CatmullRom;
    c.pts_ = allPoints;

    // like OpenFOAM polyLine::calcParam
    c.param_.resize(allPoints.size());
    c.param_[0]=0.;
    for (size_t i=1; i<allPoints.size(); ++i)
    {
        c.param_[i] = c.param_[i-1] + norm(allPoints[i]-allPoints[i-1]);
    }
    double L = c.param_.back();
    for (size_t i=1; i<allPoints.size()-1; ++i)
    {
        c.param_[i] /= L;
    }
    c.param_.back() = 1.;

    return c;
}




EdgeCurve EdgeCurve::arc(const V3& p0, const V3& pm, const V3& p1)
{
    EdgeCurve c;
    c.type_ = Arc;
    c.pts_ = {p0, pm, p1};

    V3 a = pm - p0, b = p1 - p0;
    V3 axb = cross(a, b);
    double l2 = dot(axb, axb);
    insight::assertion(
        l2 > 0.,
        "arc edge: the three points are collinear" );

    c.centre_ = p0 + cross( dot(a,a)*b - dot(b,b)*a, axb ) / (2.*l2);
    c.radius_ = norm(p0 - c.centre_);
    c.e1_ = (p0 - c.centre_)/c.radius_;
    V3 n = axb/std::sqrt(l2);
    c.e2_ = cross(n, c.e1_);

    V3 r1 = p1 - c.centre_;
    double ang = std::atan2(dot(r1, c.e2_), dot(r1, c.e1_));
    if (ang <= 0.) ang += 2.*M_PI;
    c.angle_ = ang;

    return c;
}




V3 EdgeCurve::catmullRomPosition(int segment, double mu) const
{
    // like OpenFOAM CatmullRomSpline::position(segment, mu)
    int nSegments = int(pts_.size())-1;

    if (segment < 0) return pts_.front();
    else if (segment > nSegments) return pts_.back();

    const V3& p0 = pts_[segment];
    if (segment == nSegments) return p0;
    const V3& p1 = pts_[segment+1];

    if (mu <= 0.) return p0;
    else if (mu >= 1.) return p1;

    V3 e0, e1;
    if (segment == 0)
        e0 = 2.*p0 - p1;
    else
        e0 = pts_[segment-1];

    if (segment+1 == nSegments)
        e1 = 2.*p1 - p0;
    else
        e1 = pts_[segment+2];

    return
        0.5 *
        (
            (2.*p0)
          + mu *
            (
                (-e0 + p1)
              + mu *
                (
                    (2.*e0 - 5.*p0 + 4.*p1 - e1)
                  + mu*(-e0 + 3.*p0 - 3.*p1 + e1)
                )
            )
        );
}




V3 EdgeCurve::position(double mu) const
{
    switch (type_)
    {
    case Line:
        return pts_[0] + mu*(pts_[1]-pts_[0]);

    case Arc:
        return centre_ + radius_*( std::cos(mu*angle_)*e1_ + std::sin(mu*angle_)*e2_ );

    case CatmullRom:
        {
            if (mu < foamSMALL) return pts_.front();
            else if (mu > 1.-foamSMALL) return pts_.back();

            // like OpenFOAM polyLine::localParameter
            int segment = 1;
            while (param_[segment] < mu) ++segment;
            --segment;
            double lambda = (mu - param_[segment])/(param_[segment+1] - param_[segment]);

            return catmullRomPosition(segment, lambda);
        }
    }

    return pts_[0];
}




void lineDivide(
    const EdgeCurve& c, int nDiv, double expRatio,
    std::vector<V3>& points, std::vector<double>& divisions )
{
    // like OpenFOAM lineDivide, with a single grading section
    points.resize(nDiv+1);
    divisions.resize(nDiv+1);

    divisions[0] = 0.;
    divisions[nDiv] = 1.;

    if (std::fabs(expRatio-1.) <= foamVSMALL)
    {
        for (int i=1; i<=nDiv; ++i)
            divisions[i] = double(i)/double(nDiv);
    }
    else
    {
        double expFact = nDiv>1 ? std::pow(expRatio, 1./double(nDiv-1)) : 0.;
        for (int i=1; i<=nDiv; ++i)
            divisions[i] = (1. - std::pow(expFact, i)) / (1. - std::pow(expFact, nDiv));
    }

    for (int i=0; i<=nDiv; ++i)
        points[i] = c.position(divisions[i]);
}




BlockingModel::BlockingModel(const blockMeshBlocking& bmd)
{
    std::map<arma::mat, int> vmap;

    auto vid = [&](const arma::mat& p) -> int
    {
        auto i = vmap.find(p);
        if (i!=vmap.end()) return i->second;
        int idx = int(vertices.size());
        vmap[p]=idx;
        vertices.push_back(V3(p));
        originalVertices.push_back(p);
        return idx;
    };

    for (const auto& b: bmd.allBlocks())
    {
        BlockData bd;
        auto oc = b.orderedCorners();
        for (int i=0; i<8; ++i)
            bd.v[i] = vid(oc[i]);
        for (int i=0; i<3; ++i)
            bd.n[i] = b.resolution()[i];
        bd.grading = b.edgeGradings();
        blocks.push_back(bd);
    }

    int ei=0;
    for (const auto& e: bmd.allEdges())
    {
        EdgeData ed;
        ed.v0 = vid(e.c0());
        ed.v1 = vid(e.c1());
        ed.originalIndex = ei++;

        if (const auto* se = dynamic_cast<const SplineEdge*>(&e))
        {
            ed.kind = EdgeData::Spline;
            auto ap = se->allPoints();
            for (size_t i=1; i+1<ap.size(); ++i)
                ed.intermediate.push_back(V3(ap[i]));
            ed.geometryLabel = se->geometryLabel();
            ed.splineKeyword = se->splineKeyword();
        }
        else if (const auto* ae = dynamic_cast<const ArcEdge*>(&e))
        {
            ed.kind = EdgeData::Arc;
            ed.intermediate.push_back(V3(ae->midpoint()));
        }
        else
        {
            ed.kind = EdgeData::Fixed;
        }

        edgeIndex[ std::make_pair(std::min(ed.v0, ed.v1), std::max(ed.v0, ed.v1)) ] = int(edges.size());
        edges.push_back(ed);
    }

    for (const auto& pv: bmd.projectedVertices())
    {
        auto i = vmap.find(pv.first);
        if (i!=vmap.end())
        {
            projectedVertices.insert(i->second);
            projectedVertexLabels[i->second] = pv.second;
        }
    }

    surfaceProjections = bmd.surfaceProjections();

    for (const auto& pf: bmd.allProjectedFaces())
    {
        if (pf.face().size()!=4) continue;
        std::array<int,4> key;
        bool found=true;
        for (int k=0; k<4; ++k)
        {
            auto i = vmap.find(pf.face()[k]);
            if (i==vmap.end()) { found=false; break; }
            key[k]=i->second;
        }
        if (found)
        {
            std::sort(key.begin(), key.end());
            projectedFaceLabels[key] = pf.geometryLabel();
        }
    }
}




int BlockingModel::findEdge(int a, int b) const
{
    auto i = edgeIndex.find(std::make_pair(std::min(a,b), std::max(a,b)));
    if (i==edgeIndex.end()) return -1;
    return i->second;
}




std::vector<V3> BlockingModel::edgePoints(int e) const
{
    const auto& ed = edges[e];
    std::vector<V3> pts;
    pts.push_back(vertices[ed.v0]);
    std::copy(ed.intermediate.begin(), ed.intermediate.end(), std::back_inserter(pts));
    pts.push_back(vertices[ed.v1]);
    return pts;
}




EdgeCurve BlockingModel::curve(int a, int b) const
{
    int e = findEdge(a, b);
    if (e<0 || a==b || edges[e].kind==EdgeData::Fixed)
    {
        return EdgeCurve::line(vertices[a], vertices[b]);
    }

    auto pts = edgePoints(e);
    if (edges[e].v0 != a)
        std::reverse(pts.begin(), pts.end());

    if (edges[e].kind==EdgeData::Arc)
        return EdgeCurve::arc(pts[0], pts[1], pts[2]);
    else
        return EdgeCurve::spline(pts);
}




std::string NonOrthogonalityStatistics::summary() const
{
    return str(boost::format(
        "predicted mesh (%d cells, %d internal faces): non-orthogonality max %.1f deg, avg %.1f deg, %d faces above 70 deg, %d invalid cells")
               % nCells % nFaces % maxDeg % avgDeg % nAbove70 % nInvalidCells );
}




MeshPredictor::MeshPredictor(const blockMeshBlocking& bmd, int maxCells)
: model(bmd)
{
    init(maxCells);
}




MeshPredictor::MeshPredictor(const BlockingModel& m, int maxCells)
: model(m)
{
    init(maxCells);
}




void MeshPredictor::init(int maxCells)
{
    double nTotal = 0.;
    for (const auto& b: model.blocks)
        nTotal += double(b.n[0])*double(b.n[1])*double(b.n[2]);

    resolutionFactor_ = 1.;
    if (nTotal > double(maxCells) && nTotal>0.)
        resolutionFactor_ = std::cbrt(double(maxCells)/nTotal);

    grids_.resize(model.blocks.size());
    for (size_t bi=0; bi<model.blocks.size(); ++bi)
    {
        const auto& b = model.blocks[bi];
        auto& g = grids_[bi];
        g.ni = std::max(1, int(std::ceil(resolutionFactor_*b.n[0])));
        g.nj = std::max(1, int(std::ceil(resolutionFactor_*b.n[1])));
        g.nk = std::max(1, int(std::ceil(resolutionFactor_*b.n[2])));
    }

    faceProjection_.assign(model.blocks.size(), {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr});
    for (size_t bi=0; bi<model.blocks.size(); ++bi)
    {
        const auto& b = model.blocks[bi];

        // blockMesh renumbers the faces of collapsed blocks: not modelled
        std::set<int> distinct(b.v.begin(), b.v.end());
        if (distinct.size()<8) continue;

        for (int f=0; f<6; ++f)
        {
            std::array<int,4> key;
            for (int k=0; k<4; ++k) key[k]=b.v[hexFaces[f][k]];
            std::sort(key.begin(), key.end());
            auto pl = model.projectedFaceLabels.find(key);
            if (pl!=model.projectedFaceLabels.end())
            {
                auto sp = model.surfaceProjections.find(pl->second);
                if (sp!=model.surfaceProjections.end())
                    faceProjection_[bi][f] = &sp->second;
            }
        }
    }

    setupInterfaces();
    updateAll();
}




void MeshPredictor::setupInterfaces()
{
    const size_t nb = model.blocks.size();
    faceInterface_.assign(nb, {-1,-1,-1,-1,-1,-1});
    blockInterfaces_.assign(nb, {});
    interfaces_.clear();

    auto faceCorners = [&](int bi, int f)
    {
        std::array<std::array<int,2>,2> c;
        int a=faceAxis[f], s=faceSide[f], u, v;
        faceUV(a, u, v);
        for (int ia=0; ia<2; ++ia)
        {
            for (int ib=0; ib<2; ++ib)
            {
                int x[3];
                x[a]=s; x[u]=ia; x[v]=ib;
                c[ia][ib] = model.blocks[bi].v[cornerIndex(x[0], x[1], x[2])];
            }
        }
        return c;
    };

    auto gridDim = [&](int bi, int axis)
    {
        const auto& g = grids_[bi];
        return axis==0 ? g.ni : axis==1 ? g.nj : g.nk;
    };

    std::map<std::array<int,4>, std::vector<std::pair<int,int> > > faces;
    for (size_t bi=0; bi<nb; ++bi)
    {
        for (int f=0; f<6; ++f)
        {
            auto c = faceCorners(bi, f);
            std::array<int,4> key{ c[0][0], c[1][0], c[1][1], c[0][1] };
            std::set<int> distinct(key.begin(), key.end());
            if (distinct.size()<4)
            {
                faceInterface_[bi][f] = -2; // degenerate
            }
            else
            {
                std::sort(key.begin(), key.end());
                faces[key].push_back({int(bi), f});
            }
        }
    }

    int nUnmatched = 0;
    for (const auto& fe: faces)
    {
        const auto& bf = fe.second;
        if (bf.size()==1) continue; // boundary

        if (bf.size()!=2)
        {
            for (const auto& x: bf) faceInterface_[x.first][x.second] = -2;
            nUnmatched++;
            continue;
        }

        int A=bf[0].first, fa=bf[0].second, B=bf[1].first, fb=bf[1].second;
        auto cA = faceCorners(A, fa), cB = faceCorners(B, fb);

        // local position of vertex in A's face
        auto locA = [&](int vtx, int& ia, int& ib)
        {
            for (ia=0; ia<2; ++ia) for (ib=0; ib<2; ++ib)
                if (cA[ia][ib]==vtx) return;
            throw insight::Exception("internal error: face vertex not found");
        };

        int oa, ob, ua, ub, va, vb;
        locA(cB[0][0], oa, ob);
        locA(cB[1][0], ua, ub);
        locA(cB[0][1], va, vb);
        int dua=ua-oa, dub=ub-ob, dva=va-oa, dvb=vb-ob;

        int aA=faceAxis[fa], sA=faceSide[fa], uA, vA;
        int aB=faceAxis[fb], sB=faceSide[fb], uB, vB;
        faceUV(aA, uA, vA);
        faceUV(aB, uB, vB);
        int nuA=gridDim(A, uA), nvA=gridDim(A, vA);
        int nuB=gridDim(B, uB), nvB=gridDim(B, vB);

        bool parallel = (dub==0);
        if ( (parallel && (nuA!=nuB || nvA!=nvB))
             || (!parallel && (nuA!=nvB || nvA!=nuB)) )
        {
            faceInterface_[A][fa] = faceInterface_[B][fb] = -2;
            nUnmatched++;
            continue;
        }

        Interface itf;
        itf.blockA=A; itf.faceA=fa; itf.blockB=B; itf.faceB=fb;
        itf.signA = (sA ? 1. : -1.) * uvOrientation(aA);
        itf.familyA = aA;
        itf.signOutA = sA ? 1. : -1.;

        const auto& gA = grids_[A];
        const auto& gB = grids_[B];

        for (int cvB=0; cvB<nvB; ++cvB)
        {
            for (int cuB=0; cuB<nuB; ++cuB)
            {
                int cuA, cvA;
                if (parallel)
                {
                    cuA = dua>0 ? cuB : nuA-1-cuB;
                    cvA = dvb>0 ? cvB : nvA-1-cvB;
                }
                else
                {
                    cvA = dub>0 ? cuB : nvA-1-cuB;
                    cuA = dva>0 ? cvB : nuA-1-cvB;
                }

                int xa[3], xb[3];
                xa[aA] = sA ? gridDim(A,aA)-1 : 0; xa[uA]=cuA; xa[vA]=cvA;
                xb[aB] = sB ? gridDim(B,aB)-1 : 0; xb[uB]=cuB; xb[vB]=cvB;

                itf.cellPairs.push_back({
                    gA.cellLabel(xa[0], xa[1], xa[2]),
                    gB.cellLabel(xb[0], xb[1], xb[2]) });

                int pa[3];
                pa[aA] = sA ? gridDim(A,aA) : 0;
                std::array<int,4> fp;
                const int du[4]={0,1,1,0}, dv[4]={0,0,1,1};
                for (int k=0; k<4; ++k)
                {
                    pa[uA]=cuA+du[k]; pa[vA]=cvA+dv[k];
                    fp[k] = gA.pointLabel(pa[0], pa[1], pa[2]);
                }
                itf.facePoints.push_back(fp);

                int fa3[3];
                fa3[aA] = sA ? gridDim(A,aA) : 0; fa3[uA]=cuA; fa3[vA]=cvA;
                itf.faceLabelsA.push_back( gA.faceLabel(aA, fa3[0], fa3[1], fa3[2]) );
            }
        }

        int ii=int(interfaces_.size());
        faceInterface_[A][fa] = faceInterface_[B][fb] = ii;
        blockInterfaces_[A].push_back(ii);
        blockInterfaces_[B].push_back(ii);
        interfaces_.push_back(itf);
    }

    if (nUnmatched>0)
    {
        insight::Warning(
            "mesh prediction: %d block faces could not be matched (inconsistent resolution or non-manifold). "
            "They are treated as boundaries in the quality evaluation.", nUnmatched );
    }
}




namespace {

/**
 * like OpenFOAM-v2112 blockFaces::projectFace::project:
 * alternating projection onto the surface and restoration of the parametric distribution
 * of the face points (n.first x n.second points, index i + j*n.first)
 */
void projectFacePoints(
    const SurfaceProjection& proj, int n1, int n2, std::vector<V3>& points )
{
    auto index = [n1](int i, int j) { return i + j*n1; };

    // initial normalised edge lengths
    std::vector<double> lambdaI(points.size(), 0.), lambdaJ(points.size(), 0.);
    for (int i=1; i<n1; ++i)
    {
        for (int j=1; j<n2; ++j)
        {
            int ij=index(i,j);
            lambdaI[ij] = lambdaI[index(i-1,j)] + norm(points[ij]-points[index(i-1,j)]);
            lambdaJ[ij] = lambdaJ[index(i,j-1)] + norm(points[ij]-points[index(i,j-1)]);
        }
    }
    for (int i=1; i<n1; ++i)
    {
        int ijLast = index(i, n2-1);
        for (int j=1; j<n2; ++j)
            lambdaJ[index(i,j)] /= lambdaJ[ijLast];
    }
    for (int j=1; j<n2; ++j)
    {
        int iLastj = index(n1-1, j);
        for (int i=1; i<n1; ++i)
            lambdaI[index(i,j)] /= lambdaI[iLastj];
    }

    // linear interpolation in the actual distribution along a grid line
    auto interpolate = [](const std::vector<double>& lam, double t, const std::function<V3(int)>& pt)
    {
        const int n = int(lam.size());
        if (t <= lam[0]) return pt(0);
        if (t >= lam[n-1]) return pt(n-1);
        int i = int(std::upper_bound(lam.begin(), lam.end(), t) - lam.begin()) - 1;
        i = std::max(0, std::min(n-2, i));
        double d = lam[i+1]-lam[i];
        double s = d>0. ? (t-lam[i])/d : 0.;
        return pt(i)*(1.-s) + pt(i+1)*s;
    };

    const int maxIter = 10;
    const double relTol = 0.1;
    double initialResidual=0., iResidual=0., jResidual=0.;
    std::vector<V3> residual(points.size());
    std::vector<double> projLambdas;

    for (int iter=0; iter<maxIter; ++iter)
    {
        for (auto& p: points)
            p = proj(p);

        if ( iter>0
             && ( initialResidual < 1e-300 || (iResidual+jResidual)/initialResidual < relTol ) )
            break;

        // predict along i
        std::fill(residual.begin(), residual.end(), V3(0,0,0));
        for (int j=1; j<n2-1; ++j)
        {
            projLambdas.assign(n1, 0.);
            for (int i=1; i<n1; ++i)
                projLambdas[i] = projLambdas[i-1] + norm(points[index(i,j)]-points[index(i-1,j)]);
            for (auto& l: projLambdas) l/=projLambdas.back();

            for (int i=1; i<n1-1; ++i)
            {
                int ij=index(i,j);
                residual[ij] = interpolate(projLambdas, lambdaI[ij],
                    [&](int ii) { return points[index(ii,j)]; } ) - points[ij];
            }
        }
        iResidual=0.;
        for (size_t l=0; l<points.size(); ++l)
        {
            iResidual += norm(residual[l]);
            points[l] += residual[l];
        }

        // predict along j
        std::fill(residual.begin(), residual.end(), V3(0,0,0));
        for (int i=1; i<n1-1; ++i)
        {
            projLambdas.assign(n2, 0.);
            for (int j=1; j<n2; ++j)
                projLambdas[j] = projLambdas[j-1] + norm(points[index(i,j)]-points[index(i,j-1)]);
            for (auto& l: projLambdas) l/=projLambdas.back();

            for (int j=1; j<n2-1; ++j)
            {
                int ij=index(i,j);
                residual[ij] = interpolate(projLambdas, lambdaJ[ij],
                    [&](int jj) { return points[index(i,jj)]; } ) - points[ij];
            }
        }
        jResidual=0.;
        for (const auto& r: residual) jResidual += norm(r);

        if (iter==0) initialResidual = iResidual + jResidual;

        for (size_t l=0; l<points.size(); ++l)
            points[l] += residual[l];
    }
}

}




void MeshPredictor::applyFaceProjections(int bi)
{
    auto& g = grids_[bi];
    const int ni=g.ni, nj=g.nj, nk=g.nk;
    const auto& w = g.edgeWeights;
    const auto& fp = faceProjection_[bi];
    auto& pts = g.points;

    // face points (like blockDescriptor::facePoints), projected for the curved faces
    std::array<std::vector<V3>,6> facePts;
    auto fpl = [&](int f, int a, int b) { return f<2 ? a + b*(nj+1) : a + b*(ni+1); };
    facePts[0].resize((nj+1)*(nk+1)); facePts[1].resize((nj+1)*(nk+1));
    facePts[2].resize((ni+1)*(nk+1)); facePts[3].resize((ni+1)*(nk+1));
    facePts[4].resize((ni+1)*(nj+1)); facePts[5].resize((ni+1)*(nj+1));
    for (int k=0; k<=nk; ++k)
        for (int j=0; j<=nj; ++j)
        {
            facePts[0][fpl(0,j,k)] = pts[g.pointLabel(0,j,k)];
            facePts[1][fpl(1,j,k)] = pts[g.pointLabel(ni,j,k)];
        }
    for (int k=0; k<=nk; ++k)
        for (int i=0; i<=ni; ++i)
        {
            facePts[2][fpl(2,i,k)] = pts[g.pointLabel(i,0,k)];
            facePts[3][fpl(3,i,k)] = pts[g.pointLabel(i,nj,k)];
        }
    for (int j=0; j<=nj; ++j)
        for (int i=0; i<=ni; ++i)
        {
            facePts[4][fpl(4,i,j)] = pts[g.pointLabel(i,j,0)];
            facePts[5][fpl(5,i,j)] = pts[g.pointLabel(i,j,nk)];
        }
    for (int f=0; f<6; ++f)
    {
        if (fp[f])
        {
            int n1 = f<2 ? nj+1 : ni+1;
            int n2 = f<4 ? nk+1 : nj+1;
            projectFacePoints(*fp[f], n1, n2, facePts[f]);
        }
    }

    auto flatFaceOrEdge = [&](int i, int j, int k)
    {
        if (i==0 && !fp[0]) return true;
        if (i==ni && !fp[1]) return true;
        if (j==0 && !fp[2]) return true;
        if (j==nj && !fp[3]) return true;
        if (k==0 && !fp[4]) return true;
        if (k==nk && !fp[5]) return true;
        bool iEnd=(i==0||i==ni), jEnd=(j==0||j==nj), kEnd=(k==0||k==nk);
        return (iEnd&&jEnd) || (iEnd&&kEnd) || (jEnd&&kEnd);
    };

    // corrections from the i-, j- and k-faces (points on the curved faces last)
    for (int ii=0; ii<=ni; ++ii)
    {
        const int i=(ii+1)%(ni+1);
        for (int j=0; j<=nj; ++j)
            for (int k=0; k<=nk; ++k)
            {
                if (flatFaceOrEdge(i,j,k)) continue;
                const double
                    w0=w[0][i], w1=w[1][i], w2=w[2][i], w3=w[3][i],
                    w4=w[4][j], w5=w[5][j], w6=w[6][j], w7=w[7][j],
                    w8=w[8][k], w9=w[9][k], w10=w[10][k], w11=w[11][k];
                double wf0 = (1-w0)*(1-w4)*(1-w8) + (1-w1)*w4*(1-w11) + (1-w2)*w7*w11 + (1-w3)*(1-w7)*w8;
                double wf1 = w0*(1-w5)*(1-w9) + w1*w5*(1-w10) + w2*w5*w10 + w3*(1-w6)*w9;
                double s=wf0+wf1; wf0/=s; wf1/=s;
                pts[g.pointLabel(i,j,k)] +=
                    wf0*(facePts[0][fpl(0,j,k)] - pts[g.pointLabel(0,j,k)])
                  + wf1*(facePts[1][fpl(1,j,k)] - pts[g.pointLabel(ni,j,k)]);
            }
    }
    for (int jj=0; jj<=nj; ++jj)
    {
        const int j=(jj+1)%(nj+1);
        for (int i=0; i<=ni; ++i)
            for (int k=0; k<=nk; ++k)
            {
                if (flatFaceOrEdge(i,j,k)) continue;
                const double
                    w0=w[0][i], w1=w[1][i], w2=w[2][i], w3=w[3][i],
                    w4=w[4][j], w5=w[5][j], w6=w[6][j], w7=w[7][j],
                    w8=w[8][k], w9=w[9][k], w10=w[10][k], w11=w[11][k];
                double wf2 = (1-w4)*(1-w1)*(1-w8) + (1-w5)*w0*(1-w9) + (1-w6)*w3*w9 + (1-w7)*(1-w3)*w8;
                double wf3 = w4*(1-w1)*(1-w11) + w5*w1*(1-w10) + w6*w2*w10 + w7*(1-w2)*w11;
                double s=wf2+wf3; wf2/=s; wf3/=s;
                pts[g.pointLabel(i,j,k)] +=
                    wf2*(facePts[2][fpl(2,i,k)] - pts[g.pointLabel(i,0,k)])
                  + wf3*(facePts[3][fpl(3,i,k)] - pts[g.pointLabel(i,nj,k)]);
            }
    }
    for (int kk=0; kk<=nk; ++kk)
    {
        const int k=(kk+1)%(nk+1);
        for (int i=0; i<=ni; ++i)
            for (int j=0; j<=nj; ++j)
            {
                if (flatFaceOrEdge(i,j,k)) continue;
                const double
                    w0=w[0][i], w1=w[1][i], w2=w[2][i], w3=w[3][i],
                    w4=w[4][j], w5=w[5][j], w6=w[6][j], w7=w[7][j],
                    w8=w[8][k], w9=w[9][k], w10=w[10][k], w11=w[11][k];
                double wf4 = (1-w8)*(1-w0)*(1-w4) + (1-w9)*w0*(1-w5) + (1-w10)*w1*w5 + (1-w11)*(1-w1)*w4;
                double wf5 = w8*(1-w3)*(1-w7) + w9*w3*(1-w6) + w10*w2*w6 + w11*(1-w2)*w7;
                double s=wf4+wf5; wf4/=s; wf5/=s;
                pts[g.pointLabel(i,j,k)] +=
                    wf4*(facePts[4][fpl(4,i,j)] - pts[g.pointLabel(i,j,0)])
                  + wf5*(facePts[5][fpl(5,i,j)] - pts[g.pointLabel(i,j,nk)]);
            }
    }
}




void MeshPredictor::updateBlock(int bi)
{
    const auto& b = model.blocks[bi];
    auto& g = grids_[bi];
    const int ni=g.ni, nj=g.nj, nk=g.nk;

    std::array<const V3*,8> P;
    for (int i=0; i<8; ++i) P[i] = &model.vertices[b.v[i]];

    const V3 &p000=*P[0], &p100=*P[1], &p110=*P[2], &p010=*P[3],
             &p001=*P[4], &p101=*P[5], &p111=*P[6], &p011=*P[7];

    auto& p = g.edgePoints;
    auto& w = g.edgeWeights;
    bool curved=false;
    for (int e=0; e<12; ++e)
    {
        int va=b.v[hexEdges[e][0]], vb=b.v[hexEdges[e][1]];
        int nDiv = e<4 ? ni : e<8 ? nj : nk;
        int ei = model.findEdge(va, vb);
        if (ei>=0 && va!=vb && model.edges[ei].kind!=BlockingModel::EdgeData::Fixed)
            curved=true;
        lineDivide(model.curve(va, vb), nDiv, b.grading[e], p[e], w[e]);
    }

    g.points.resize((ni+1)*(nj+1)*(nk+1));

    // like OpenFOAM block::createPoints
    for (int k=0; k<=nk; ++k)
    {
        for (int j=0; j<=nj; ++j)
        {
            for (int i=0; i<=ni; ++i)
            {
                V3& pt = g.points[g.pointLabel(i,j,k)];

                bool isVertex = (i==0||i==ni) && (j==0||j==nj) && (k==0||k==nk);
                if (isVertex)
                {
                    pt = *P[cornerIndex(i==ni, j==nj, k==nk)];
                    continue;
                }

                const double
                    w0=w[0][i], w1=w[1][i], w2=w[2][i], w3=w[3][i],
                    w4=w[4][j], w5=w[5][j], w6=w[6][j], w7=w[7][j],
                    w8=w[8][k], w9=w[9][k], w10=w[10][k], w11=w[11][k];

                double wx1 = (1 - w0)*(1 - w4)*(1 - w8) + w0*(1 - w5)*(1 - w9);
                double wx2 = (1 - w1)*w4*(1 - w11)      + w1*w5*(1 - w10);
                double wx3 = (1 - w2)*w7*w11            + w2*w6*w10;
                double wx4 = (1 - w3)*(1 - w7)*w8       + w3*(1 - w6)*w9;
                double sumWx = wx1 + wx2 + wx3 + wx4;
                wx1/=sumWx; wx2/=sumWx; wx3/=sumWx; wx4/=sumWx;

                double wy1 = (1 - w4)*(1 - w0)*(1 - w8) + w4*(1 - w1)*(1 - w11);
                double wy2 = (1 - w5)*w0*(1 - w9)       + w5*w1*(1 - w10);
                double wy3 = (1 - w6)*w3*w9             + w6*w2*w10;
                double wy4 = (1 - w7)*(1 - w3)*w8       + w7*(1 - w2)*w11;
                double sumWy = wy1 + wy2 + wy3 + wy4;
                wy1/=sumWy; wy2/=sumWy; wy3/=sumWy; wy4/=sumWy;

                double wz1 = (1 - w8)*(1 - w0)*(1 - w4) + w8*(1 - w3)*(1 - w7);
                double wz2 = (1 - w9)*w0*(1 - w5)       + w9*w3*(1 - w6);
                double wz3 = (1 - w10)*w1*w5            + w10*w2*w6;
                double wz4 = (1 - w11)*(1 - w1)*w4      + w11*(1 - w2)*w7;
                double sumWz = wz1 + wz2 + wz3 + wz4;
                wz1/=sumWz; wz2/=sumWz; wz3/=sumWz; wz4/=sumWz;

                const V3 edgex1 = p000 + (p100 - p000)*w0;
                const V3 edgex2 = p010 + (p110 - p010)*w1;
                const V3 edgex3 = p011 + (p111 - p011)*w2;
                const V3 edgex4 = p001 + (p101 - p001)*w3;

                const V3 edgey1 = p000 + (p010 - p000)*w4;
                const V3 edgey2 = p100 + (p110 - p100)*w5;
                const V3 edgey3 = p101 + (p111 - p101)*w6;
                const V3 edgey4 = p001 + (p011 - p001)*w7;

                const V3 edgez1 = p000 + (p001 - p000)*w8;
                const V3 edgez2 = p100 + (p101 - p100)*w9;
                const V3 edgez3 = p110 + (p111 - p110)*w10;
                const V3 edgez4 = p010 + (p011 - p010)*w11;

                pt =
                (
                    wx1*edgex1 + wx2*edgex2 + wx3*edgex3 + wx4*edgex4
                  + wy1*edgey1 + wy2*edgey2 + wy3*edgey3 + wy4*edgey4
                  + wz1*edgez1 + wz2*edgez2 + wz3*edgez3 + wz4*edgez4
                )/3.;

                if (curved)
                {
                    pt +=
                        wx1*(p[0][i] - edgex1) + wx2*(p[1][i] - edgex2)
                      + wx3*(p[2][i] - edgex3) + wx4*(p[3][i] - edgex4)
                      + wy1*(p[4][j] - edgey1) + wy2*(p[5][j] - edgey2)
                      + wy3*(p[6][j] - edgey3) + wy4*(p[7][j] - edgey4)
                      + wz1*(p[8][k] - edgez1) + wz2*(p[9][k] - edgez2)
                      + wz3*(p[10][k] - edgez3) + wz4*(p[11][k] - edgez4);
                }
            }
        }
    }

    for (int f=0; f<6; ++f)
    {
        if (faceProjection_[bi][f])
        {
            applyFaceProjections(bi);
            break;
        }
    }

    // geometry of all faces, each computed once.
    // Point order (u,v): see faceUV; area vectors oriented along the positive index direction
    const int nf[3] = { (ni+1)*nj*nk, ni*(nj+1)*nk, ni*nj*(nk+1) };
    for (int fam=0; fam<3; ++fam)
    {
        g.faceCentres[fam].resize(nf[fam]);
        g.faceAreas[fam].resize(nf[fam]);
    }
    auto faceGeom = [&](int fam, int i, int j, int k, int l0, int l1, int l2, int l3)
    {
        int fl = g.faceLabel(fam, i, j, k);
        faceCentreAndArea(
            { &g.points[l0], &g.points[l1], &g.points[l2], &g.points[l3] },
            g.faceCentres[fam][fl], g.faceAreas[fam][fl] );
        g.faceAreas[fam][fl] *= uvOrientation(fam);
    };
    for (int k=0; k<nk; ++k)
        for (int j=0; j<nj; ++j)
            for (int i=0; i<=ni; ++i)
                faceGeom(0, i, j, k,
                    g.pointLabel(i,j,k), g.pointLabel(i,j+1,k),
                    g.pointLabel(i,j+1,k+1), g.pointLabel(i,j,k+1) );
    for (int k=0; k<nk; ++k)
        for (int j=0; j<=nj; ++j)
            for (int i=0; i<ni; ++i)
                faceGeom(1, i, j, k,
                    g.pointLabel(i,j,k), g.pointLabel(i+1,j,k),
                    g.pointLabel(i+1,j,k+1), g.pointLabel(i,j,k+1) );
    for (int k=0; k<=nk; ++k)
        for (int j=0; j<nj; ++j)
            for (int i=0; i<ni; ++i)
                faceGeom(2, i, j, k,
                    g.pointLabel(i,j,k), g.pointLabel(i+1,j,k),
                    g.pointLabel(i+1,j+1,k), g.pointLabel(i,j+1,k) );

    // cell centres and volumes, like OpenFOAM primitiveMesh (pyramid decomposition)
    g.cellCentres.resize(ni*nj*nk);
    g.cellVolumes.resize(ni*nj*nk);
    for (int k=0; k<nk; ++k)
    {
        for (int j=0; j<nj; ++j)
        {
            for (int i=0; i<ni; ++i)
            {
                const int fl[6] = {
                    g.faceLabel(0,i,j,k), g.faceLabel(0,i+1,j,k),
                    g.faceLabel(1,i,j,k), g.faceLabel(1,i,j+1,k),
                    g.faceLabel(2,i,j,k), g.faceLabel(2,i,j,k+1)
                };
                const V3* fc[6]; V3 S[6];
                for (int f=0; f<6; ++f)
                {
                    int fam=f/2;
                    fc[f] = &g.faceCentres[fam][fl[f]];
                    S[f] = (f%2) ? g.faceAreas[fam][fl[f]] : -g.faceAreas[fam][fl[f]]; // outward
                }

                V3 cEst = (*fc[0] + *fc[1] + *fc[2] + *fc[3] + *fc[4] + *fc[5])/6.;

                V3 sumC;
                double sumV = 0.;
                for (int f=0; f<6; ++f)
                {
                    double pyr3Vol = dot(S[f], *fc[f]-cEst);
                    sumC += pyr3Vol*(0.75*(*fc[f]) + 0.25*cEst);
                    sumV += pyr3Vol;
                }

                int ci=g.cellLabel(i,j,k);
                g.cellCentres[ci] = std::fabs(sumV) > foamVSMALL ? sumC/sumV : cEst;
                g.cellVolumes[ci] = sumV/3.;
            }
        }
    }

    computeBlockQuality(bi);
}




double MeshPredictor::thetaPow(double theta) const
{
    const double p = objectiveExponent_;
    if (p==8.)
    {
        double t2=theta*theta, t4=t2*t2;
        return t4*t4;
    }
    else if (p>=1. && p<=64. && p==std::floor(p))
    {
        // exponentiation by squaring
        unsigned int n = unsigned(p);
        double r=1., b=theta;
        while (n)
        {
            if (n&1u) r*=b;
            b*=b;
            n>>=1;
        }
        return r;
    }
    return std::pow(theta, p);
}




namespace {

// non-orthogonality of a face; returns false for degenerate (collapsed) faces
inline bool faceNonOrthogonality(const V3& cP, const V3& cN, const V3& S, double& theta)
{
    V3 d = cN - cP;
    double magS = norm(S);
    if (magS <= 1e-14*magSqr(d)) return false;
    theta = nonOrthogonalityDeg(d, S);
    return true;
}

}




void MeshPredictor::computeBlockQuality(int bi)
{
    auto& g = grids_[bi];
    const int ni=g.ni, nj=g.nj, nk=g.nk;

    double sum=0., maxT=0., theta;
    auto add = [&](const V3& cP, const V3& cN, const V3& S)
    {
        if (faceNonOrthogonality(cP, cN, S, theta))
        {
            sum += thetaPow(theta);
            maxT = std::max(maxT, theta);
        }
    };

    for (int k=0; k<nk; ++k)
        for (int j=0; j<nj; ++j)
            for (int i=1; i<ni; ++i)
                add( g.cellCentres[g.cellLabel(i-1,j,k)], g.cellCentres[g.cellLabel(i,j,k)],
                     g.faceAreas[0][g.faceLabel(0,i,j,k)] );
    for (int k=0; k<nk; ++k)
        for (int j=1; j<nj; ++j)
            for (int i=0; i<ni; ++i)
                add( g.cellCentres[g.cellLabel(i,j-1,k)], g.cellCentres[g.cellLabel(i,j,k)],
                     g.faceAreas[1][g.faceLabel(1,i,j,k)] );
    for (int k=1; k<nk; ++k)
        for (int j=0; j<nj; ++j)
            for (int i=0; i<ni; ++i)
                add( g.cellCentres[g.cellLabel(i,j,k-1)], g.cellCentres[g.cellLabel(i,j,k)],
                     g.faceAreas[2][g.faceLabel(2,i,j,k)] );

    g.sumThetaP = sum;
    g.maxTheta = maxT;

    // invalid cells: negative volume or incorrectly oriented face pyramids (like checkMesh)
    g.nInvalid = 0;
    for (int k=0; k<nk; ++k)
    {
        for (int j=0; j<nj; ++j)
        {
            for (int i=0; i<ni; ++i)
            {
                int ci = g.cellLabel(i,j,k);
                bool invalid = g.cellVolumes[ci]<=0.;
                const V3& c = g.cellCentres[ci];
                const int fl[6] = {
                    g.faceLabel(0,i,j,k), g.faceLabel(0,i+1,j,k),
                    g.faceLabel(1,i,j,k), g.faceLabel(1,i,j+1,k),
                    g.faceLabel(2,i,j,k), g.faceLabel(2,i,j,k+1)
                };
                double maxS=0.;
                for (int f=0; f<6; ++f)
                    maxS = std::max(maxS, norm(g.faceAreas[f/2][fl[f]]));
                for (int f=0; f<6 && !invalid; ++f)
                {
                    int fam=f/2;
                    const V3& S = g.faceAreas[fam][fl[f]];
                    double magS = norm(S);
                    if (magS < 1e-6*maxS) continue; // collapsed face: not present in the mesh
                    V3 d = g.faceCentres[fam][fl[f]] - c;
                    double pyr = (f%2 ? 1. : -1.)*dot(S, d); // outward area vector
                    if (pyr < -1e-9*magS*norm(d)) invalid=true;
                }
                if (invalid) g.nInvalid++;
            }
        }
    }
}




void MeshPredictor::setObjectiveExponent(double p)
{
    if (p!=objectiveExponent_)
    {
        objectiveExponent_ = p;
        for (size_t bi=0; bi<grids_.size(); ++bi)
            computeBlockQuality(int(bi));
    }
}




void MeshPredictor::updateAll()
{
    for (size_t bi=0; bi<model.blocks.size(); ++bi)
        updateBlock(int(bi));
}




void MeshPredictor::forEachFace(
    const std::set<int>& blocks,
    const std::function<void(const std::array<V3,4>&, double, int)>& f ) const
{
    std::vector<int> bl(blocks.begin(), blocks.end());
    if (bl.empty())
    {
        for (size_t bi=0; bi<grids_.size(); ++bi) bl.push_back(int(bi));
    }

    double theta;
    std::set<int> itfs;
    for (int bi: bl)
    {
        const auto& g = grids_[bi];
        const int ni=g.ni, nj=g.nj, nk=g.nk;
        auto P = [&](int i, int j, int k) { return g.points[g.pointLabel(i,j,k)]; };

        // x-faces
        for (int k=0; k<nk; ++k)
            for (int j=0; j<nj; ++j)
                for (int i=1; i<ni; ++i)
                    if (faceNonOrthogonality(
                            g.cellCentres[g.cellLabel(i-1,j,k)], g.cellCentres[g.cellLabel(i,j,k)],
                            g.faceAreas[0][g.faceLabel(0,i,j,k)], theta ))
                        f( { P(i,j,k), P(i,j+1,k), P(i,j+1,k+1), P(i,j,k+1) }, theta, bi );
        // y-faces
        for (int k=0; k<nk; ++k)
            for (int j=1; j<nj; ++j)
                for (int i=0; i<ni; ++i)
                    if (faceNonOrthogonality(
                            g.cellCentres[g.cellLabel(i,j-1,k)], g.cellCentres[g.cellLabel(i,j,k)],
                            g.faceAreas[1][g.faceLabel(1,i,j,k)], theta ))
                        f( { P(i,j,k), P(i+1,j,k), P(i+1,j,k+1), P(i,j,k+1) }, theta, bi );
        // z-faces
        for (int k=1; k<nk; ++k)
            for (int j=0; j<nj; ++j)
                for (int i=0; i<ni; ++i)
                    if (faceNonOrthogonality(
                            g.cellCentres[g.cellLabel(i,j,k-1)], g.cellCentres[g.cellLabel(i,j,k)],
                            g.faceAreas[2][g.faceLabel(2,i,j,k)], theta ))
                        f( { P(i,j,k), P(i+1,j,k), P(i+1,j+1,k), P(i,j+1,k) }, theta, bi );

        itfs.insert(blockInterfaces_[bi].begin(), blockInterfaces_[bi].end());
    }

    for (int ii: itfs)
    {
        const auto& itf = interfaces_[ii];
        const auto& gA = grids_[itf.blockA];
        const auto& gB = grids_[itf.blockB];
        for (size_t k=0; k<itf.cellPairs.size(); ++k)
        {
            if (faceNonOrthogonality(
                    gA.cellCentres[itf.cellPairs[k].first], gB.cellCentres[itf.cellPairs[k].second],
                    itf.signOutA*gA.faceAreas[itf.familyA][itf.faceLabelsA[k]], theta ))
            {
                const auto& fp = itf.facePoints[k];
                f( { gA.points[fp[0]], gA.points[fp[1]], gA.points[fp[2]], gA.points[fp[3]] },
                   theta, itf.blockA );
            }
        }
    }
}




double MeshPredictor::objective(const std::set<int>& blocks, double p, int& nInvalidCells)
{
    setObjectiveExponent(p);

    double sum=0.;
    nInvalidCells=0;

    std::vector<int> itfs;
    auto addBlock = [&](int bi)
    {
        sum += grids_[bi].sumThetaP;
        nInvalidCells += grids_[bi].nInvalid;
        itfs.insert(itfs.end(), blockInterfaces_[bi].begin(), blockInterfaces_[bi].end());
    };
    if (blocks.empty())
        for (size_t bi=0; bi<grids_.size(); ++bi) addBlock(int(bi));
    else
        for (int bi: blocks) addBlock(bi);

    std::sort(itfs.begin(), itfs.end());
    itfs.erase(std::unique(itfs.begin(), itfs.end()), itfs.end());

    double theta;
    for (int ii: itfs)
    {
        const auto& itf = interfaces_[ii];
        const auto& gA = grids_[itf.blockA];
        const auto& gB = grids_[itf.blockB];
        const auto& SA = gA.faceAreas[itf.familyA];
        for (size_t k=0; k<itf.cellPairs.size(); ++k)
        {
            if (faceNonOrthogonality(
                    gA.cellCentres[itf.cellPairs[k].first], gB.cellCentres[itf.cellPairs[k].second],
                    itf.signOutA*SA[itf.faceLabelsA[k]], theta ))
                sum += thetaPow(theta);
        }
    }

    return sum>0. ? std::pow(sum, 1./p) : 0.;
}




NonOrthogonalityStatistics MeshPredictor::statistics() const
{
    NonOrthogonalityStatistics s;
    double sum=0.;
    forEachFace(
        {},
        [&](const std::array<V3,4>&, double theta, int)
        {
            s.maxDeg = std::max(s.maxDeg, theta);
            sum += std::cos(theta*M_PI/180.);
            s.nFaces++;
            if (theta>70.) s.nAbove70++;
        } );
    // average as reported by checkMesh: angle of the mean cosine
    if (s.nFaces>0) s.avgDeg = std::acos(std::min(1., sum/double(s.nFaces)))*180./M_PI;

    for (const auto& g: grids_)
    {
        s.nCells += int(g.cellVolumes.size());
        s.nInvalidCells += g.nInvalid;
    }
    return s;
}




} // namespace bmd
} // namespace insight
