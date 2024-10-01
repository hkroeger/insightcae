
#include "base/exception.h"
#include "boost/range/adaptor/indexed.hpp"
#include "datum.h"
#include "constrainedsketch.h"
#include "cadfeatures/line.h"

#include "constrainedsketchentities/distanceconstraint.h"
#include "constrainedsketchentities/angleconstraint.h"
#include "constrainedsketchentities/fixedpointconstraint.h"
#include "constrainedsketchentities/horizontalconstraint.h"
#include "constrainedsketchentities/verticalconstraint.h"
#include <ostream>

using namespace insight;
using namespace insight::cad;




arma::mat sketchPointCoordinates(
    const insight::cad::ConstrainedSketch& sk )
{
    int k=0;
    for (const auto& geo : sk)
    {
        if (const auto sp = std::dynamic_pointer_cast<SketchPoint>(geo.second))
            k++;
    }

    arma::mat coords=arma::zeros(k,2);

    k=0;
    for (const auto& geo : sk)
    {
        if (const auto sp = std::dynamic_pointer_cast<SketchPoint>(geo.second))
        {
            coords.row(k)=sp->coords2D().t();
            k++;
        }
    }
    return coords;
}


std::ostream& operator<<(std::ostream& os, const ConstrainedSketch& sk)
{
    os<<std::endl;
    for (auto g = sk.begin(); g!=sk.end(); g++)
    {
        int k=std::distance(sk.begin(), g);
        os<<"entity_"<<k<<" ("<<g->second->type()<<")\t id:"<<g->first<<"\t "<<g->second<<std::endl;
        auto deps = g->second->dependencies();
        for (const auto& d: deps)
        {
            auto i = std::find_if(
                sk.begin(), sk.end(),
                [&d](const ConstrainedSketch::value_type& v)
                 { return v.second==d.lock(); } );

            if (i!=sk.end())
            {
                size_t j=std::distance(sk.begin(), i);
                os << std::string(5, ' ')
                   << "- entity_"<<j<<" ("<<i->second->type()<<")\t id:"<<i->first<<"\t "<<i->second<<std::endl;
            }
            else
            {
                os << "- ???? ("<<d.lock()->type()<<")"<<std::endl;
            }
        }
    }
    os << std::endl;
    return os;
}


int main(int argc, char* argv[])
{
    try
    {
        // create a sample sketch with all entity types

        // - point
        // - line

        // - angle constraint
        // - distance constraint
        // - external reference
        // not useable here:
        // - iqvtkfixedpoint
        // - iqvtkhorizontalconstraint
        // - iqvtkverticalconstraint
        // - iqvtkpointoncurveconstraint

        auto pl = std::make_shared<DatumPlane>(
            vec3const(0,0,0), vec3const(0,0,1) );


        arma::mat skp1;

        auto sk1 = std::dynamic_pointer_cast<ConstrainedSketch>(
            ConstrainedSketch::create(pl));
        int idp1, idp2;
        {
            /*
             *
             *  p1 *-----* p2
             *       l1
             *
             */
            auto p1 = SketchPoint::create(pl, 0, 0);
            idp1=sk1->insertGeometry(p1);
            auto p2 = SketchPoint::create(pl, 2, 0);
            idp2=sk1->insertGeometry(p2);
            auto l1 = Line::create(p1, p2, false); // y=const, horiz
            sk1->insertGeometry(l1);

            sk1->insertGeometry(
                insight::cad::FixedPointConstraint::create(p1) );
            sk1->insertGeometry(
                insight::cad::HorizontalConstraint::create(l1) );
            sk1->insertGeometry(
                insight::cad::FixedDistanceConstraint::create( p1, p2, sk1->sketchPlaneNormal()));

            sk1->resolveConstraints();

            skp1=sketchPointCoordinates(*sk1);

            std::cout << "SKETCH (incomplete)" << *sk1 << std::endl;

            std::cout << "SKETCH (incomplete) script" << std::endl;
            sk1->generateScript(std::cout);

            std::cout << "SKETCH (incomplete) points =" << skp1 << std::endl;
        }

        {
            // check identity copy
            auto skb = ConstrainedSketch::create<const ConstrainedSketch&>(*sk1);
            *sk1=*skb;
            std::cout << "SKETCH (incomplete, identity copied)" << *sk1 << std::endl;
            std::cout << "SKETCH (incomplete, identity copied) script" << std::endl;
            sk1->generateScript(std::cout);
        }


        {
            // create a copied sketch
            auto sk = ConstrainedSketch::create<const ConstrainedSketch&>(*sk1);
            std::cout << "SKETCH (copied, still incomplete)" << *sk << std::endl;

            // complete to this:
            /*
             *           * p3
             *          /|
             *     l3  / |
             *        /  | l2
             *       /   |
             *      /    |
             *  p1 *-----* p2
             *       l1
             */

            auto p1 = sk->get<SketchPoint>(idp1);
            auto p2 = sk->get<SketchPoint>(idp2);
            auto p3 = SketchPoint::create(pl, 1, 1);
            sk->insertGeometry(p3);

            auto l2 = Line::create(p2, p3); // x=const, vert
            sk->insertGeometry(l2);
            auto l3 = Line::create(p3, p1); // diagonal
            sk->insertGeometry(l3);

            sk->insertGeometry(
                insight::cad::VerticalConstraint::create(l2) );

            auto ac = insight::cad::FixedAngleConstraint::create(
                l3->start(), nullptr, l3->end() );
            ac->setTargetValue( 45.*M_PI/180. );
            sk->insertGeometry(ac);

            std::cout << "SKETCH (completed)" << *sk << std::endl;

            sk->resolveConstraints();


            arma::mat skp=sketchPointCoordinates(*sk);
            std::cout << "SKETCH (completed) points =" << skp << std::endl;

            // assign modified copy back to
            *sk1 = *sk;

            std::cout << "SKETCH (copied)" << *sk1 << std::endl;

            sk1->resolveConstraints();
            arma::mat skp2=sketchPointCoordinates(*sk1);
            std::cout << "P2=" << skp2 << std::endl;

            arma::mat diff = skp-skp2;

            insight::assertion(
                arma::norm(diff,2)<SMALL,
                "unexpected difference" );
        }

    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
