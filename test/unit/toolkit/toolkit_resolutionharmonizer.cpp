#include "base/exception.h"
#include "openfoam/blockmesh.h"
#include "openfoam/blockmesh/gradinganalyzer.h"
#include "openfoam/blockmesh/resolutionharmonizer.h"

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

// box block [x0,x1]x[y0,y1]x[0,1]
PointList box(double x0, double x1, double y0, double y1)
{
    return {
        vec3(x0,y0,0), vec3(x1,y0,0), vec3(x1,y1,0), vec3(x0,y1,0),
        vec3(x0,y0,1), vec3(x1,y0,1), vec3(x1,y1,1), vec3(x0,y1,1)
    };
}

// cell size ratio across the interface at x=x
double jumpAt(const ResolutionHarmonizer::Report& r, double x)
{
    double j=1.;
    for (const auto& jj: r.jumps)
        if (std::fabs(jj.location.x-x)<1e-9) j=std::max(j, jj.ratio);
    return j;
}

}




int main(int, char*[])
{
    try
    {
        // ===== row of blocks: fixed A, graded B, uniform C
        {
            blockMeshBlocking bm;
            bm.addBlock(new Block(box(0,1,0,1), 10, 1, 1));                     // A
            bm.addBlock(new Block(box(1,4,0,1), 1, 1, 1, {4., 1., 1.}));        // B
            bm.addBlock(new Block(box(4,6,0,1), 1, 1, 1));                      // C

            ResolutionHarmonizer h(bm);
            check( h.nGroups()==5, "3 separate x-groups, one y- and one z-group" );
            h.fixGroupsOfBlocks({0});
            std::cout<<h.harmonize([](const V3&) { return 1.; });
            h.apply(bm);

            int nB = bm.allBlocks()[1].resolution()[0], nC = bm.allBlocks()[2].resolution()[0];
            int nExp = GradingAnalyzer(4.).calc_n(0.1, 3.);
            std::cout<<"nB="<<nB<<" (GradingAnalyzer: "<<nExp<<"), nC="<<nC<<std::endl;
            check( std::abs(nB-nExp)<=1, "graded block: cell count from GradingAnalyzer" );

            auto r = ResolutionHarmonizer(bm).check();
            std::cout<<r.summary()<<std::endl;
            check( jumpAt(r, 1.)<1.1, "no jump at the interface fixed/graded block" );
            check( jumpAt(r, 4.)<1.15, "no jump at the interface graded/uniform block" );
            check( r.gradingMismatches.empty() && r.inconsistentGroups.empty(), "consistent" );
        }

        // ===== grading mismatch on a shared edge
        {
            blockMeshBlocking bm;
            bm.addBlock(new Block(box(0,1,0,1), 4, 4, 1, {2., 1., 1.}));
            bm.addBlock(new Block(box(0,1,1,2), 4, 4, 1, {3., 1., 1.}));
            auto r = ResolutionHarmonizer(bm).check();
            check( r.gradingMismatches.size()==2, "different gradings on the shared edges are reported" );
        }
        {
            // second block with local x along -x (and z along -z): reciprocal grading is consistent
            blockMeshBlocking bm;
            bm.addBlock(new Block(box(0,1,0,1), 4, 4, 1, {2., 1., 1.}));
            PointList c = {
                vec3(1,1,1), vec3(0,1,1), vec3(0,2,1), vec3(1,2,1),
                vec3(1,1,0), vec3(0,1,0), vec3(0,2,0), vec3(1,2,0)
            };
            bm.addBlock(new Block(c, 4, 4, 1, {0.5, 1., 1.}));
            auto r = ResolutionHarmonizer(bm).check();
            check( r.gradingMismatches.empty(), "reversed edge with reciprocal grading is consistent" );
            check( r.inconsistentGroups.empty(), "shared x-edges form one group" );
        }

        // ===== preference by weight: B/B' (one x-group) attached to a fine (A) and a coarse (A') block
        for (int pref: {0, 1})
        {
            blockMeshBlocking bm;
            bm.addBlock(new Block(box(0,1,0,1), 10, 1, 1));      // A: cell size 0.1
            bm.addBlock(new Block(box(-0.5,1,1,2), 6, 1, 1));    // A': cell size 0.25
            bm.addBlock(new Block(box(1,3,0,1), 1, 1, 1));       // B
            bm.addBlock(new Block(box(1,3,1,2), 1, 1, 1));       // B'
            ResolutionHarmonizer h(bm);
            h.fixGroupsOfBlocks({0, 1});
            h.harmonize( [pref](const V3& p) { return std::exp(-10.*(pref==0 ? p.y : 2.-p.y)); } );
            h.apply(bm);
            int n = bm.allBlocks()[2].resolution()[0];
            check( n==bm.allBlocks()[3].resolution()[0], "one count for the group" );
            std::cout<<"preferred "<<(pref==0?"y=0":"y=2")<<": n="<<n<<std::endl;
            check( pref==0 ? n==20 : n==8, "count matches the preferred interface" );
        }

        // ===== equal weights: compromise between the fine (A) and the coarse (A') interface
        {
            blockMeshBlocking bm;
            bm.addBlock(new Block(box(0,1,0,1), 10, 1, 1));      // A: cell size 0.1
            bm.addBlock(new Block(box(-0.5,1,1,2), 6, 1, 1));    // A': cell size 0.25
            bm.addBlock(new Block(box(1,3,0,1), 1, 1, 1));       // B
            bm.addBlock(new Block(box(1,3,1,2), 1, 1, 1));       // B'
            ResolutionHarmonizer h(bm);
            h.fixGroupsOfBlocks({0, 1});
            std::cout<<h.harmonize( [](const V3&) { return 1.; } );
            h.apply(bm);
            int n = bm.allBlocks()[2].resolution()[0];
            std::cout<<"equal weights: n="<<n<<std::endl;
            check( n==12 || n==13, "count is the compromise (cell size ~ sqrt(0.1*0.25))" );

            auto r = ResolutionHarmonizer(bm).check();
            std::cout<<r.summary()<<std::endl;
            check( r.maxJump()<1.7, "the jump is split between both interfaces" );
        }

        // ===== collapsed edges do not connect directions
        {
            blockMeshBlocking bm;
            // wedge: edge 3-7 collapsed in z (corners 3 and 7 coincide)
            PointList c = box(0,1,0,1);
            c[7] = c[3];
            bm.addBlock(new Block(c, 4, 5, 6));
            ResolutionHarmonizer h(bm);
            check( h.nGroups()==3, "collapsed block keeps 3 direction groups" );
        }
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
