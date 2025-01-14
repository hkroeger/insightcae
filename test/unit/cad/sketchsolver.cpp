
#include "base/exception.h"

#include "datum.h"
#include "constrainedsketch.h"


using namespace insight;
using namespace insight::cad;

int main(int, char*argv[])
{
    try
    {
        std::string buf=
        "SketchPoint( 0, 120.59, -20.9308),\n"
        "SketchPoint( 10, 46.631, 4.14828),\n"
        "SketchPoint( 11, -3.23179, 7.84965),\n"
        "Line(15, 11, 10),\n"
        "HorizontalConstraint( 1, 15),\n"
        "SketchPoint( 36, 120.029, -30.9151),\n"
        "Line(2, 0, 36),\n"
        "SketchPoint( 7, -6.66453, -97.093),\n"
        "SketchPoint( 34, -4.50874, -97.0821),\n"
        "Line(30, 34, 7),\n"
        "HorizontalConstraint( 3, 30),\n"
        "SketchPoint( 4, 115.425, -112.892),\n"
        "SketchPoint( 18, 121.313, -8.05673),\n"
        "DistanceConstraint( 5, 18, 0, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"distance\" value=\"7\"/></root>),\n"
        "DistanceConstraint( 6, 0, 36, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"distance\" value=\"10\"/></root>),\n"
        "Line(8, 18, 4),\n"
        "FixedPoint( 9, 11, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"x\" value=\"0\"/><double name=\"y\" value=\"0\"/></root>),\n"
        "SketchPoint( 29, 46.4936, -2.85018),\n"
        "DistanceConstraint( 12, 10, 29, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"distance\" value=\"7\"/></root>),\n"
        "Line(13, 10, 29),\n"
        "PointOnCurveConstraint( 14, 8, 36),\n"
        "Line(31, 29, 18),\n"
        "HorizontalConstraint( 16, 31),\n"
        "SketchPoint( 21, -4.57017, -111.995),\n"
        "DistanceConstraint( 17, 4, 21, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"distance\" value=\"120\"/></root>),\n"
        "VerticalConstraint( 19, 13),\n"
        "DistanceConstraint( 20, 11, 7, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"distance\" value=\"105\"/></root>),\n"
        "VerticalConstraint( 22, 8),\n"
        "DistanceConstraint( 23, 29, 18, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"distance\" value=\"75\"/></root>),\n"
        "Line(33, 11, 7),\n"
        "VerticalConstraint( 24, 33),\n"
        "PointOnCurveConstraint( 25, 8, 0),\n"
        "Line(37, 4, 21),\n"
        "HorizontalConstraint( 26, 37),\n"
        "DistanceConstraint( 27, 11, 10, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"distance\" value=\"50\"/></root>),\n"
        "Line(32, 21, 34),\n"
        "VerticalConstraint( 28, 32),\n"
        "DistanceConstraint( 35, 18, 4, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?><root><double name=\"distance\" value=\"105\"/></root>)\n"
            ;
        std::istringstream is(buf);

        auto pl = std::make_shared<DatumPlane>(
            vec3const(0,0,0), vec3const(0,0,1)
            );
        auto csk = ConstrainedSketch::createFromStream(pl, is, *noParametersDelegate);

        auto ss = csk->solverSettings();
        ss.solver_=ConstrainedSketch::rootND;
        ss.tolerance_=1e-12;
        csk->changeSolverSettings(ss);

        csk->resolveConstraints();

        std::cout<<"solution=\n======================\n"<<std::endl;
        for (const auto& g: *csk)
        {
            if (auto sp = std::dynamic_pointer_cast<SketchPoint>(g.second))
            {
                arma::mat p2 = sp->coords2D();
                std::cout<<p2.t()<<std::endl;
            }
        }

        return 0;
    }
    catch (insight::Exception& e)
    {
        std::cerr<<e<<std::endl;
    }
    return -1;
}
