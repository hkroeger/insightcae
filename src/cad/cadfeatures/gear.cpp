
/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "gear.h"
#include "circle.h"

#include "constantvector.h"
#include "constantscalar.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "GeomAPI_Interpolate.hxx"
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {



double involute_intersect_angle (double base_radius, double radius)
{
    double r = ::sqrt (::pow (radius/base_radius, 2) - 1.);
//     std::cout<<base_radius<<" "<<radius<<" "<<r<<std::endl;
    return r;
}

arma::mat rotate_point (double rotate, const arma::mat& coord)
{
    arma::mat r; 
    r
	 << ::cos (rotate) * coord[0] + ::sin (rotate) * coord[1]
     << ::cos (rotate) * coord[1] - ::sin (rotate) * coord[0]
    ;
    return r;
}

arma::mat mirror_point (const arma::mat& coord)
{
    arma::mat r;
    r
	 << coord[0]
	 << -coord[1]
    ;
    return r;
}

arma::mat involute(double base_radius, double involute_angle)
{
    arma::mat r;
    r 
     << base_radius*(::cos (involute_angle) + involute_angle*::sin (involute_angle))
     << base_radius*(::sin (involute_angle) - involute_angle*::cos (involute_angle))
    ;
    return r;
}



    
defineType(ExternalGear);
addToFactoryTable(Feature, ExternalGear);




ExternalGear::ExternalGear()
{}




ExternalGear::ExternalGear(VectorPtr p0, VectorPtr n, ScalarPtr m, ScalarPtr z)
: p0_(p0), n_(n), m_(m), z_(z)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=p0_->value();
  h+=n_->value();
  h+=m_->value();
  h+=z_->value();
}




ExternalGear::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape());
}




FeaturePtr ExternalGear::create(VectorPtr p0, VectorPtr n, ScalarPtr m, ScalarPtr z)
{
    return FeaturePtr(new ExternalGear(p0, n, m, z));
}

class involute_gear_tooth
: public TopoDS_Face
{
public:  
    
    double min_radius;
    arma::mat pitch_point;
    double pitch_angle;
    double centre_angle;
    double start_angle;
    double stop_angle;
    
    involute_gear_tooth
    (
        double pitch_radius,
        double root_radius,
        double base_radius,
        double outer_radius,
        double half_thick_angle,
        int resolution=10
    )
    : min_radius( std::max(base_radius,root_radius) ),
      pitch_point( involute (base_radius, involute_intersect_angle (base_radius, pitch_radius)) ),
      pitch_angle( ::atan2 (pitch_point[1], pitch_point[0]) ),
      centre_angle( pitch_angle + half_thick_angle ),
      start_angle( involute_intersect_angle (base_radius, min_radius) ),
      stop_angle( involute_intersect_angle (base_radius, outer_radius) )
    {
        
        Handle_TColgp_HArray1OfPnt pts1 = new TColgp_HArray1OfPnt( 1, resolution );
        Handle_TColgp_HArray1OfPnt pts2 = new TColgp_HArray1OfPnt( 1, resolution );

        TopTools_ListOfShape edgs;
        
        for (int i=0; i<resolution; i++)
        {
            arma::mat point=involute (base_radius, start_angle+(stop_angle - start_angle)* double(i)/double(resolution-1) );
            
            arma::mat side1_point=rotate_point (centre_angle, point);
            arma::mat side2_point=mirror_point (side1_point);
            
            pts1->SetValue ( i+1, gp_Pnt( side1_point[0], side1_point[1], 0 ) );
            pts2->SetValue ( i+1, gp_Pnt( side2_point[0], side2_point[1], 0 ) );
            
            if (i==0)
            {
                double f=root_radius/min_radius;
                gp_Pnt s1p(f*side1_point[0], f*side1_point[1], 0);
                gp_Pnt s2p(f*side2_point[0], f*side2_point[1], 0);
                
                edgs.Append
                (
                    BRepBuilderAPI_MakeEdge 
                    ( 
                        GC_MakeArcOfCircle
                        (
                            s2p, gp_Pnt(root_radius, 0, 0), s1p
                        ).Value() 
                    )
                );
//                 std::cout <<root_radius<<" "<<base_radius<<" "
//                     <<s1p.X()<<" "<<s1p.Y()<<" "<<s1p.Z()
//                     <<" "
//                     <<s2p.X()<<" "<<s2p.Y()<<" "<<s2p.Z()
//                     <<endl;
                if (root_radius<min_radius)
                {
                    edgs.Append(BRepBuilderAPI_MakeEdge ( gp_Pnt(side1_point[0], side1_point[1],0), s1p ));
                    edgs.Append(BRepBuilderAPI_MakeEdge ( gp_Pnt(side2_point[0], side2_point[1],0), s2p ));
//                 std::cout
//                     <<side1_point[0]<<" "<<side1_point[1]
//                     <<" "
//                     <<side2_point[0]<<" "<<side2_point[1]
//                     <<endl;
                }
            }
            else if (i==resolution-1)
            {
                GC_MakeArcOfCircle arc(
                    gp_Pnt(side1_point[0], side1_point[1], 0), 
                    gp_Pnt(outer_radius, 0, 0),
                    gp_Pnt(side2_point[0], side2_point[1], 0)
                    );
                edgs.Append(BRepBuilderAPI_MakeEdge ( arc.Value() ));
            }
        }


        GeomAPI_Interpolate ip1( pts1, false, 1e-6 );
        GeomAPI_Interpolate ip2( pts2, false, 1e-6 );
        
        ip1.Perform();
        ip2.Perform();
        
        Handle_Geom_BSplineCurve crv1= ip1.Curve();
        Handle_Geom_BSplineCurve crv2= ip2.Curve();
        
        edgs.Append(BRepBuilderAPI_MakeEdge ( BRepBuilderAPI_MakeEdge ( crv1, crv1->FirstParameter(), crv1->LastParameter() ) ));
        edgs.Append(BRepBuilderAPI_MakeEdge ( BRepBuilderAPI_MakeEdge ( crv2, crv2->FirstParameter(), crv2->LastParameter() ) ));
        
        BRepBuilderAPI_MakeWire w;
        w.Add(edgs);

        BRepBuilderAPI_MakeFace fb(w.Wire(), true);
        if (!fb.IsDone())
            throw insight::Exception("Failed to generate planar face!");
        
        TopoDS_Face::operator=( fb.Face() );
    }
};


class gear_shape
: public TopoDS_Shape 
{
public:
    involute_gear_tooth tooth_;
    
    gear_shape
    (
        int number_of_teeth,
        double pitch_radius,
        double root_radius,
        double base_radius,
        double outer_radius,
        double half_thick_angle,
        int resolution
    )
    : tooth_
    (
        pitch_radius,
        root_radius,
        base_radius,
        outer_radius,
        half_thick_angle,
        resolution
    )
    {
        TopoDS_Edge circ=BRepBuilderAPI_MakeEdge(gp_Circ( gp_Ax2(gp_Pnt(0,0,0), gp_Dir(0,0,1)), root_radius)).Edge();
        
        TopoDS_Shape f = BRepBuilderAPI_MakeFace( BRepBuilderAPI_MakeWire(circ).Wire() ).Shape();


        for ( int i = 0; i<number_of_teeth; i++ )
        {
            gp_Trsf tr;
            tr.SetRotation ( gp_Ax1 ( gp_Pnt ( 0,0,0 ), gp_Vec ( 0,0,1 ) ), i*2.*M_PI/double ( number_of_teeth ) );
            f=BRepAlgoAPI_Fuse
            (
                f,
                BRepBuilderAPI_Transform
                (
                    tooth_,
                    tr
                ).Shape()
            );
        }
            

        TopoDS_Shape::operator=( f );
    }
};


class gear 
: public TopoDS_Shape
{
    
public:
    
    double pitch_diameter;
    double pitch_radius;

    // Base Circle
    double base_radius;

    // Diametrial pitch: Number of teeth per unit length.
    double pitch_diametrial;

    // Addendum: Radial distance from pitch circle to outside circle.
    double addendum;

    //Outer Circle
    double outer_radius;

    // Dedendum: Radial distance from pitch circle to root diameter
    double dedendum;

    // Root diameter: Diameter of bottom of tooth spaces.
    double root_radius;
    double backlash_angle;
    double half_thick_angle;

    // Variables controlling the rim.
    double rim_radius;

    // Variables controlling the circular holes in the gear.
    double circle_orbit_diameter;
    double circle_orbit_curcumference;

    // Limit the circle size to 90% of the gear face.
    double circle_diameter;

    gear_shape gear_shape_;

    gear
    (
        int number_of_teeth=17,
        double module=3.,
        double pressure_angle=20.*M_PI/180.,
        double clearance = 0.2,
        double gear_thickness=5,
        double rim_thickness=8,
        double rim_width=5,
        double hub_thickness=10,
        double hub_diameter=15,
        double bore_diameter=5,
        double circles=8,
        double backlash=0,
        double twist=0,
        int resolution=10
    )
        // Pitch diameter: Diameter of pitch circle.
    :   pitch_diameter(number_of_teeth * module),
        pitch_radius(pitch_diameter/2.),

        // Base Circle
        base_radius(pitch_radius*::cos(pressure_angle)),

        // Diametrial pitch: Number of teeth per unit length.
        pitch_diametrial(number_of_teeth / pitch_diameter),

        // Addendum: Radial distance from pitch circle to outside circle.
        addendum( module /*1./pitch_diametrial*/),

        //Outer Circle
        outer_radius(pitch_radius+addendum),

        // Dedendum: Radial distance from pitch circle to root diameter
        dedendum(addendum + clearance*module),

        // Root diameter: Diameter of bottom of tooth spaces.
        root_radius(pitch_radius-dedendum),
        backlash_angle(backlash / pitch_radius),
        half_thick_angle((2.*M_PI / number_of_teeth - backlash_angle) / 4.),

        // Variables controlling the rim.
        rim_radius(root_radius - rim_width),

        // Variables controlling the circular holes in the gear.
        circle_orbit_diameter(hub_diameter/2.+rim_radius),
        circle_orbit_curcumference(M_PI*circle_orbit_diameter),

        // Limit the circle size to 90% of the gear face.
        circle_diameter(
            min (
                0.70*circle_orbit_curcumference/circles,
                (rim_radius-hub_diameter/2.)*0.9)),
    
        gear_shape_
        (
            number_of_teeth,
            pitch_radius,
            root_radius,
            base_radius,
            outer_radius,
            half_thick_angle,
            resolution
        )
    {
        
        double zg = 2./ ::pow(::sin(pressure_angle), 2); // minimum number of teeth without undercut
        if (number_of_teeth < zg)
            insight::Warning
            (
                boost::str(boost::format("number of teeth (%g) is lower than the minimum number of teeth (%g)!")%number_of_teeth%zg)
            );

        TopoDS_Shape s1=BRepPrimAPI_MakePrism
        (
            gear_shape_,
            gp_Vec(0,0,rim_thickness)
        ).Shape();
/*
        if (gear_thickness < rim_thickness)
        {
            gp_Trsf tr;
            tr.SetTranslation(gp_Vec(0,0,gear_thickness));
            s1=BRepAlgoAPI_Cut
            (
                s1, 
                BRepBuilderAPI_Transform
                (
                    BRepPrimAPI_MakeCylinder(rim_radius, rim_thickness-gear_thickness+1).Solid(),
                    tr
                ).Shape()
            );
        }
                            
        if (gear_thickness > rim_thickness)
        {
            s1=BRepAlgoAPI_Fuse
            (
                s1,
                BRepPrimAPI_MakeCylinder(rim_radius, gear_thickness).Solid()
            );
        }
        if (hub_thickness > gear_thickness)
        {
            gp_Trsf tr;
            tr.SetTranslation(gp_Vec(0,0,gear_thickness));
            s1=BRepAlgoAPI_Fuse
            (
                s1,
                BRepBuilderAPI_Transform
                (
                    BRepPrimAPI_MakeCylinder(hub_diameter/2., hub_thickness-gear_thickness).Solid(),
                    tr
                ).Shape()
            ).Shape();
        }

        {
            gp_Trsf tr;
            tr.SetTranslation(gp_Vec(0,0,-1.));
            s1=BRepAlgoAPI_Cut
            (
                s1, 
                BRepBuilderAPI_Transform
                (
                    BRepPrimAPI_MakeCylinder(bore_diameter/2., 2+std::max(std::max(rim_thickness,hub_thickness),gear_thickness)).Solid(),
                    tr
                ).Shape()
            );
        }

        if (circles>0)
        {
            for (int i=0; i<circles; i++)
            {
                gp_Trsf tr1, tr2;
                tr1.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(0,0,1)), double(i)*2.*M_PI/double(circles));
                tr2.SetTranslation(gp_Vec(circle_orbit_diameter/2., 0, -1.));
                s1=BRepAlgoAPI_Cut
                (
                    s1, 
                    BRepBuilderAPI_Transform
                    (
                        BRepPrimAPI_MakeCylinder(circle_diameter/2., std::max(gear_thickness,rim_thickness)+3).Solid(),
                        tr1.Multiplied(tr2)
                    ).Shape()
                );
            }
        }
        */
        TopoDS_Shape::operator=( s1 );

    }
};

void ExternalGear::build()
{
    gear g(z_->value(), m_->value());
    
    refvalues_["pitch_radius"] = g.pitch_radius;
    refvalues_["thick_angle"] = g.half_thick_angle*2.;
    
    providedSubshapes_["pitch_circle"]=Circle::create( vec3const(0,0,0), vec3const(0,0,1), scalarconst(g.pitch_diameter));
    
    setShape( g );

}




void ExternalGear::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "ExternalGear",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression > ',' > ruleset.r_scalarExpression > ')' ) 
	[ qi::_val = phx::bind(&ExternalGear::create, qi::_1, qi::_2, qi::_3, qi::_4) ]
      
    ))
  );
}
   



FeatureCmdInfoList ExternalGear::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "ExternalGear",
         
            "( <vector:p0>, <vector:n>, <scalar:m>, <scalar:z> )",
         
            "Creates an external gear mockup. The gear starts at point p0, axis and thickness are specified by vector n."
            " The diameter follows from modulus m and tooth number z."
        )
    );
}
   
   
   
}
}
