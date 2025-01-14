
#include "base/exception.h"
#include "datum.h"
#include "constrainedsketch.h"
#include "cadfeatures/line.h"
#include "constrainedsketchentities/distanceconstraint.h"
#include "constrainedsketchentities/angleconstraint.h"

using namespace insight;
using namespace insight::cad;

int main(int argc, char* argv[])
{
    try
    {
        auto scriptFile = boost::filesystem::temp_directory_path() / "constrainedSketchScript.scr";
        int nWritten, nRetrieved;

        {
            auto pl = std::make_shared<DatumPlane>(
                vec3const(0,0,0), vec3const(0,0,1) );

            auto sk = std::dynamic_pointer_cast<ConstrainedSketch>(
                ConstrainedSketch::create(pl, *noParametersDelegate));

            auto p1 = SketchPoint::create(pl, 0, 0);
            sk->insertGeometry(p1);
            auto p2 = SketchPoint::create(pl, 1, 0);
            sk->insertGeometry(p2);
            auto p3 = SketchPoint::create(pl, 1, 1);
            sk->insertGeometry(p3);

            auto l1 = Line::create(p1, p2, false);
            sk->insertGeometry(std::dynamic_pointer_cast<ConstrainedSketchEntity>(l1));
            auto l2 = Line::create(p2, p3, false);
            sk->insertGeometry(std::dynamic_pointer_cast<ConstrainedSketchEntity>(l2));
            auto l3 = Line::create(p3, p1, false);
            sk->insertGeometry(std::dynamic_pointer_cast<ConstrainedSketchEntity>(l3));

            auto lc1 = FixedDistanceConstraint::create( p1, p2, sk->sketchPlaneNormal());
            sk->insertGeometry(lc1);
            auto lc2 = FixedDistanceConstraint::create( p2, p3, sk->sketchPlaneNormal());
            sk->insertGeometry(lc2);
            auto lc3 = FixedDistanceConstraint::create( p1, p3, sk->sketchPlaneNormal());
            sk->insertGeometry(lc3);


            auto p1e=std::make_shared<insight::cad::AddedVector>(
                l1->start(), vec3const(1,0,0) );
            auto ac = FixedAngleConstraint::create(
                p1e, l1->end(), l1->start() );
            sk->insertGeometry(ac);

            auto lc0 = FixedDistanceConstraint::create( p1, vec3const(0,0,0), sk->sketchPlaneNormal() );
            sk->insertGeometry(lc0);

            sk->resolveConstraints();
//            sk->saveAs(boost::filesystem::temp_directory_path() / "sketch.brep");

            std::ofstream f(scriptFile.string());
            sk->generateScript(f);

            sk->generateScript(std::cout);

            nWritten = sk->size();

            int k=0;
            for (const auto& geo : *sk)
            {
                if (const auto sp = std::dynamic_pointer_cast<SketchPoint>(geo.second))
                {
                    std::cout<<(k++)<<" : "<<sp->coords2D().t();
                }
            }


            std::cout<<"nWritten="<<nWritten<<std::endl;

            arma::mat bb=sk->sketchBoundingBox();
            std::cout<<"bb=\n"<<bb<<std::endl;
            insight::assertion(
                arma::norm(bb.col(0)-vec3Zero(),2 )<0.001,
                "unexpected minimum bounding box coordinate!");
            insight::assertion(
                arma::norm(bb.col(1)-vec3(1., 0.866025, 0.), 2)<0.001,
                "unexpected maximum bounding box coordinate!");
        }

//        {
//            auto pl = std::make_shared<DatumPlane>(
//                vec3const(0,0,0), vec3const(0,0,1) );

//            std::istringstream is(
//"SketchPoint( 3, 1, 9.46859e-07),\n"
//"SketchPoint( 2, -4.83543e-07, 8.35933e-07),\n"
//"SketchPoint( 4, 0.5, 0.866026),\n"
//"Line(5, 2, 3),\n"
//"DistanceConstraint( 1, 2, 3, parameters <?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
//"<root>\n"
//"	<double name=\"distance\" value=\"1\"/>\n"
//"</root>\n"
//"\n"
//")\n" /*,\n"
//"Line(6, 3, 4),\n"
//"Line(7, 4, 2)\n"*/
//                );

//            auto sk = ConstrainedSketch::create(pl, is);

//            std::cout<<sk->geometry().size()<<std::endl;
//        }

        {
            auto pl = std::make_shared<DatumPlane>(
                vec3const(0,0,0), vec3const(0,0,1) );

            std::ifstream f(scriptFile.string());
            auto sk = ConstrainedSketch::createFromStream(pl, f, *noParametersDelegate);

            std::cout<<sk->size()<<std::endl;
            nRetrieved = sk->size();

            std::cout<<"nRetrieved="<<nRetrieved<<std::endl;

            sk->resolveConstraints();

            int k=0;
            for (const auto& geo : *sk)
            {
                if (const auto sp = std::dynamic_pointer_cast<SketchPoint>(geo.second))
                {
                    std::cout<<(k++)<<" : "<<sp->coords2D().t();
                }
            }

        }

        insight::assertion(
            nWritten==nRetrieved,
            "incomplete restore!" );

    }
    catch ( const boost::spirit::qi::expectation_failure<std::string::iterator>& e )
    {
        std::ostringstream os;
        os << e.what_;
        return -2;
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
