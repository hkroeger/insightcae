#include "base/exception.h"
#include "base/framedvector.h"

#include <sstream>

using namespace insight;

INSIGHT_DECLARE_FRAME(BodyFrame);
INSIGHT_DECLARE_FRAME(SensorFrame);


void check(bool cond, const std::string& what)
{
    if (!cond)
        throw insight::Exception("check failed: "+what);
}

bool equal(const arma::mat& a, const arma::mat& b)
{
    return arma::norm(a-b, 2) < 1e-10;
}


int main(int /*argc*/, char*/*argv*/[])
{
  try
  {
    // affine algebra
    {
        PointIn<GlobalFrame> p1(1, 2, 3), p2(4, 6, 3);
        VectorIn<GlobalFrame> d = p2 - p1;
        check(equal(d.raw(), vec3(3, 4, 0)), "point difference");
        check(fabs(norm(d)-5.)<1e-12, "norm");
        check(fabs(distance(p1, p2)-5.)<1e-12, "distance");
        check(equal((p1+d).raw(), p2.raw()), "point + vector");
        check(equal((d+p1).raw(), p2.raw()), "vector + point");
        check(equal((p2-d).raw(), p1.raw()), "point - vector");
        check(equal((2.*d).raw(), vec3(6, 8, 0)), "scalar * vector");
        check(equal((d/2.).raw(), vec3(1.5, 2, 0)), "vector / scalar");
        check(equal((-d).raw(), vec3(-3, -4, 0)), "negation");
        check(fabs(norm(normalized(d))-1.)<1e-12, "normalized");

        VectorIn<GlobalFrame> ex(1, 0, 0), ey(0, 1, 0);
        check(equal(cross(ex, ey).raw(), vec3(0, 0, 1)), "cross");
        check(fabs(dot(ex, ey))<1e-12, "dot");
    }

    // tagging untagged data
    {
        PointIn<GlobalFrame> p(vec3(1, 2, 3).t()); // row vector is accepted
        check(p.raw().n_rows==3 && p.raw().n_cols==1, "column storage");
        bool thrown=false;
        try { VectorIn<GlobalFrame> v(arma::mat(2,1)); }
        catch (const insight::Exception&) { thrown=true; }
        check(thrown, "wrong size is rejected");
    }

    // transformations
    SpatialTransformation stBG(vec3(1, -2, 0.5), vec3(10, 20, 30), 2.);
    SpatialTransformation stSB(vec3(0, 3, 0), vec3(-45, 0, 90));
    FrameTransformation<BodyFrame,GlobalFrame> tBG(stBG);
    FrameTransformation<SensorFrame,BodyFrame> tSB(stSB);

    PointIn<SensorFrame> ps(0.3, -1, 4);
    VectorIn<SensorFrame> vs(1, 1, 0);

    {
        check(equal(tSB(ps).raw(), stSB.trsfPt(ps.raw())), "point transform = trsfPt");
        check(equal(tSB(vs).raw(), stSB.trsfVec(vs.raw())), "vector transform = trsfVec");

        PointIn<SensorFrame> pBack = tSB.inverted()(tSB(ps));
        check(equal(pBack.raw(), ps.raw()), "inverse round trip");

        FrameTransformation<SensorFrame,GlobalFrame> tSG = tBG*tSB;
        check(equal(tSG(ps).raw(), tBG(tSB(ps)).raw()), "composition of points");
        check(equal(tSG(vs).raw(), tBG(tSB(vs)).raw()), "composition of vectors");

        // vectors ignore translation
        FrameTransformation<BodyFrame,GlobalFrame> tTrans(SpatialTransformation(vec3(5, 5, 5)));
        VectorIn<BodyFrame> vb(1, 2, 3);
        check(equal(tTrans(vb).raw(), vb.raw()), "vector unaffected by translation");

        // difference of transformed points == transformed difference
        PointIn<SensorFrame> ps2(2, 2, 2);
        check(equal((tSG(ps2)-tSG(ps)).raw(), tSG(ps2-ps).raw()), "affine consistency");
    }

    // bridge to CoordinateSystem
    {
        CoordinateSystem cs(vec3(1, 2, 3), vec3(0, 1, 0), vec3(0, 0, 1));
        auto toGlobal = localToParent<BodyFrame>(cs);
        PointIn<BodyFrame> pl(2, 0, 0);
        check(equal(toGlobal(pl).raw(), cs.localToGlobal().trsfPt(pl.raw())), "localToParent");
        check(equal(toGlobal(pl).raw(), (originOf<GlobalFrame>(cs)+2.*exOf<GlobalFrame>(cs)).raw()), "local x axis");
        check(equal(toGlobal(PointIn<BodyFrame>()).raw(), cs.origin), "local origin");
        check(equal(toGlobal(VectorIn<BodyFrame>(0, 0, 1)).raw(), ezOf<GlobalFrame>(cs).raw()), "local z axis");
        check(equal(toGlobal(VectorIn<BodyFrame>(0, 1, 0)).raw(), eyOf<GlobalFrame>(cs).raw()), "local y axis");

        auto toLocal = parentToLocal<BodyFrame>(cs);
        check(equal(toLocal(toGlobal(pl)).raw(), pl.raw()), "parentToLocal");
    }

    // output
    {
        std::ostringstream os;
        os << PointIn<BodyFrame>(1, 2, 3);
        check(os.str()=="point(1 2 3)@BodyFrame", "output: "+os.str());
    }
  }
  catch (const std::exception& e)
  {
      std::cerr<<e.what()<<std::endl;
      return -1;
  }

  return 0;
}
