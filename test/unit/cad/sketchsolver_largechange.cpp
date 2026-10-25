#include "base/exception.h"
#include "base/units.h"

#include "datum.h"
#include "constrainedsketch.h"
#include "constrainedsketchentities/sketchpoint.h"


using namespace insight;
using namespace insight::cad;


const std::string xmlDouble(const std::string& name, double v)
{
    return
        "parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root>"
        "<double name=\""+name+"\" value=\""+std::to_string(v)+"\"/>"
        "</root>";
}


ConstrainedSketchPtr createSketch(const std::string& script)
{
    std::istringstream is(script);
    auto pl = std::make_shared<DatumPlane>(
        vec3const(0,0,0), vec3const(0,0,1)
        );
    auto csk = ConstrainedSketch::createFromStream(pl, is, *noParametersDelegate);

    auto ss = csk->solverSettings();
    ss.solver_=ConstrainedSketch::rootND;
    ss.tolerance_=1e-10;
    csk->changeSolverSettings(ss);
    return csk;
}


double maxResidual(ConstrainedSketch& csk)
{
    double r=0;
    for (const auto& g: csk)
    {
        for (int i=0; i<g.second->nConstraints(); ++i)
        {
            r=std::max(r, fabs(g.second->getConstraintError(i)));
        }
    }
    return r;
}


arma::mat pointCoords(ConstrainedSketch& csk, int id)
{
    for (const auto& g: csk)
    {
        if (g.first==id)
        {
            auto sp = std::dynamic_pointer_cast<SketchPoint>(g.second);
            insight::assertion(bool(sp), "entity %d is not a point", id);
            return sp->coords2D();
        }
    }
    throw insight::Exception("entity %d not found", id);
}


void setParameter(ConstrainedSketch& csk, int id, const std::string& name, double v)
{
    for (const auto& g: csk)
    {
        if (g.first==id)
        {
            g.second->parametersRef().setDouble(name, v);
            return;
        }
    }
    throw insight::Exception("entity %d not found", id);
}


void check(bool cond, const std::string& msg)
{
    if (!cond) throw insight::Exception("check failed: "+msg);
}


const std::string rectangle =
    "SketchPoint( 1, 0, 0),\n"
    "SketchPoint( 2, 50, 0),\n"
    "SketchPoint( 3, 50, 30),\n"
    "SketchPoint( 4, 0, 30),\n"
    "Line(10, 1, 2),\n"
    "Line(11, 2, 3),\n"
    "Line(12, 3, 4),\n"
    "Line(13, 4, 1),\n"
    "FixedPoint( 5, 1, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"x\" value=\"0\"/><double name=\"y\" value=\"0\"/></root>),\n"
    "HorizontalConstraint( 20, 10),\n"
    "HorizontalConstraint( 21, 12),\n"
    "VerticalConstraint( 22, 11),\n"
    "VerticalConstraint( 23, 13),\n"
    "DistanceConstraint( 24, 1, 2, "+xmlDouble("distance", 50)+"),\n"
    "DistanceConstraint( 25, 2, 3, "+xmlDouble("distance", 30)+")\n"
    ;

const std::string angledLine =
    "SketchPoint( 1, 0, 0),\n"
    "SketchPoint( 2, 40, 23),\n"
    "Line(10, 1, 2),\n"
    "FixedPoint( 5, 1, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"x\" value=\"0\"/><double name=\"y\" value=\"0\"/></root>),\n"
    "DistanceConstraint( 24, 1, 2, "+xmlDouble("distance", 46)+"),\n"
    "AngleConstraint( 25, 2, toHorizontal, 1, "+xmlDouble("angle", 30)+")\n"
    ;


int main(int, char*argv[])
{
    try
    {
        // large change of dimensions: rectangle must keep its orientation
        {
            auto csk = createSketch(rectangle);
            csk->resolveConstraints();
            check(maxResidual(*csk)<1e-8, "initial rectangle solve");

            setParameter(*csk, 24, "distance", 500);
            setParameter(*csk, 25, "distance", 3);
            csk->resolveConstraints();

            arma::mat p3 = pointCoords(*csk, 3);
            std::cout<<"rectangle: p3="<<p3.t()<<std::endl;
            check(maxResidual(*csk)<1e-8, "rectangle residual after large change");
            check(fabs(p3(0)-500)<1e-6 && fabs(p3(1)-3)<1e-6, "rectangle corner position (no flip)");
        }

        // large change of an angle: point must stay on its side of the horizontal
        {
            auto csk = createSketch(angledLine);
            csk->resolveConstraints();
            check(maxResidual(*csk)<1e-8, "initial angle solve");

            setParameter(*csk, 25, "angle", 170);
            setParameter(*csk, 24, "distance", 460);
            csk->resolveConstraints();

            arma::mat p2 = pointCoords(*csk, 2);
            std::cout<<"angle: p2="<<p2.t()<<std::endl;
            check(maxResidual(*csk)<1e-8, "angle residual after large change");
            double phi=atan2(p2(1), p2(0));
            check(fabs(phi-170.*SI::deg)<1e-6, "angle on original side (no mirror flip)");
        }

        // failing solve must restore the initial geometry
        {
            auto csk = createSketch(rectangle);
            csk->resolveConstraints();
            arma::mat p3_before = pointCoords(*csk, 3);

            auto ss = csk->solverSettings();
            ss.maxContinuationRefinements_=2;
            csk->changeSolverSettings(ss);

            // unsatisfiable: measured distance is non-negative
            setParameter(*csk, 24, "distance", -10);
            bool failed=false;
            try
            {
                csk->resolveConstraints();
            }
            catch (const insight::Exception& ex)
            {
                std::cout<<"expected failure: "<<ex.message()<<std::endl;
                failed=true;
            }
            check(failed, "solve of unsatisfiable constraint should fail");
            arma::mat p3_after = pointCoords(*csk, 3);
            check(arma::norm(p3_after-p3_before, 2)<1e-12, "geometry restored after failure");
        }

        return 0;
    }
    catch (insight::Exception& e)
    {
        std::cerr<<e<<std::endl;
    }
    return -1;
}
