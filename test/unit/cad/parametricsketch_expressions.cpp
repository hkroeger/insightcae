#include "base/exception.h"

#include "cadfeature.h"
#include "cadmodel.h"
#include "parser.h"
#include "datum.h"
#include "constrainedsketch.h"
#include "cadfeatures/parametricsketch.h"
#include "constrainedsketchentities/sketchpoint.h"

using namespace insight;
using namespace insight::cad;


std::shared_ptr<Model> parseModel(const std::string& scr)
{
    auto m = std::make_shared<cad::Model>();
    int failloc=-1;
    if (!parseISCADModel(scr, m.get(), &failloc))
    {
        throw insight::Exception(
            "Parser failed at \"%s\"",
            failloc>=0 ? scr.substr(failloc).c_str() : "" );
    }
    return m;
}


std::shared_ptr<const ConstrainedSketch> builtSketch(const Model& m)
{
    auto ps = std::dynamic_pointer_cast<ParametricSketch>(
        m.lookupModelstep("sk") );
    insight::assertion(bool(ps), "sk is not a ParametricSketch");
    ps->shape(); // trigger build
    return ps->sketch();
}


void checkGeometry(const ConstrainedSketch& sk, double L)
{
    auto p1 = sk.get<SketchPoint>(1)->coords2D();
    auto p2 = sk.get<SketchPoint>(2)->coords2D();
    std::cout<<"L="<<L<<": p1="<<p1.t()<<"p2="<<p2.t();

    insight::assertion(
        std::fabs(p1(0)-1.)<1e-6 && std::fabs(p1(1)-0.5*L)<1e-6,
        "fixed point is not at the location given by the expressions" );
    insight::assertion(
        std::fabs(arma::norm(p2-p1, 2)-L)<1e-6,
        "point distance does not match the expression" );
}


bool parseFails(const std::string& scr)
{
    try
    {
        auto m = std::make_shared<cad::Model>();
        if (!parseISCADModel(scr, m.get()))
            return true;
        builtSketch(*m);
    }
    catch (const std::exception& e)
    {
        std::cout<<"expected failure: "<<e.what()<<std::endl;
        return true;
    }
    return false;
}


int main(int, char*argv[])
{
    try
    {
        std::string sketchHead = "sk=ParametricSketch( XY,\n";
        std::string sketchScript =
            " SketchPoint(1, 0, 0),\n"
            " SketchPoint(2, 3, 0.1),\n"
            " Line(3, 1, 2),\n"
            " FixedPoint(4, 1, set (x = 1, y = 0.5*L)),\n"
            " DistanceConstraint(5, 1, 2, set (distance = L)),\n"
            " HorizontalConstraint(6, 3)\n";

        // initial parse: geometry is resolved according to the expressions
        auto m1 = parseModel("L=4;\n" + sketchHead + sketchScript + ");\n");
        auto sk1 = builtSketch(*m1);
        checkGeometry(*sk1, 4.);

        // round trip: bindings are preserved verbatim
        std::ostringstream gs;
        sk1->generateScript(gs);
        std::string generated = gs.str();
        std::cout<<generated<<std::endl;
        insight::assertion(
            generated.find("set (x = 1, y = 0.5*L)")!=std::string::npos,
            "binding of fixed point was not written back" );
        insight::assertion(
            generated.find("set (distance = L)")!=std::string::npos,
            "binding of distance constraint was not written back" );

        // same stored DoFs, changed variable: sketch has to be re-solved
        auto m2 = parseModel("L=4;\n" + sketchHead + generated + ");\n");
        auto m3 = parseModel("L=6;\n" + sketchHead + generated + ");\n");
        auto sk2 = builtSketch(*m2);
        auto sk3 = builtSketch(*m3);
        checkGeometry(*sk2, 4.);
        checkGeometry(*sk3, 6.);
        insight::assertion(
            sk2->hash() != sk3->hash(),
            "sketch hash does not reflect changed expression value" );

        // unknown parameter
        insight::assertion(
            parseFails(
                "L=4;\n" + sketchHead +
                " SketchPoint(1, 0, 0),\n"
                " FixedPoint(2, 1, set (z = L))\n"
                ");\n" ),
            "binding of non-existing parameter was accepted" );

        // bindings are not available outside of ISCAD scripts
        {
            auto pl = std::make_shared<DatumPlane>(
                vec3const(0,0,0), vec3const(0,0,1) );
            std::istringstream is(
                "SketchPoint(1, 0, 0),\n"
                "FixedPoint(2, 1, set (x = 1))\n" );
            bool failed=false;
            try
            {
                ConstrainedSketch::createFromStream(pl, is, *noParametersDelegate);
            }
            catch (const std::exception& e)
            {
                std::cout<<"expected failure: "<<e.what()<<std::endl;
                failed=true;
            }
            insight::assertion(
                failed,
                "binding was accepted in standalone sketch script" );
        }
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }
    return 0;
}
