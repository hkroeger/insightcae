
#include "base/exception.h"
#include "openfoam/blockmesh.h"
#include "openfoam/blockmesh/blockmeshprediction.h"

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

PointList boxCorners(const arma::mat& p0, const arma::mat& L, double shearX=0.)
{
    double x0=p0(0), y0=p0(1), z0=p0(2);
    double x1=x0+L(0), y1=y0+L(1), z1=z0+L(2);
    return {
        vec3(x0,y0,z0), vec3(x1,y0,z0), vec3(x1,y1,z0), vec3(x0,y1,z0),
        vec3(x0+shearX,y0,z1), vec3(x1+shearX,y0,z1), vec3(x1+shearX,y1,z1), vec3(x0+shearX,y1,z1)
    };
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
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
