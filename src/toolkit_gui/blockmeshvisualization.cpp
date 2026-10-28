#include "blockmeshvisualization.h"

#include "GeomAPI_Interpolate.hxx"
#include "BRepBuilderAPI_MakeEdge.hxx"
#include "BRepAdaptor_Curve.hxx"
#include "BRep_Builder.hxx"
#include "GCPnts_AbscissaPoint.hxx"
#include "GC_MakeArcOfCircle.hxx"

#include "boost/algorithm/string.hpp"

#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkSTLReader.h"
#include "vtkOBJReader.h"
#include "vtkPolyDataReader.h"
#include "vtkCellLocator.h"

#include "base/exception.h"
#include "base/spatialtransformation.h"
#include "occinclude.h"
#include "occtools.h"
#include "cadparametersetvisualizer.h"
#include "cadfeatures/importsolidmodel.h"
#include "cadpostprocactions/featureannotation.h"

#include "BRepBuilderAPI_Transform.hxx"
#include "TopoDS_Compound.hxx"

namespace insight
{


namespace bmd
{



namespace
{

/**
 * normalized arc length coordinate of the points of a polyline
 */
std::vector<double> relativeDistances(const std::vector<gp_Pnt>& pts)
{
    std::vector<double> s(pts.size(), 0.);
    for (size_t k=1; k<pts.size(); ++k)
        s[k]=s[k-1]+pts[k].Distance(pts[k-1]);
    if (s.size()>1 && s.back()<=0.)
    {
        for (size_t k=0; k<s.size(); ++k)
            s[k]=double(k)/double(s.size()-1);
    }
    else
    {
        for (auto& si: s)
            si/=s.back();
    }
    return s;
}


/**
 * positions on the polyline at the given normalized arc length coordinates
 */
std::vector<gp_Pnt> resampled(
    const std::vector<gp_Pnt>& pl,
    const std::vector<double>& targets )
{
    auto s=relativeDistances(pl);
    std::vector<gp_Pnt> res;
    size_t k=1;
    for (double t: targets)
    {
        while (k<pl.size()-1 && s[k]<t) ++k;
        double ds=s[k]-s[k-1];
        double w= ds>0. ? std::min(1., std::max(0., (t-s[k-1])/ds)) : 0.;
        res.push_back(gp_Pnt( (1.-w)*pl[k-1].XYZ() + w*pl[k].XYZ() ));
    }
    return res;
}


/**
 * number of correction iterations in projections
 */
const int nProjectionCorrectionIters = 5;



/**
 * the edges of a hex in OpenFOAM order (etc/cellModels)
 */
const int blockEdges[12][2] = {
    {0,1}, {3,2}, {7,6}, {4,5},
    {0,3}, {1,2}, {5,6}, {4,7},
    {0,4}, {1,5}, {2,6}, {3,7}
};


/**
 * index of the hex edge between the corners a and b (in any direction)
 */
int blockEdgeIndex(int a, int b)
{
    for (int k=0; k<12; ++k)
    {
        if ( (blockEdges[k][0]==a && blockEdges[k][1]==b)
             || (blockEdges[k][0]==b && blockEdges[k][1]==a) )
            return k;
    }
    throw insight::Exception("internal error: no hex edge between corners %d and %d", a, b);
}


/**
 * the faces of a hex in OpenFOAM order (etc/cellModels)
 */
const int ofHexFaces[6][4] = {
    {0, 4, 7, 3}, // x-min
    {1, 2, 6, 5}, // x-max
    {0, 1, 5, 4}, // y-min
    {3, 7, 6, 2}, // y-max
    {0, 3, 2, 1}, // z-min
    {4, 5, 6, 7}  // z-max
};


/**
 * centre and area normal of a quad face,
 * computed like Foam::face::centre and Foam::face::areaNormal
 */
void faceCentreAndAreaNormal(
    const std::array<gp_XYZ,4>& p,
    gp_XYZ& centre, gp_XYZ& areaNormal )
{
    gp_XYZ cp(0,0,0);
    for (const auto& x: p) cp+=x;
    cp/=4.;

    double sumA=0;
    gp_XYZ sumAc(0,0,0), sumN(0,0,0);
    for (int i=0; i<4; ++i)
    {
        const gp_XYZ& next=p[(i+1)%4];
        gp_XYZ ttc = p[i] + next + cp;
        gp_XYZ n = 0.5*(next-p[i]).Crossed(cp-p[i]);
        double a = n.Modulus();
        sumA+=a;
        sumAc+=a*ttc;
        sumN+=n;
    }
    centre = sumA>1e-300 ? sumAc/(3.*sumA) : cp;
    areaNormal = sumN;
}


/**
 * the faces of a block, which are not outward-pointing,
 * with the same criterion as Foam::blockDescriptor::check
 */
std::vector<int> inwardPointingFaces(const std::array<gp_XYZ,8>& v)
{
    std::array<gp_XYZ,6> fc, fn;
    for (int fi=0; fi<6; ++fi)
    {
        std::array<gp_XYZ,4> fp;
        for (int k=0; k<4; ++k) fp[k]=v[ofHexFaces[fi][k]];
        faceCentreAndAreaNormal(fp, fc[fi], fn[fi]);
    }

    // block centre like Foam::cellModel::centre:
    // pyramid volume weighted average of the pyramid centres
    gp_XYZ cEst(0,0,0);
    for (const auto& x: v) cEst+=x;
    cEst/=8.;
    double sumV=0;
    gp_XYZ sumVc(0,0,0);
    for (int fi=0; fi<6; ++fi)
    {
        double pyrVol = fn[fi].Dot(fc[fi]-cEst)/3.;
        gp_XYZ pyrCentre = 0.75*fc[fi] + 0.25*cEst;
        sumV+=pyrVol;
        sumVc+=pyrVol*pyrCentre;
    }
    gp_XYZ blockCentre = sumVc/(sumV + 1e-300);

    std::vector<int> inward;
    for (int fi=0; fi<6; ++fi)
    {
        if ( fn[fi].Modulus() > insight::SMALL
             && (fc[fi]-blockCentre).Dot(fn[fi]) <= 0. )
        {
            inward.push_back(fi);
        }
    }
    return inward;
}

}




/**
 * @brief The ProjectionSurface struct
 * a triangulated surface, onto which points are projected
 * (corresponds to a triSurfaceMesh geometry in blockMesh)
 */
struct blockMeshVisualization::ProjectionSurface
{
    vtkSmartPointer<vtkPolyData> surface;
    vtkSmartPointer<vtkCellLocator> locator;

    ProjectionSurface(vtkSmartPointer<vtkPolyData> s)
        : surface(s),
          locator(vtkSmartPointer<vtkCellLocator>::New())
    {
        locator->SetDataSet(surface);
        locator->BuildLocator();
    }

    gp_Pnt nearest(const gp_Pnt& p) const
    {
        double x[3]={p.X(), p.Y(), p.Z()}, cp[3];
        vtkIdType cellId;
        int subId;
        double dist2;
        locator->FindClosestPoint(x, cp, cellId, subId, dist2);
        if (cellId<0) return p;
        return gp_Pnt(cp[0], cp[1], cp[2]);
    }
};




const blockMeshVisualization::ProjectionSurface*
blockMeshVisualization::projectionSurface(const std::string& label)
{
    auto i=surfaces_.find(label);
    if (i!=surfaces_.end())
        return i->second.get();

    std::shared_ptr<ProjectionSurface> surf;

    auto geo=std::find_if(
        bm_.allGeometry().begin(), bm_.allGeometry().end(),
        [&](const Geometry& g) { return g==label; } );

    if (geo==bm_.allGeometry().end())
    {
        insight::Warning(
            "The geometry \"%s\" for projection is not defined in the blocking."
            " Projection is not shown in the visualization.", label.c_str() );
    }
    else
    {
        // triSurfaceMesh files are searched in constant/triSurface by blockMesh
        namespace fs=boost::filesystem;
        fs::path fn;
        for (const auto& candidate: {
                 geo->fileName(),
                 geometryDir_/geo->fileName(),
                 geometryDir_/"constant"/"triSurface"/geo->fileName() })
        {
            if (fs::exists(candidate)) { fn=candidate; break; }
        }

        if (fn.empty())
        {
            insight::Warning(
                "The file %s of geometry \"%s\" was not found."
                " Projection is not shown in the visualization.",
                geo->fileName().string().c_str(), label.c_str() );
        }
        else
        {
            vtkSmartPointer<vtkPolyData> pd;
            auto ext=fn.extension().string();
            boost::algorithm::to_lower(ext);
            if (ext==".stl" || ext==".stlb")
            {
                auto r=vtkSmartPointer<vtkSTLReader>::New();
                r->SetFileName(fn.string().c_str());
                r->Update();
                pd=r->GetOutput();
            }
            else if (ext==".obj")
            {
                auto r=vtkSmartPointer<vtkOBJReader>::New();
                r->SetFileName(fn.string().c_str());
                r->Update();
                pd=r->GetOutput();
            }
            else if (ext==".vtk")
            {
                auto r=vtkSmartPointer<vtkPolyDataReader>::New();
                r->SetFileName(fn.string().c_str());
                r->Update();
                pd=r->GetOutput();
            }

            if (pd && pd->GetNumberOfCells()>0)
            {
                surf=std::make_shared<ProjectionSurface>(pd);
            }
            else
            {
                insight::Warning(
                    "Could not read a surface from file %s of geometry \"%s\"."
                    " Projection is not shown in the visualization.",
                    fn.string().c_str(), label.c_str() );
            }
        }
    }

    surfaces_[label]=surf;
    return surf.get();
}




const blockMeshVisualization::ProjectionSurface*
blockMeshVisualization::edgeProjectionSurface(const arma::mat& p0, const arma::mat& p1)
{
    const auto* e = bm_.edgeBetween(p0, p1);
    if (const auto* pe = dynamic_cast<const ProjectedEdge*>(e))
        return projectionSurface(pe->geometryLabel());
    if (const auto* se = dynamic_cast<const SplineEdge*>(e))
        if (!se->geometryLabel().empty())
            return projectionSurface(se->geometryLabel());
    return nullptr;
}




gp_Pnt blockMeshVisualization::vertexPosition(const Point& p)
{
    gp_Pnt x=cad::to_Pnt(p);
    auto pv=bm_.projectedVertices().find(p);
    if (pv!=bm_.projectedVertices().end())
    {
        if (auto *s=projectionSurface(pv->second))
            x=s->nearest(x);
    }
    return x;
}




std::vector<gp_Pnt> blockMeshVisualization::projectedLine(
    const gp_Pnt& x0, const gp_Pnt& x1,
    const ProjectionSurface& surf,
    const std::vector<double>& lambda )
{
    // start from straight line, project and correct the distribution
    // iteratively (like the projectEdge of blockMesh)
    int n=lambda.size()-1;
    std::vector<gp_Pnt> pts;
    for (double l: lambda)
        pts.push_back(gp_Pnt( (1.-l)*x0.XYZ() + l*x1.XYZ() ));

    for (int iter=0; iter<nProjectionCorrectionIters; ++iter)
    {
        for (int k=1; k<n; ++k)
            pts[k]=surf.nearest(pts[k]);
        pts=resampled(pts, lambda);
    }
    for (int k=1; k<n; ++k)
        pts[k]=surf.nearest(pts[k]);
    pts.front()=x0;
    pts.back()=x1;
    return pts;
}




bool blockMeshVisualization::EdgeKeyLess::operator()(
    const std::pair<Point,Point>& e1,
    const std::pair<Point,Point>& e2 ) const
{
    std::less<arma::mat> lt;
    auto normalized = [&](const std::pair<Point,Point>& e)
    {
        return lt(e.second, e.first) ?
                   std::make_pair(&e.second, &e.first)
                 : std::make_pair(&e.first, &e.second);
    };
    auto n1=normalized(e1), n2=normalized(e2);

    if (lt(*n1.first, *n2.first)) return true;
    if (lt(*n2.first, *n1.first)) return false;
    return lt(*n1.second, *n2.second);
}




blockMeshVisualization::TransformFunction
    blockMeshVisualization::NoTransformFunction =
        [](const arma::mat& p) { return p; };




TopoDS_Edge blockMeshVisualization::buildEdge(
    const arma::mat& p0, const arma::mat& p1,
    const PointMapping& map )
{
    // all geometry (incl. projection) is evaluated in blockMesh coordinates,
    // the defining points of the curves are mapped afterwards
    gp_Pnt x0=vertexPosition(p0), x1=vertexPosition(p1);

    auto interpolate = [&map](const std::vector<gp_Pnt>& pts)
    {
        Handle_TColgp_HArray1OfPnt pts_col = new TColgp_HArray1OfPnt( 1, pts.size() );
        for ( size_t j=0; j<pts.size(); j++ )
        {
            pts_col->SetValue ( j+1, map(pts[j]) );
        }
        GeomAPI_Interpolate splbuilder ( pts_col, false, 1e-6 );
        splbuilder.Perform();
        Handle_Geom_BSplineCurve crv=splbuilder.Curve();
        return BRepBuilderAPI_MakeEdge ( crv, crv->FirstParameter(), crv->LastParameter() ).Edge();
    };

    TopoDS_Edge edg;
    if (x0.Distance(x1)>insight::SMALL)
    {
        if (const auto* e = bm_.edgeBetween(p0, p1))
        {
            bool fwd = arma::norm(e->c0()-p0, 2)<SMALL;

            if (auto* se = dynamic_cast<const bmd::SplineEdge*>(e))
            {
                std::vector<gp_Pnt> pts;
                for (const auto& p: se->allPoints())
                    pts.push_back(cad::to_Pnt(p));
                if (!fwd) std::reverse(pts.begin(), pts.end());
                // end points are possibly projected vertices
                pts.front()=x0;
                pts.back()=x1;

                if (auto *surf=edgeProjectionSurface(p0, p1))
                {
                    // projected spline: project the spline curve onto the surface
                    std::vector<gp_Pnt> spts;
                    {
                        Handle_TColgp_HArray1OfPnt pts_col = new TColgp_HArray1OfPnt( 1, pts.size() );
                        for ( size_t j=0; j<pts.size(); j++ ) pts_col->SetValue ( j+1, pts[j] );
                        GeomAPI_Interpolate splbuilder ( pts_col, false, 1e-6 );
                        splbuilder.Perform();
                        auto crv=splbuilder.Curve();
                        const int nSample=32;
                        for (int k=0; k<=nSample; ++k)
                        {
                            auto x=crv->Value(crv->FirstParameter()+double(k)/double(nSample)
                                                *(crv->LastParameter()-crv->FirstParameter()));
                            spts.push_back( (k==0||k==nSample) ? x : surf->nearest(x) );
                        }
                    }
                    spts.front()=x0;
                    spts.back()=x1;
                    edg=interpolate(spts);
                }
                else
                {
                    edg=interpolate(pts);
                }
            }
            else if (auto* ae = dynamic_cast<const bmd::ArcEdge*>(e))
            {
                auto crv = GC_MakeArcOfCircle(
                    map(x0), map(cad::to_Pnt(ae->midpoint())), map(x1) ).Value();
                edg=BRepBuilderAPI_MakeEdge(crv).Edge();
            }
            else if (dynamic_cast<const bmd::ProjectedEdge*>(e))
            {
                if (auto *surf=edgeProjectionSurface(p0, p1))
                {
                    const int nSample=32;
                    std::vector<double> l;
                    for (int k=0; k<=nSample; ++k) l.push_back(double(k)/double(nSample));
                    edg=interpolate(projectedLine(x0, x1, *surf, l));
                }
            }
        }

        if (edg.IsNull())
        {
            edg=BRepBuilderAPI_MakeEdge(map(x0), map(x1)).Edge();
        }
    }

    return edg;
}




TopoDS_Edge blockMeshVisualization::edge(
    const arma::mat& p0, const arma::mat& p1 )
{
    auto key=std::make_pair(p0, p1);
    auto existing=edges_.find(key);
    if (existing!=edges_.end())
        return existing->second;

    auto edg=buildEdge(p0, p1, [](const gp_Pnt& x) { return x; });
    edges_[key]=edg;
    return edg;
}




gp_Pnt blockMeshVisualization::transformed(const gp_Pnt& x) const
{
    return cad::to_Pnt(tf_(vec3(x)));
}




std::vector<gp_Pnt> blockMeshVisualization::edgePoints(
    const arma::mat& p0, const arma::mat& p1,
    int n, double grading )
{
    // relative positions of the points along the edge (as blockMesh does)
    std::vector<double> lambda(n+1);
    double r = (n>1) ? std::pow(grading, 1./double(n-1)) : 1.;
    for (int k=0; k<=n; ++k)
    {
        if (std::fabs(r-1.)<1e-9)
            lambda[k]=double(k)/double(n);
        else
            lambda[k]=(std::pow(r, k)-1.)/(std::pow(r, n)-1.);
    }

    gp_Pnt x0=vertexPosition(p0), x1=vertexPosition(p1);

    std::vector<gp_Pnt> pts(n+1, x0);
    auto e = edge(p0, p1);
    if (!e.IsNull())
    {
        BRepAdaptor_Curve c(e);
        double t0=c.FirstParameter(), t1=c.LastParameter();
        if (c.Value(t0).Distance(x0) > c.Value(t1).Distance(x0))
            std::swap(t0, t1); // edge is stored in reverse direction

        double L=GCPnts_AbscissaPoint::Length(c);
        double dir = t1>t0 ? 1. : -1.;
        for (int k=1; k<n; ++k)
        {
            GCPnts_AbscissaPoint ap(c, dir*lambda[k]*L, t0);
            pts[k] = ap.IsDone() ?
                         c.Value(ap.Parameter())
                       : c.Value(t0+lambda[k]*(t1-t0));
        }
        pts[n]=x1;

        // the curve of a projected edge is only an approximation:
        // put the points exactly onto the surface
        if (auto *surf=edgeProjectionSurface(p0, p1))
        {
            for (int k=1; k<n; ++k)
                pts[k]=surf->nearest(pts[k]);
        }
    }
    return pts;
}




namespace
{

/**
 * the faces of a hex block, described by its edges (pairs of local corner indices)
 * bottom and top run in u-direction, left and right in v-direction.
 * The indices of the edge gradings refer to Block::edgeGradings()
 * and the directions to Block::resolution().
 */
struct HexFace
{
    int b0, b1, gb;
    int t0, t1, gt;
    int l0, l1, gl;
    int r0, r1, gr;
    int du, dv;
};

const HexFace hexFaces[6] = {
    { 0,1,0,  3,2,1,  0,3,4,  1,2,5,   0,1 },
    { 4,5,3,  7,6,2,  4,7,7,  5,6,6,   0,1 },
    { 0,1,0,  4,5,3,  0,4,8,  1,5,9,   0,2 },
    { 3,2,1,  7,6,2,  3,7,11, 2,6,10,  0,2 },
    { 0,3,4,  4,7,7,  0,4,8,  3,7,11,  1,2 },
    { 1,2,5,  5,6,6,  1,5,9,  2,6,10,  1,2 }
};




/**
 * transfinite interpolation of the interior points from the edge points
 */
cad::StructuredSurfacePatch coonsPatch(
    const std::vector<gp_Pnt>& B, const std::vector<gp_Pnt>& T,
    const std::vector<gp_Pnt>& L, const std::vector<gp_Pnt>& R )
{
    cad::StructuredSurfacePatch p;
    p.nu=B.size()-1;
    p.nv=L.size()-1;
    p.pts.resize((p.nu+1)*(p.nv+1));

    auto ub=relativeDistances(B), ut=relativeDistances(T);
    auto vl=relativeDistances(L), vr=relativeDistances(R);

    const gp_XYZ P00=B.front().XYZ(), P10=B.back().XYZ(),
                 P01=T.front().XYZ(), P11=T.back().XYZ();

    for (int j=0; j<=p.nv; ++j)
    {
        for (int i=0; i<=p.nu; ++i)
        {
            // local coordinates, which take differing distributions
            // on opposite edges into account
            double du=ut[i]-ub[i], dv=vr[j]-vl[j];
            double u=(ub[i]+vl[j]*du)/(1.-dv*du);
            double v=vl[j]+u*dv;

            gp_XYZ x =
                (1.-v)*B[i].XYZ() + v*T[i].XYZ()
              + (1.-u)*L[j].XYZ() + u*R[j].XYZ()
              - ( (1.-u)*(1.-v)*P00 + u*(1.-v)*P10
                 + (1.-u)*v*P01 + u*v*P11 );

            p.pts[j*(p.nu+1)+i]=gp_Pnt(x);
        }
    }
    return p;
}


}




void blockMeshVisualization::projectFace(
    cad::StructuredSurfacePatch& p,
    const ProjectionSurface& surf )
{
    // Like the projectFace of blockMesh: the face points are projected
    // and the point distribution along the grid lines is corrected iteratively,
    // so that it matches the distribution of the unprojected face.
    // The boundary points are taken from the edges. They are on the surface,
    // if the edges are projected as well.

    auto row=[&](int j)
    {
        std::vector<gp_Pnt> r;
        for (int i=0; i<=p.nu; ++i) r.push_back(p.pt(i,j));
        return r;
    };
    auto col=[&](int i)
    {
        std::vector<gp_Pnt> c;
        for (int j=0; j<=p.nv; ++j) c.push_back(p.pt(i,j));
        return c;
    };
    auto set=[&](int i, int j, const gp_Pnt& x)
    {
        p.pts[j*(p.nu+1)+i]=x;
    };
    auto projectInterior=[&]()
    {
        for (int j=1; j<p.nv; ++j)
            for (int i=1; i<p.nu; ++i)
                set(i, j, surf.nearest(p.pt(i,j)));
    };

    std::vector<std::vector<double> > tu, tv;
    for (int j=0; j<=p.nv; ++j) tu.push_back(relativeDistances(row(j)));
    for (int i=0; i<=p.nu; ++i) tv.push_back(relativeDistances(col(i)));

    for (int iter=0; iter<nProjectionCorrectionIters; ++iter)
    {
        projectInterior();
        for (int j=1; j<p.nv; ++j)
        {
            auto r=resampled(row(j), tu[j]);
            for (int i=1; i<p.nu; ++i) set(i, j, r[i]);
        }
        for (int i=1; i<p.nu; ++i)
        {
            auto c=resampled(col(i), tv[i]);
            for (int j=1; j<p.nv; ++j) set(i, j, c[j]);
        }
    }
    projectInterior();
}




blockMeshVisualization::blockMeshVisualization(
    const bmd::blockMeshBlocking& bm,
    TransformFunction tf,
    int maxPreviewFaces,
    const boost::filesystem::path& geometryDir )
    : bm_(bm),
      tf_(tf),
      geometryDir_(geometryDir),
      previewResolutionFactor_(1.)
{
    // block edges
    for (const auto& bl: bm.allBlocks())
    {
        auto c=bl.orderedCorners();

        BlockVisualization bv;
        BRep_Builder bb;
        bb.MakeCompound(bv.edges);
        for (int k=0; k<12; ++k)
        {
            auto e = buildEdge(
                c[blockEdges[k][0]], c[blockEdges[k][1]],
                [this](const gp_Pnt& x) { return transformed(x); } );
            bv.displayEdges[k]=e;
            if (!e.IsNull())
                bb.Add(bv.edges, e);
        }

        bv.centre=vec3(0,0,0);
        for (const auto& ci: c)
            bv.centre+=vec3(transformed(vertexPosition(ci)));
        bv.centre/=double(c.size());

        // check orientation like blockMesh does
        // (projected vertices, blockMesh coordinates)
        std::array<gp_XYZ,8> v;
        for (int k=0; k<8; ++k)
            v[k]=vertexPosition(c[k]).XYZ();
        bv.inwardFaces=inwardPointingFaces(v);
        bv.insideOut = (bv.inwardFaces.size()==6);

        blocks_.push_back(bv);
    }

    if (maxPreviewFaces<=0)
        return;

    // collect the block faces. Faces, which are shared between neighbouring blocks,
    // are computed only once, but displayed in each block,
    // so that each block shows the mesh on all of its faces.
    struct FaceRef { int block, face; std::vector<int> displayedInBlocks; std::string projectionGeometry; };
    std::vector<FaceRef> faces;
    int nFaceDisplays=0;
    {
        auto lessPoint = std::less<arma::mat>();
        auto lessFace = [&](const std::vector<Point>& f1, const std::vector<Point>& f2)
        {
            return std::lexicographical_compare(
                f1.begin(), f1.end(), f2.begin(), f2.end(), lessPoint );
        };
        std::map<std::vector<Point>, int, decltype(lessFace)> faceIndex(lessFace);

        std::map<std::vector<Point>, std::string, decltype(lessFace)> projectedFaces(lessFace);
        for (const auto& pf: bm.allProjectedFaces())
        {
            auto key=pf.face();
            std::sort(key.begin(), key.end(), lessPoint);
            projectedFaces[key]=pf.geometryLabel();
        }

        for (int ib=0; ib<int(bm.allBlocks().size()); ++ib)
        {
            auto c=bm.allBlocks()[ib].orderedCorners();
            for (int fi=0; fi<6; ++fi)
            {
                const auto& hf=hexFaces[fi];
                std::vector<Point> key{ c[hf.b0], c[hf.b1], c[hf.t0], c[hf.t1] };
                std::sort(key.begin(), key.end(), lessPoint);
                auto i=faceIndex.find(key);
                if (i==faceIndex.end())
                {
                    faceIndex.insert({key, int(faces.size())});
                    auto pf=projectedFaces.find(key);
                    faces.push_back({ib, fi, {ib},
                                     pf!=projectedFaces.end() ? pf->second : std::string() });
                }
                else
                {
                    faces[i->second].displayedInBlocks.push_back(ib);
                }
                ++nFaceDisplays;
            }
        }
    }

    // scale the resolution uniformly, so that adjacent blocks remain conformal
    auto scaledResolution = [](int n, double f)
    {
        return std::max(1, int(std::round(double(n)*f)));
    };
    auto nPreviewFaces = [&](double f)
    {
        long nf=0;
        for (const auto& fr: faces)
        {
            const auto& res=bm.allBlocks()[fr.block].resolution();
            const auto& hf=hexFaces[fr.face];
            nf+=long(fr.displayedInBlocks.size())
                *long(scaledResolution(res[hf.du], f))*long(scaledResolution(res[hf.dv], f));
        }
        return nf;
    };

    double f=1.;
    long nf=nPreviewFaces(f);
    if (nf>maxPreviewFaces)
    {
        f=std::sqrt(double(maxPreviewFaces)/double(nf));
        while ( (nf=nPreviewFaces(f))>maxPreviewFaces && nf>long(nFaceDisplays) )
        {
            f*=0.95;
        }
    }
    previewResolutionFactor_=f;

    // create the mesh on the faces
    for (const auto& fr: faces)
    {
        const auto& bl=bm.allBlocks()[fr.block];
        const auto& hf=hexFaces[fr.face];
        auto c=bl.orderedCorners();
        auto g=bl.edgeGradings();
        int nu=scaledResolution(bl.resolution()[hf.du], f);
        int nv=scaledResolution(bl.resolution()[hf.dv], f);

        auto patch = coonsPatch(
                edgePoints(c[hf.b0], c[hf.b1], nu, g[hf.gb]),
                edgePoints(c[hf.t0], c[hf.t1], nu, g[hf.gt]),
                edgePoints(c[hf.l0], c[hf.l1], nv, g[hf.gl]),
                edgePoints(c[hf.r0], c[hf.r1], nv, g[hf.gr]) );

        if (!fr.projectionGeometry.empty())
        {
            if (auto *surf=projectionSurface(fr.projectionGeometry))
                projectFace(patch, *surf);
        }

        for (auto& x: patch.pts)
            x=transformed(x);

        // identical points in all blocks avoid display artifacts on coincident faces
        for (int ib: fr.displayedInBlocks)
            blocks_[ib].meshPreview.push_back(patch);
    }
}



bool blockMeshVisualization::hasInvalidBlocks() const
{
    return std::any_of(
        blocks_.begin(), blocks_.end(),
        [](const BlockVisualization& b) { return b.isInvalid(); } );
}




std::string blockMeshVisualization::blockProblemDescription(size_t i) const
{
    const auto& b=blocks_[i];
    std::ostringstream os;
    if (b.insideOut)
    {
        os << "Block " << i << " is inside-out";
    }
    else if (b.isInvalid())
    {
        os << "Block " << i << " has inward-pointing faces";
        for (int fi: b.inwardFaces)
        {
            os << " (";
            for (int k=0; k<4; ++k) os << (k?" ":"") << ofHexFaces[fi][k];
            os << ")";
        }
    }
    return os.str();
}




std::string blockMeshVisualization::blockProblemsReport() const
{
    std::ostringstream os;
    for (size_t i=0; i<blocks_.size(); ++i)
    {
        if (blocks_[i].isInvalid())
            os << blockProblemDescription(i) << "\n";
    }
    return os.str();
}




TopoDS_Shape blockMeshVisualization::problemEdges(size_t i) const
{
    const auto& b=blocks_[i];

    std::set<int> edges;
    if (b.insideOut)
    {
        for (int k=0; k<12; ++k) edges.insert(k);
    }
    else
    {
        for (int fi: b.inwardFaces)
        {
            const auto& f=ofHexFaces[fi];
            for (int k=0; k<4; ++k)
                edges.insert(blockEdgeIndex(f[k], f[(k+1)%4]));
        }
    }

    TopoDS_Compound res;
    BRep_Builder bb;
    bb.MakeCompound(res);
    for (int k: edges)
    {
        if (!b.displayEdges[k].IsNull())
            bb.Add(res, b.displayEdges[k]);
    }
    return res;
}




void blockMeshVisualization::addToVisualizer(
    CADParameterSetModelVisualizer &vz,
    const std::string &prefix,
    const ScaleOrTransform& s_or_t,
    insight::DatasetRepresentation repr ) const
{
    std::string p=prefix;
    if (!p.empty()) p+="/";

    gp_Trsf trsf;
    if (auto* tr = boost::get<gp_Trsf>(&s_or_t))
        trsf=*tr;
    else if (auto* tr = boost::get<insight::SpatialTransformation>(&s_or_t))
        trsf=cad::is_gp_Trsf(*tr);
    else if (auto* sf = boost::get<double>(&s_or_t))
        trsf.SetScaleFactor(*sf);
    else
        throw insight::UnhandledSelection();

    if (hasInvalidBlocks())
    {
        insight::Warning(
            "blockMesh will stop with an error. The following blocks are invalid"
            " (shown in red):\n%s",
            blockProblemsReport().c_str() );
    }

    for (size_t i=0; i<blocks_.size(); ++i)
    {
        if (blocks_[i].isInvalid())
        {
            vz.addEvaluation(
                p+str(boost::format("meshBlock_%d_problem")%i),
                std::make_shared<cad::FeatureAnnotation>(
                    cad::Import::create(
                        BRepBuilderAPI_Transform(problemEdges(i), trsf, true).Shape() ),
                    vec3(cad::to_Pnt(blocks_[i].centre).Transformed(trsf)),
                    blockProblemDescription(i),
                    vec3(1,0,0) ),
                true );
        }

        vz.addFeature
            (
                p+str(boost::format("meshBlock_%d")%i),
                cad::StructuredSurfaceMesh::create(
                    TopoDS_Shape(blocks_[i].edges),
                    blocks_[i].meshPreview,
                    trsf ),
            { repr,
              blocks_[i].isInvalid() ? vec3(1,0,0) : arma::mat(),
              {}, 0.33 }
                );
    }
}




void addBlockingToVisualizer(
    CADParameterSetModelVisualizer& vz,
    const bmd::blockMeshBlocking& bm,
    const std::string& prefix,
    const blockMeshVisualization::ScaleOrTransform& s_or_t,
    insight::DatasetRepresentation repr )
{
    blockMeshVisualization(bm).addToVisualizer(vz, prefix, s_or_t, repr);
}



}


} // namespace insight
