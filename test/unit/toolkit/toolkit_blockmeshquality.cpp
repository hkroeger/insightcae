
#include "base/exception.h"
#include "openfoam/blockmesh.h"
#include "openfoam/blockmesh/blockmeshprediction.h"
#include "base/temporaryfile.h"

#include <iostream>
#include <cmath>
#include <set>

using namespace insight;
using namespace insight::bmd;


namespace {

void check(bool cond, const std::string& msg)
{
    std::cout << (cond ? "OK:     " : "FAILED: ") << msg << std::endl;
    if (!cond) throw insight::Exception("check failed: "+msg);
}

/**
 * 2x2x2 (or nx x 2 x 2) blocks with displaced centre vertex and displaced boundary vertex,
 * all points transformed by T
 */
void distortedBlocks(
    blockMeshBlocking& bm,
    const arma::mat& c,
    const std::function<arma::mat(const arma::mat&)>& T,
    int nx=2 );

PointList boxCorners(const arma::mat& p0, const arma::mat& L, double shearX=0.)
{
    double x0=p0(0), y0=p0(1), z0=p0(2);
    double x1=x0+L(0), y1=y0+L(1), z1=z0+L(2);
    return {
        vec3(x0,y0,z0), vec3(x1,y0,z0), vec3(x1,y1,z0), vec3(x0,y1,z0),
        vec3(x0+shearX,y0,z1), vec3(x1+shearX,y0,z1), vec3(x1+shearX,y1,z1), vec3(x0+shearX,y1,z1)
    };
}

void distortedBlocks(
    blockMeshBlocking& bm,
    const arma::mat& c,
    const std::function<arma::mat(const arma::mat&)>& T,
    int nx )
{
    for (int k=0; k<2; ++k)
    {
        for (int j=0; j<2; ++j)
        {
            for (int i=0; i<nx; ++i)
            {
                auto pts = boxCorners(vec3(i,j,k), vec3(1,1,1));
                for (auto& p: pts)
                {
                    if (arma::norm(p-vec3(1,1,1))<1e-12) p=c;
                    else if (arma::norm(p-vec3(1,1,0))<1e-12) p=vec3(1.2,0.9,0);
                    p=T(p);
                }
                bm.addBlock(new Block(pts, 4, 4, 4));
            }
        }
    }
}

std::vector<arma::mat> allCorners(const blockMeshBlocking& bm)
{
    std::vector<arma::mat> res;
    for (const auto& b: bm.allBlocks())
        for (const auto& p: b.corners())
            res.push_back(p);
    return res;
}

std::vector<arma::mat> allEdgePoints(const blockMeshBlocking& bm)
{
    std::vector<arma::mat> res;
    for (const auto& e: bm.allEdges())
        if (const auto* se = dynamic_cast<const SplineEdge*>(&e))
            for (const auto& p: se->allPoints())
                res.push_back(p);
    return res;
}

double maxDistance(
    const std::vector<arma::mat>& a,
    const std::vector<arma::mat>& b,
    const std::function<arma::mat(const arma::mat&)>& T )
{
    if (a.size()!=b.size()) return 1e30;
    double d=0.;
    for (size_t i=0; i<a.size(); ++i)
        d=std::max(d, arma::norm(T(a[i])-b[i], 2));
    return d;
}

}




int main(int, char*[])
{
    try
    {
        // ===== spline evaluation
        {
            auto c = EdgeCurve::spline({ V3{0,0,0}, V3{0.3,0,0}, V3{1,0,0} });
            V3 p = c.position(0.5);
            check( std::fabs(p(1))<1e-12 && std::fabs(p(2))<1e-12, "spline through collinear points stays on the line" );

            auto a = EdgeCurve::arc( V3{1,0,0}, V3{M_SQRT1_2,M_SQRT1_2,0}, V3{0,1,0} );
            V3 q = a.position(0.5);
            check( norm(q-V3{M_SQRT1_2,M_SQRT1_2,0})<1e-12, "arc mid point" );
        }

        // ===== grading
        {
            std::vector<V3> pts;
            std::vector<double> d;
            lineDivide(EdgeCurve::line(V3{0,0,0}, V3{1,0,0}), 4, 8., pts, d);
            double r = std::pow(8., 1./3.); // cell to cell ratio
            double d0 = 1./(1.+r+r*r+r*r*r);
            check( std::fabs(pts[1](0)-d0)<1e-12, "first cell size with grading" );
            check( std::fabs((pts[4](0)-pts[3](0))/d0-8.)<1e-9, "last/first cell size ratio" );
        }

        // ===== two orthogonal blocks
        {
            blockMeshBlocking bm;
            bm.addBlock(new Block(boxCorners(vec3(0,0,0), vec3(1,1,1)), 4, 4, 4));
            bm.addBlock(new Block(boxCorners(vec3(1,0,0), vec3(1,1,1)), 4, 4, 4));
            MeshPredictor mp(bm);
            auto s = mp.statistics();
            std::cout<<s.summary()<<std::endl;
            check( s.nFaces == 2*3*3*16 + 16, "number of internal faces incl. block interface" );
            check( s.maxDeg < 1e-6, "orthogonal mesh has zero non-orthogonality" );
            check( s.nInvalidCells == 0, "no invalid cells" );
        }

        // ===== sheared block
        {
            blockMeshBlocking bm;
            bm.addBlock(new Block(boxCorners(vec3(0,0,0), vec3(1,1,1), 0.5), 4, 4, 4));
            MeshPredictor mp(bm);
            auto s = mp.statistics();
            std::cout<<s.summary()<<std::endl;
            check( std::fabs(s.maxDeg - std::atan(0.5)*180./M_PI) < 1e-6, "non-orthogonality of sheared block" );
        }

        // ===== smoothing: 2x2x2 blocks with displaced centre vertex
        {
            blockMeshBlocking bm;
            arma::mat c = vec3(1.3, 0.8, 1.2);
            for (int k=0; k<2; ++k)
            {
                for (int j=0; j<2; ++j)
                {
                    for (int i=0; i<2; ++i)
                    {
                        auto pts = boxCorners(vec3(i,j,k), vec3(1,1,1));
                        for (auto& p: pts)
                        {
                            if (arma::norm(p-vec3(1,1,1))<1e-12) p=c;
                        }
                        bm.addBlock(new Block(pts, 4, 4, 4));
                    }
                }
            }

            auto s0 = MeshPredictor(bm).statistics();
            std::cout<<"before: "<<s0.summary()<<std::endl;

            SmoothingParameters par;
            par.nSweeps=10;
            auto s1 = bm.smooth(par);
            std::cout<<"after: "<<s1.summary()<<std::endl;

            // re-predict from the modified blocking
            auto s2 = MeshPredictor(bm).statistics();
            std::cout<<"re-predicted: "<<s2.summary()<<std::endl;

            check( s1.maxDeg < 0.5*s0.maxDeg, "smoothing reduces max non-orthogonality" );
            check( std::fabs(s2.maxDeg-s1.maxDeg) < 1e-6, "modified blocking reproduces smoothed state" );
            check( s2.nInvalidCells == 0, "no invalid cells after smoothing" );

            // find centre vertex
            double dmin=1e10;
            for (const auto& b: bm.allBlocks())
                for (const auto& p: b.corners())
                    dmin=std::min(dmin, arma::norm(p-vec3(1,1,1)));
            check( dmin < 0.05, "centre vertex moved back close to the centre" );
        }

        // ===== sliding along a planar boundary: 2x2x1 blocks, centre vertex of the bottom face displaced in-plane
        {
            const arma::mat c0 = vec3(1.3, 0.7, 0.);
            auto build = [&](blockMeshBlocking& bm)
            {
                auto& bottom = bm.addPatch("bottom", new Patch());
                for (int j=0; j<2; ++j)
                {
                    for (int i=0; i<2; ++i)
                    {
                        auto pts = boxCorners(vec3(i,j,0), vec3(1,1,1));
                        for (auto& p: pts)
                        {
                            if (arma::norm(p-vec3(1,1,0))<1e-12) p=c0;
                        }
                        auto& b = bm.addBlock(new Block(pts, 4, 4, 4));
                        bottom.addFace(b.face("0321"));
                    }
                }
            };

            SmoothingParameters par;
            par.nSweeps=10;

            {
                blockMeshBlocking bm;
                build(bm);
                auto s0 = MeshPredictor(bm).statistics();
                auto s1 = bm.smooth(par);
                std::cout<<"fixed boundary: "<<s0.summary()<<" => "<<s1.summary()<<std::endl;
                int nFound=0;
                for (const auto& b: bm.allBlocks())
                    for (const auto& p: b.corners())
                        if (arma::norm(p-c0)<1e-12) nFound++;
                check( nFound==4, "boundary vertex stays fixed by default" );
            }
            {
                blockMeshBlocking bm;
                build(bm);
                auto s0 = MeshPredictor(bm).statistics();
                par.slideOnBoundaries=true;
                auto s1 = bm.smooth(par);
                std::cout<<"sliding boundary: "<<s0.summary()<<" => "<<s1.summary()<<std::endl;
                check( s1.maxDeg < 0.5*s0.maxDeg, "sliding reduces max non-orthogonality" );

                // bottom vertices stay in the bottom plane, the domain corners (on three planes) are fixed
                double zmax=0.;
                std::set<Point> corners;
                for (const auto& b: bm.allBlocks())
                {
                    for (const auto& p: b.corners())
                    {
                        corners.insert(p);
                        if (p(2)<0.5) zmax=std::max(zmax, std::fabs(p(2)));
                    }
                }
                check( zmax < 1e-12, "boundary vertices stay in the boundary plane" );
                bool cornersFixed=true;
                for (int k=0; k<2; ++k)
                    for (int j=0; j<2; ++j)
                        for (int i=0; i<2; ++i)
                            if (!corners.count(vec3(2*i,2*j,k))) cornersFixed=false;
                check( cornersFixed, "domain corners stay fixed" );

                // patch faces follow the moved vertices
                bool patchOK=true;
                for (const auto& f: bm.allPatches().at("bottom").faces())
                    for (const auto& p: f)
                        if (!corners.count(p)) patchOK=false;
                check( patchOK, "patch faces are updated" );
            }
        }

        // ===== sliding of a vertex on a triangular boundary face of a collapsed block:
        // 2x2x1 blocks, the block at (1,1) collapsed into a prism, centre vertex of the bottom face displaced in-plane
        {
            const arma::mat c0 = vec3(1.2, 0.9, 0.);
            blockMeshBlocking bm;
            auto& bottom = bm.addPatch("bottom", new Patch());
            for (int j=0; j<2; ++j)
            {
                for (int i=0; i<2; ++i)
                {
                    auto pts = boxCorners(vec3(i,j,0), vec3(1,1,1));
                    for (auto& p: pts)
                    {
                        if (arma::norm(p-vec3(1,1,0))<1e-12) p=c0;
                        if (i==1 && j==1 && std::fabs(p(0)-2.)<1e-12 && std::fabs(p(1)-2.)<1e-12)
                            p(0)=1.;
                    }
                    auto& b = bm.addBlock(new Block(pts, 4, 4, 4));
                    bottom.addFace(b.face("0321"));
                }
            }

            auto s0 = MeshPredictor(bm).statistics();
            SmoothingParameters par;
            par.nSweeps=10;
            par.slideOnBoundaries=true;
            auto s1 = bm.smooth(par);
            std::cout<<"collapsed block: "<<s0.summary()<<" => "<<s1.summary()<<std::endl;

            double dmin=1e10, zmax=0.;
            for (const auto& b: bm.allBlocks())
            {
                for (const auto& p: b.corners())
                {
                    if (p(2)<0.5)
                    {
                        dmin=std::min(dmin, arma::norm(p-c0, 2));
                        zmax=std::max(zmax, std::fabs(p(2)));
                    }
                }
            }
            check( dmin>1e-3, "vertex on triangular boundary face slides" );
            check( zmax<1e-12, "it stays in the boundary plane" );
            check( s1.maxDeg < s0.maxDeg, "sliding reduces max non-orthogonality" );
            check( s1.nInvalidCells==0, "no invalid cells" );
        }

        // ===== edge shapes: bounded bow, no S-shaped edges
        // 2x2x2 blocks, centre vertex displaced, boundary vertices sliding:
        // vertex moves and tangent rotations bend the edges
        {
            blockMeshBlocking bm;
            arma::mat c = vec3(1.3, 0.8, 1.2);
            for (int k=0; k<2; ++k)
            {
                for (int j=0; j<2; ++j)
                {
                    for (int i=0; i<2; ++i)
                    {
                        auto pts = boxCorners(vec3(i,j,k), vec3(1,1,1));
                        for (auto& p: pts)
                        {
                            if (arma::norm(p-vec3(1,1,1))<1e-12) p=c;
                            else if (arma::norm(p-vec3(1,1,0))<1e-12) p=vec3(1.2,0.9,0);
                        }
                        bm.addBlock(new Block(pts, 4, 4, 4));
                    }
                }
            }

            auto s0 = MeshPredictor(bm).statistics();

            SmoothingParameters par;
            par.nSweeps=10;
            par.maxBowRel=0.03;
            par.slideOnBoundaries=true;
            auto s1 = bm.smooth(par);
            std::cout<<"bent edges: "<<s0.summary()<<" => "<<s1.summary()<<std::endl;
            check( s1.maxDeg < s0.maxDeg, "smoothing reduces max non-orthogonality" );

            double maxBow=0.;
            int nS=0;
            for (const auto& e: bm.allEdges())
            {
                const auto* se = dynamic_cast<const SplineEdge*>(&e);
                if (!se) continue;
                auto P = se->allPoints();
                arma::mat ch = P.back()-P.front();
                double L = arma::norm(ch, 2);
                ch/=L;
                std::vector<arma::mat> dev;
                for (const auto& p: P)
                {
                    arma::mat r = p-P.front();
                    dev.push_back( r - arma::dot(r, ch)*ch );
                }
                size_t im=0;
                for (size_t i=0; i<dev.size(); ++i)
                    if (arma::norm(dev[i],2)>arma::norm(dev[im],2)) im=i;
                double bow = arma::norm(dev[im],2)/L;
                maxBow = std::max(maxBow, bow);
                if (bow>1e-9)
                {
                    arma::mat n = dev[im]/arma::norm(dev[im],2);
                    for (const auto& d: dev)
                        if (arma::dot(d, n) < -1e-3*L) { nS++; break; }
                }
            }
            std::cout<<"max. bow "<<maxBow<<", S-shaped edges "<<nS<<std::endl;
            check( maxBow <= par.maxBowRel+1e-9, "edge bow is bounded" );
            check( nS==0, "no S-shaped edges" );
        }

        // ===== stored modification: extract, re-apply, transfer to a modified geometry
        {
            auto identity = [](const arma::mat& p) { return arma::mat(p); };

            // rotation by 5 deg around the z-axis and scaling
            auto T = [](const arma::mat& p)
            {
                const double a=5.*M_PI/180., s=1.05;
                return arma::mat( s*vec3(
                    std::cos(a)*p(0)-std::sin(a)*p(1),
                    std::sin(a)*p(0)+std::cos(a)*p(1),
                    p(2) ) );
            };

            const arma::mat c = vec3(1.3, 0.8, 1.2);

            SmoothingParameters par;
            par.nSweeps=10;
            par.slideOnBoundaries=true;

            // reference
            blockMeshBlocking ref;
            distortedBlocks(ref, c, identity);
            auto s0 = MeshPredictor(ref).statistics();
            BlockingModification mod;
            auto s1 = ref.smooth(par, nullptr, &mod);
            std::cout<<"reference: "<<s0.summary()<<" => "<<s1.summary()<<std::endl;
            std::cout<<"stored: "<<mod.vertices.n_rows<<" vertices, "<<mod.edges.n_rows
                      <<" edges, topology "<<mod.topology<<", "<<mod.summary<<std::endl;
            check( !mod.empty() && mod.vertices.n_rows>0, "modification is returned" );

            // re-apply to the same geometry
            {
                blockMeshBlocking bm;
                distortedBlocks(bm, c, identity);
                std::string msg;
                bool ok = bm.applyModification(mod, par, &msg);
                std::cout<<msg<<std::endl;
                check( ok, "modification is applicable to the same blocking" );
                double dc = maxDistance(allCorners(ref), allCorners(bm), identity);
                double de = maxDistance(allEdgePoints(ref), allEdgePoints(bm), identity);
                std::cout<<"deviation of corners "<<dc<<", of edge points "<<de<<std::endl;
                check( dc<1e-9 && de<1e-9, "re-applied modification reproduces the smoothed blocking" );
                auto s2 = MeshPredictor(bm).statistics();
                check( std::fabs(s2.maxDeg-MeshPredictor(ref).statistics().maxDeg)<1e-6, "same quality" );
            }

            // file round trip
            {
                TemporaryFile tf("blockingmodification-%%%%.bmo");
                mod.write(tf.path());
                auto mod2 = BlockingModification::read(tf.path());
                check( mod2.topology==mod.topology && mod2.summary==mod.summary
                       && arma::approx_equal(mod2.vertices, mod.vertices, "absdiff", 1e-14)
                       && arma::approx_equal(mod2.edges, mod.edges, "absdiff", 1e-14)
                       && mod2.edges.n_cols==5,
                       "file round trip" );
            }

            // rotated and scaled geometry: local frames transform with it
            {
                blockMeshBlocking bm;
                distortedBlocks(bm, c, T);
                check( bm.applyModification(mod, par), "modification is applicable to the transformed blocking" );
                double dc = maxDistance(allCorners(ref), allCorners(bm), T);
                double de = maxDistance(allEdgePoints(ref), allEdgePoints(bm), T);
                std::cout<<"deviation from the transformed reference: corners "<<dc<<", edge points "<<de<<std::endl;
                check( dc<1e-9 && de<1e-9, "modification of transformed blocking equals transformed reference" );
            }

            // slightly different geometry
            {
                blockMeshBlocking bm;
                distortedBlocks(bm, c+vec3(0.04,-0.03,0.02), T);
                auto sv0 = MeshPredictor(bm).statistics();
                std::string msg;
                check( bm.applyModification(mod, par, &msg), "modification is applicable to the modified geometry" );
                auto sv1 = MeshPredictor(bm).statistics();
                std::cout<<"variant: "<<sv0.summary()<<" => "<<sv1.summary()<<std::endl;
                check( sv1.maxDeg < 0.7*sv0.maxDeg, "transferred modification improves the variant" );
                check( sv1.nInvalidCells==0, "no invalid cells in the variant" );
            }

            // different topology
            {
                blockMeshBlocking bm;
                distortedBlocks(bm, c, identity, 3);
                auto before = allCorners(bm);
                std::string msg;
                bool ok = bm.applyModification(mod, par, &msg);
                std::cout<<msg<<std::endl;
                check( !ok, "modification is rejected for a different topology" );
                check( maxDistance(before, allCorners(bm), identity)==0., "blocking stays unchanged" );
            }

            // warm start
            {
                blockMeshBlocking bm;
                distortedBlocks(bm, c, identity);
                BlockingModification mod2;
                auto s3 = bm.smooth(par, nullptr, &mod2, &mod);
                std::cout<<"warm start: "<<s3.summary()<<std::endl;
                check( s3.maxDeg <= s1.maxDeg+1e-6, "warm start does not get worse" );
                check( mod2.empty() || mod2.topology==mod.topology, "warm start result refers to the same topology" );

                if (!mod2.empty())
                {
                    blockMeshBlocking bm2;
                    distortedBlocks(bm2, c, identity);
                    bm2.applyModification(mod2, par);
                    check( maxDistance(allCorners(bm), allCorners(bm2), identity)<1e-9,
                           "warm start result is relative to the unmodified blocking" );
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
