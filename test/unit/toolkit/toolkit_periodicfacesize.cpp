#include "base/exception.h"
#include "openfoam/blockmesh.h"
#include "openfoam/blockmesh/blockmeshprediction.h"
#include "openfoam/blockmesh/periodicfacesize.h"

#include <iostream>
#include <cmath>

using namespace insight;
using namespace insight::bmd;


namespace {

void check(bool cond, const std::string& msg)
{
    std::cout << (cond ? "OK:     " : "FAILED: ") << msg << std::endl;
    if (!cond) throw insight::Exception("check failed: "+msg);
}

PointList boxCorners(const arma::mat& p0, const arma::mat& L)
{
    double x0=p0(0), y0=p0(1), z0=p0(2);
    double x1=x0+L(0), y1=y0+L(1), z1=z0+L(2);
    return {
        vec3(x0,y0,z0), vec3(x1,y0,z0), vec3(x1,y1,z0), vec3(x0,y1,z0),
        vec3(x0,y0,z1), vec3(x1,y0,z1), vec3(x1,y1,z1), vec3(x0,y1,z1)
    };
}

/**
 * two separate blocks: the y-max face of block A (y=1) is periodic with the y-min face of block B (y=2),
 * T: y+=1. Block B is graded in x by gradingB (block A is uniform).
 */
void build(blockMeshBlocking& bm, double gradingB, int nxB=10)
{
    auto& a = bm.addBlock(new Block(boxCorners(vec3(0,0,0), vec3(1,1,1)), 10, 4, 4));
    auto& b = bm.addBlock(new Block(
        boxCorners(vec3(0,2,0), vec3(1,1,1)), nxB, 4, 4, { gradingB, 1., 1. } ));
    bm.addProjectedFace(ProjectedFace(a.face("2376"), "symA"));
    bm.addProjectedFace(ProjectedFace(b.face("0154"), "symB"));
}

V3 T(const V3& p) { return V3(p.x, p.y+1., p.z); }

}


int main(int, char*[])
{
    try
    {
        // conformal: ratio 1
        {
            blockMeshBlocking bm;
            build(bm, 1.);
            MeshPredictor mp(bm);
            PeriodicFaceSizeCheck c(mp, "symA", "symB", T);
            check(c.active(), "faces found on both sides");
            check(c.involves({0}) && c.involves({1}), "both blocks involved");
            auto r = c.evaluate(mp, 0.1);
            std::cout<<"conformal: "<<r.summary(0.1)<<std::endl;
            check(r.nFaces==2*10*4, "all faces of both sides compared");
            check(std::fabs(r.minAreaRatio-1.)<1e-9, "ratio 1 for identical subdivision");
            check(r.nViolating==0 && r.violation==0., "no violation");
        }

        // non-conformal, but similar face sizes
        {
            blockMeshBlocking bm;
            build(bm, 1., 7);
            MeshPredictor mp(bm);
            auto r = PeriodicFaceSizeCheck(mp, "symA", "symB", T).evaluate(mp, 0.1);
            std::cout<<"non-conformal: "<<r.summary(0.1)<<std::endl;
            check(std::fabs(r.minAreaRatio-0.7)<1e-9, "ratio of differently subdivided sides");
            check(r.nViolating==0, "no violation");
        }

        // squeezed faces on side B
        {
            blockMeshBlocking bm;
            build(bm, 1000.);
            MeshPredictor mp(bm);
            PeriodicFaceSizeCheck c(mp, "symA", "symB", T);
            auto r = c.evaluate(mp, 0.1);
            std::cout<<"squeezed: "<<r.summary(0.1)<<std::endl;
            // first cell of 10 cells with expansion ratio 1000
            double q = std::pow(1000., 1./9.);
            double h1 = (q-1.)/(std::pow(q, 10)-1.);
            check(std::fabs(r.minAreaRatio-h1/0.1)<1e-6, "ratio of the smallest face");
            check(r.nViolating>0 && r.violation>0., "violation detected");
            check(c.evaluate(mp, 0.).nViolating==0, "no limit: no violation");

            // reference: faces below the limit in the reference state may not get smaller
            auto ref = c.areaRatios(mp);
            check(ref.size()==size_t(r.nFaces), "one ratio per face");
            auto rr = c.evaluate(mp, 0.1, &ref);
            check(rr.nViolating==0 && rr.violation==0. && rr.nBelowLimit==r.nBelowLimit,
                  "no violation relative to the own state as reference");

            blockMeshBlocking bm2;
            build(bm2, 3000.);
            MeshPredictor mp2(bm2);
            auto r2 = c.evaluate(mp2, 0.1, &ref);
            std::cout<<"squeezed more: "<<r2.summary(0.1)<<std::endl;
            check(r2.nViolating>0 && r2.minAreaRatio<r.minAreaRatio, "squeezed faces smaller than in the reference");

            blockMeshBlocking bm3;
            build(bm3, 1.);
            MeshPredictor mp3(bm3);
            check(c.evaluate(mp3, 0.1, &ref).nViolating==0, "no violation of the reference by a uniform state");
        }

        // no faces on one side
        {
            blockMeshBlocking bm;
            build(bm, 1.);
            MeshPredictor mp(bm);
            PeriodicFaceSizeCheck c(mp, "symA", "other", T);
            check(!c.active(), "inactive without faces on side B");
            check(c.evaluate(mp, 0.1).nFaces==0, "nothing evaluated");
        }
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
