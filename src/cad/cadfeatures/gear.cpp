
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
#include "TColgp_HArray1OfPnt.hxx"

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
                
                if (root_radius<min_radius)
                {
                    edgs.Append(BRepBuilderAPI_MakeEdge ( gp_Pnt(side1_point[0], side1_point[1],0), s1p ));
                    edgs.Append(BRepBuilderAPI_MakeEdge ( gp_Pnt(side2_point[0], side2_point[1],0), s2p ));
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
        
        edgs.Append( BRepBuilderAPI_MakeEdge ( crv1, crv1->FirstParameter(), crv1->LastParameter() ) );
        edgs.Append( BRepBuilderAPI_MakeEdge ( crv2, crv2->FirstParameter(), crv2->LastParameter() ) );
        
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
//     double rim_radius;

    // Variables controlling the circular holes in the gear.
//     double circle_orbit_diameter;
//     double circle_orbit_curcumference;

    // Limit the circle size to 90% of the gear face.
//     double circle_diameter;

    gear_shape gear_shape_;

    gear
    (
        double module=3.,
        int number_of_teeth=17,
        double gear_thickness=5,
        double clearance = 0.2,
//         double rim_thickness=8,
//         double rim_width=5,
//         double hub_thickness=10,
//         double hub_diameter=15,
//         double bore_diameter=5,
//         double circles=8,
        double backlash=0,
        double pressure_angle=20.*M_PI/180.,
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
//         rim_radius(root_radius - rim_width),

        // Variables controlling the circular holes in the gear.
//         circle_orbit_diameter(hub_diameter/2.+rim_radius),
//         circle_orbit_curcumference(M_PI*circle_orbit_diameter),

        // Limit the circle size to 90% of the gear face.
//         circle_diameter(
//             min (
//                 0.70*circle_orbit_curcumference/circles,
//                 (rim_radius-hub_diameter/2.)*0.9)),
    
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
            gp_Vec(0,0,/*rim_thickness*/gear_thickness)
        ).Shape();

//         if (gear_thickness < rim_thickness)
//         {
//             gp_Trsf tr;
//             tr.SetTranslation(gp_Vec(0,0,gear_thickness));
//             s1=BRepAlgoAPI_Cut
//             (
//                 s1, 
//                 BRepBuilderAPI_Transform
//                 (
//                     BRepPrimAPI_MakeCylinder(rim_radius, rim_thickness-gear_thickness+1).Solid(),
//                     tr
//                 ).Shape()
//             );
//         }
//                             
//         if (gear_thickness > rim_thickness)
//         {
//             s1=BRepAlgoAPI_Fuse
//             (
//                 s1,
//                 BRepPrimAPI_MakeCylinder(rim_radius, gear_thickness).Solid()
//             );
//         }
//         if (hub_thickness > gear_thickness)
//         {
//             gp_Trsf tr;
//             tr.SetTranslation(gp_Vec(0,0,gear_thickness));
//             s1=BRepAlgoAPI_Fuse
//             (
//                 s1,
//                 BRepBuilderAPI_Transform
//                 (
//                     BRepPrimAPI_MakeCylinder(hub_diameter/2., hub_thickness-gear_thickness).Solid(),
//                     tr
//                 ).Shape()
//             ).Shape();
//         }

//         {
//             gp_Trsf tr;
//             tr.SetTranslation(gp_Vec(0,0,-1.));
//             s1=BRepAlgoAPI_Cut
//             (
//                 s1, 
//                 BRepBuilderAPI_Transform
//                 (
//                     BRepPrimAPI_MakeCylinder(bore_diameter/2., 2+std::max(std::max(rim_thickness,hub_thickness),gear_thickness)).Solid(),
//                     tr
//                 ).Shape()
//             );
//         }

//         if (circles>0)
//         {
//             for (int i=0; i<circles; i++)
//             {
//                 gp_Trsf tr1, tr2;
//                 tr1.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(0,0,1)), double(i)*2.*M_PI/double(circles));
//                 tr2.SetTranslation(gp_Vec(circle_orbit_diameter/2., 0, -1.));
//                 s1=BRepAlgoAPI_Cut
//                 (
//                     s1, 
//                     BRepBuilderAPI_Transform
//                     (
//                         BRepPrimAPI_MakeCylinder(circle_diameter/2., std::max(gear_thickness,rim_thickness)+3).Solid(),
//                         tr1.Multiplied(tr2)
//                     ).Shape()
//                 );
//             }
//         }

        TopoDS_Shape::operator=( s1 );

    }
};






    
defineType(SpurGear);
addToFactoryTable(Feature, SpurGear);




SpurGear::SpurGear()
{}




SpurGear::SpurGear(ScalarPtr m, ScalarPtr z, ScalarPtr t, ScalarPtr clearance)
: m_(m), z_(z), t_(t), clearance_(clearance)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=m_->value();
  h+=z_->value();
  h+=t_->value();
  h+=clearance_->value();
}




SpurGear::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape());
}




FeaturePtr SpurGear::create(ScalarPtr m, ScalarPtr z, ScalarPtr t, ScalarPtr clearance)
{
    return FeaturePtr(new SpurGear(m, z, t, clearance));
}




void SpurGear::build()
{
    gear g( m_->value(), z_->value(), t_->value(), clearance_->value() );
    
    refvalues_["pitch_radius"] = g.pitch_radius;
    refvalues_["thick_angle"] = g.half_thick_angle*2.;
    
    providedSubshapes_["pitch_circle"]=Circle::create( vec3const(0,0,0), vec3const(0,0,1), scalarconst(g.pitch_diameter));
    
    setShape( g );

}




void SpurGear::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "SpurGear",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_scalarExpression > ',' 
      > ruleset.r_scalarExpression > ',' 
      > ruleset.r_scalarExpression
      > ( ( ',' > ruleset.r_scalarExpression ) | (qi::attr(scalarconst(0.2))) )
      > ')'
    ) 
	[ qi::_val = phx::bind(&SpurGear::create, qi::_1, qi::_2, qi::_3, qi::_4) ]
      
    ))
  );
}
   



FeatureCmdInfoList SpurGear::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "SpurGear",
         
            "( <scalar:m>, <scalar:z>, <scalar:t>[, <scalar:clearance>] )",
         
            "Creates a spur gear with thickness t."
            " The diameter follows from module m and tooth number z. Optionally, the clearance (as a fraction of module) can be specified. If not specified, 0.2 is used."
        )
    );
}
   
   

   
   
   
   
   
   
   
   


class involute_bevel_gear_tooth
: public TopoDS_Shape
{
public:
    
    involute_bevel_gear_tooth
    (
        double back_cone_radius,
        double root_radius,
        double base_radius,
        double outer_radius,
        double pitch_apex,
        double cone_distance,
        double half_thick_angle,
        int resolution = 10
    )
    {
//     BRep_Builder bb;
//     TopoDS_Compound result;
//     bb.MakeCompound ( result );
//         

        double min_radius = max (base_radius*2,root_radius*2);

        arma::mat pitch_point = 
            involute (
                base_radius*2, 
                involute_intersect_angle (base_radius*2, back_cone_radius*2));
        double pitch_angle = ::atan2 (pitch_point[1], pitch_point[0]);
        double centre_angle = pitch_angle + half_thick_angle;

        double start_angle = involute_intersect_angle (base_radius*2, min_radius);
        double stop_angle = involute_intersect_angle (base_radius*2, outer_radius*2);

//         res=(involute_facets!=0)?involute_facets:($fn==0)?5:$fn/4;
        
        Handle_TColgp_HArray1OfPnt pts1 = new TColgp_HArray1OfPnt( 1, resolution );
        Handle_TColgp_HArray1OfPnt pts2 = new TColgp_HArray1OfPnt( 1, resolution );

        Handle_TColgp_HArray1OfPnt pts1a = new TColgp_HArray1OfPnt( 1, resolution );
        Handle_TColgp_HArray1OfPnt pts2a = new TColgp_HArray1OfPnt( 1, resolution );
        
        TopTools_ListOfShape e1, e2;
        
        for (int i=0; i<resolution; i++)
        {
            arma::mat point=
                involute( base_radius*2,start_angle+(stop_angle - start_angle)*double(i)/double(resolution-1) );
            arma::mat side1_point = rotate_point (centre_angle, point);
            arma::mat side2_point = mirror_point (rotate_point (centre_angle, point));
            arma::mat pend; pend << (back_cone_radius*2.+0.1) << 0 << (cone_distance*2.);
            
            std::cout<<i<<" "<<side1_point[0]<<" "<<side1_point[1]<<" "<<side2_point[0]<<" "<<side2_point[1]<<std::endl;
            
            pts1->SetValue ( i+1, gp_Pnt( side1_point[0], side1_point[1], 0 ) );
            pts2->SetValue ( i+1, gp_Pnt( side2_point[0], side2_point[1], 0 ) );

            double f=0.1/(cone_distance*2.);
            double ze=cone_distance*2.;
            gp_Pnt ep1( f*side1_point[0]+(1.-f)*pend[0], f*side1_point[1]+(1.-f)*pend[1], pend[2] );
            gp_Pnt ep2( f*side2_point[0]+(1.-f)*pend[0], f*side2_point[1]+(1.-f)*pend[1], pend[2] );
            pts1a->SetValue ( i+1, ep1 );
            pts2a->SetValue ( i+1, ep2 );
            
            if (i==0)
            {
                e1.Append( BRepBuilderAPI_MakeEdge ( gp_Pnt( side1_point[0], side1_point[1], 0 ), gp_Pnt(0,0,0) ) );
                e1.Append( BRepBuilderAPI_MakeEdge ( gp_Pnt( side2_point[0], side2_point[1], 0 ), gp_Pnt(0,0,0) ) );
                
                e2.Append( BRepBuilderAPI_MakeEdge ( ep1, gp_Pnt(0,0,pend[2]) ) );
                e2.Append( BRepBuilderAPI_MakeEdge ( ep2, gp_Pnt(0,0,pend[2]) ) );
            }
            else if (i==resolution-1)
            {
                e1.Append
                ( 
                    BRepBuilderAPI_MakeEdge 
                    ( 
                        GC_MakeArcOfCircle
                        (
                            gp_Pnt(side2_point[0], side2_point[1], 0),
                            gp_Pnt(2.*outer_radius, 0, 0),
                            gp_Pnt(side1_point[0], side1_point[1], 0) 
                        ).Value() 
                    )
                );
                e2.Append
                ( 
                    BRepBuilderAPI_MakeEdge 
                    ( 
                      ep1, ep2
//                         GC_MakeArcOfCircle
//                         (
//                             ep1,
//                             gp_Pnt(2.*f*outer_radius, 0, pend[2]),
//                             ep2
//                         ).Value() 
                    )
                );
            }
            
        }
        
        GeomAPI_Interpolate ip1( pts1, false, 1e-6 );
        GeomAPI_Interpolate ip2( pts2, false, 1e-6 );
        GeomAPI_Interpolate ip1a( pts1a, false, 1e-6 );
        GeomAPI_Interpolate ip2a( pts2a, false, 1e-6 );
        
        ip1.Perform();
        ip2.Perform();
        ip1a.Perform();
        ip2a.Perform();
        
        Handle_Geom_BSplineCurve crv1= ip1.Curve();
        Handle_Geom_BSplineCurve crv2= ip2.Curve();
        Handle_Geom_BSplineCurve crv1a= ip1a.Curve();
        Handle_Geom_BSplineCurve crv2a= ip2a.Curve();
        
        e1.Append( BRepBuilderAPI_MakeEdge ( crv1, crv1->FirstParameter(), crv1->LastParameter() ) );
        e1.Append( BRepBuilderAPI_MakeEdge ( crv2, crv2->FirstParameter(), crv2->LastParameter() ) );

        e2.Append( BRepBuilderAPI_MakeEdge ( crv1a, crv1a->FirstParameter(), crv1a->LastParameter() ) );
        e2.Append( BRepBuilderAPI_MakeEdge ( crv2a, crv2a->FirstParameter(), crv2a->LastParameter() ) );
        
        BRepBuilderAPI_MakeWire w1, w2;
        w1.Add(e1);
        w2.Add(e2);
        
        TopoDS_Face f1=BRepBuilderAPI_MakeFace(w1.Wire());
        TopoDS_Face f2=BRepBuilderAPI_MakeFace(w2.Wire());
        
//         bb.Add(result, f1);
//         bb.Add(result, f2);
        
        BRepOffsetAPI_ThruSections sb1 ( true );
        sb1.AddVertex(BRepBuilderAPI_MakeVertex(gp_Pnt(back_cone_radius*2+0.1,0,cone_distance*2)).Vertex());
        sb1.AddWire ( w1.Wire() );
        BRepOffsetAPI_ThruSections sb2 ( true );
        sb2.AddWire ( w1.Wire() );
        sb2.AddVertex(BRepBuilderAPI_MakeVertex(gp_Pnt(0.1,0,0)).Vertex());
//         sb.AddWire ( w2.Wire() );
//         bb.Add(result, sb.Shape());
        
        TopoDS_Shape sb=BRepAlgoAPI_Fuse(sb1.Shape(), sb2.Shape()).Shape();
        
        gp_Trsf tr1, tr2, tr3;
        tr1.SetTranslation(gp_Vec(0,0,pitch_apex));
        tr2.SetRotation(gp::OY(), -::atan(back_cone_radius/cone_distance));
        tr3.SetTranslation(gp_Vec(-back_cone_radius*2, 0, -cone_distance*2));
        
        TopoDS_Shape::operator=( BRepBuilderAPI_Transform(sb, tr1.Multiplied(tr2.Multiplied(tr3))).Shape() );
        
/*
        translate ([0,0,pitch_apex])
        rotate ([0,-atan(back_cone_radius/cone_distance),0])
        translate ([-back_cone_radius*2,0,-cone_distance*2])
        union ()
        {
            for (i=[1:res])
            {
                assign (
                    point1=
                        involute (base_radius*2,start_angle+(stop_angle - start_angle)*(i-1)/res),
                    point2=
                        involute (base_radius*2,start_angle+(stop_angle - start_angle)*(i)/res))
                {
                    assign (
                        side1_point1 = rotate_point (centre_angle, point1),
                        side1_point2 = rotate_point (centre_angle, point2),
                        side2_point1 = mirror_point (rotate_point (centre_angle, point1)),
                        side2_point2 = mirror_point (rotate_point (centre_angle, point2)))
                    {
                        polyhedron (
                            points=[
                                [back_cone_radius*2+0.1,0,cone_distance*2],
                                [side1_point1[0],side1_point1[1],0],
                                [side1_point2[0],side1_point2[1],0],
                                [side2_point2[0],side2_point2[1],0],
                                [side2_point1[0],side2_point1[1],0],
                                [0.1,0,0]],
                            triangles=[[0,1,2],[0,2,3],[0,3,4],[0,5,1],[1,5,2],[2,5,3],[3,5,4],[0,4,5]]);
                    }
                }
            }
        }
        */

    }
    
};






class bevel_gear 
: public TopoDS_Shape
{
    
public:
    bevel_gear 
    (
        int number_of_teeth=11,
        double cone_distance=100,
        double face_width=20,
        double outside_circular_pitch=50,
        double pressure_angle=30.*M_PI/180.,
        double clearance = 0.2,
        double bore_diameter=5,
        double gear_thickness = 15,
        double backlash = 0,
        int resolution=10
    )
    {

        // Pitch diameter: Diameter of pitch circle at the fat end of the gear.
        double outside_pitch_diameter  =  number_of_teeth * outside_circular_pitch / M_PI;
        double outside_pitch_radius = outside_pitch_diameter / 2.;

        // The height of the pitch apex.
        double pitch_apex = ::sqrt (::pow (cone_distance, 2) - ::pow (outside_pitch_radius, 2));
        double pitch_angle = ::asin (outside_pitch_radius/cone_distance);

//         echo ("Num Teeth:", number_of_teeth, " Pitch Angle:", pitch_angle);

//         finish = (finish != -1) ? finish : (pitch_angle < 45) ? bevel_gear_flat : bevel_gear_back_cone;

        double apex_to_apex=cone_distance / ::cos (pitch_angle);
        double back_cone_radius = apex_to_apex * ::sin (pitch_angle);

        // Calculate and display the pitch angle. This is needed to determine the angle to mount two meshing cone gears.

        // Base Circle for forming the involute teeth shape.
        double base_radius = back_cone_radius * ::cos (pressure_angle);	

        // Diametrial pitch: Number of teeth per unit length.
        double pitch_diametrial = number_of_teeth / outside_pitch_diameter;

        // Addendum: Radial distance from pitch circle to outside circle.
        double addendum = 1. / pitch_diametrial;
        // Outer Circle
        double outer_radius = back_cone_radius + addendum;

        // Dedendum: Radial distance from pitch circle to root diameter
        double dedendum = addendum + clearance;
        double dedendum_angle = ::atan (dedendum / cone_distance);
        double root_angle = pitch_angle - dedendum_angle;

        double root_cone_full_radius = ::tan (root_angle)*apex_to_apex;
        double back_cone_full_radius=apex_to_apex / ::tan (pitch_angle);

        double back_cone_end_radius = 
            outside_pitch_radius - 
            dedendum * ::cos (pitch_angle) - 
            gear_thickness / ::tan (pitch_angle);
        double back_cone_descent = dedendum * ::sin (pitch_angle) + gear_thickness;

        // Root diameter: Diameter of bottom of tooth spaces.
        double root_radius = back_cone_radius - dedendum;

        double half_tooth_thickness = outside_pitch_radius * ::sin (2.*M_PI / (4 * number_of_teeth)) - backlash / 4.;
        double half_thick_angle = ::asin (half_tooth_thickness / back_cone_radius);

        double face_cone_height = apex_to_apex-face_width / ::cos (pitch_angle);
        double face_cone_full_radius = face_cone_height / ::tan (pitch_angle);
        double face_cone_descent = dedendum * ::sin (pitch_angle);
        double face_cone_end_radius = 
            outside_pitch_radius -
            face_width / ::sin (pitch_angle) - 
            face_cone_descent / ::tan (pitch_angle);

        // For the bevel_gear_flat finish option, calculate the height of a cube to select the portion of the gear that includes the full pitch face.
        double bevel_gear_flat_height = pitch_apex - (cone_distance - face_width) * ::cos (pitch_angle);
        
//         std::cout
//           <<outside_pitch_diameter<<std::endl
//           <<outside_pitch_radius<<std::endl
//           <<pitch_apex<<std::endl
//           <<pitch_angle<<std::endl
//           <<apex_to_apex<<std::endl
//           <<back_cone_radius<<std::endl
//           <<base_radius<<std::endl
//           <<pitch_diametrial<<std::endl
//           <<addendum<<std::endl
//           <<outer_radius<<std::endl
//           <<dedendum<<std::endl
//           <<dedendum_angle<<std::endl
//           <<root_angle<<std::endl
// 
//           <<root_cone_full_radius<<std::endl
//           <<back_cone_full_radius<<std::endl
// 
//           <<back_cone_end_radius<<std::endl
//           <<back_cone_descent<<std::endl
// 
//         // Root diameter: Diameter of bottom of tooth spaces.
//           <<root_radius<<std::endl
// 
//           <<half_tooth_thickness<<std::endl
//           <<half_thick_angle<<std::endl
// 
//           <<face_cone_height<<std::endl
//           <<face_cone_full_radius<<std::endl
//           <<face_cone_descent <<std::endl
//           <<face_cone_end_radius<<std::endl
// 
//         // For the bevel_gear_flat finish option, calculate the height of a cube to select the portion of the gear that includes the full pitch face.
//           <<bevel_gear_flat_height
//         <<std::endl;        
        
                std::cout << "PP" << std::endl 
                <<back_cone_radius<< std::endl 
            <<root_radius<< std::endl 
            <<base_radius<< std::endl 
            <<outer_radius<< std::endl 
            <<pitch_apex<< std::endl 
            <<cone_distance<< std::endl 
            <<half_thick_angle<< std::endl 
            <<resolution<< std::endl << "PP"<< std::endl ;
            
        involute_bevel_gear_tooth tooth_
        (
            back_cone_radius,
            root_radius,
            base_radius,
            outer_radius,
            pitch_apex,
            cone_distance,
            half_thick_angle,
            resolution
        );
        
    
        
        
        TopoDS_Shape result;
        
        for (int i = 0; i<number_of_teeth; i++)
        {
            gp_Trsf tr;
            tr.SetRotation ( gp_Ax1 ( gp_Pnt ( 0,0,0 ), gp_Vec ( 0,0,1 ) ), double(i)*2.*M_PI/double ( number_of_teeth ) );
            TopoDS_Shape t =BRepBuilderAPI_Transform
            (
                tooth_,
                tr
            ).Shape();
            if (i==0) 
            {
                result=t;
            }
            else 
            {
                result=BRepAlgoAPI_Fuse
                (
                    result, t
                ).Shape();
            }
        }
            
    //	translate([0,0,-pitch_apex])
//         difference ()
//         {
//             intersection ()
//             {
//                 union()
//                 {
//                     rotate (half_thick_angle)
//                     translate ([0,0,pitch_apex-apex_to_apex])
//                     cylinder ($fn=number_of_teeth*2, r1=root_cone_full_radius,r2=0,h=apex_to_apex);
//                     
//                     for (i = [1:number_of_teeth])
//     //				for (i = [1:1])
//                     {
//                         rotate ([0,0,i*360/number_of_teeth])
//                         {
//                             involute_bevel_gear_tooth (
//                                 back_cone_radius = back_cone_radius,
//                                 root_radius = root_radius,
//                                 base_radius = base_radius,
//                                 outer_radius = outer_radius,
//                                 pitch_apex = pitch_apex,
//                                 cone_distance = cone_distance,
//                                 half_thick_angle = half_thick_angle,
//                                 involute_facets = involute_facets);
//                         }
//                     }
//                 }
// 
//                 if (finish == bevel_gear_back_cone)
//                 {
//                     translate ([0,0,-back_cone_descent])
//                     cylinder (
//                         $fn=number_of_teeth*2, 
//                         r1=back_cone_end_radius,
//                         r2=back_cone_full_radius*2,
//                         h=apex_to_apex + back_cone_descent);
//                 }
//                 else
//                 {
//                     translate ([-1.5*outside_pitch_radius,-1.5*outside_pitch_radius,0])
//                     cube ([3*outside_pitch_radius,
//                         3*outside_pitch_radius,
//                         bevel_gear_flat_height]);
//                 }
//             }
//             
//             if (finish == bevel_gear_back_cone)
//             {
//                 translate ([0,0,-face_cone_descent])
//                 cylinder (
//                     r1=face_cone_end_radius,
//                     r2=face_cone_full_radius * 2,
//                     h=face_cone_height + face_cone_descent+pitch_apex);
//             }
// 
//             translate ([0,0,pitch_apex - apex_to_apex])
//             cylinder (r=bore_diameter/2,h=apex_to_apex);
//         }	

        TopoDS_Shape::operator=( result );
    }

};






    
defineType(BevelGear);
addToFactoryTable(Feature, BevelGear);




BevelGear::BevelGear()
{}




BevelGear::BevelGear(ScalarPtr m, ScalarPtr z, ScalarPtr t, ScalarPtr clearance)
: m_(m), z_(z), t_(t), clearance_(clearance)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=m_->value();
  h+=z_->value();
  h+=t_->value();
  h+=clearance_->value();
}




BevelGear::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape());
}




FeaturePtr BevelGear::create(ScalarPtr m, ScalarPtr z, ScalarPtr t, ScalarPtr clearance)
{
    return FeaturePtr(new BevelGear(m, z, t, clearance));
}




void BevelGear::build()
{
//     involute_bevel_gear_tooth g( 100, 90, 92, 110, 0, 0, 5*M_PI/180. );
    bevel_gear g;
//     refvalues_["pitch_radius"] = g.pitch_radius;
//     refvalues_["thick_angle"] = g.half_thick_angle*2.;
//     
//     providedSubshapes_["pitch_circle"]=Circle::create( vec3const(0,0,0), vec3const(0,0,1), scalarconst(g.pitch_diameter));
    
    setShape( g );

}




void BevelGear::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "BevelGear",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_scalarExpression > ',' 
      > ruleset.r_scalarExpression > ',' 
      > ruleset.r_scalarExpression
      > ( ( ',' > ruleset.r_scalarExpression ) | (qi::attr(scalarconst(0.2))) )
      > ')'
    ) 
	[ qi::_val = phx::bind(&BevelGear::create, qi::_1, qi::_2, qi::_3, qi::_4) ]
      
    ))
  );
}
   



FeatureCmdInfoList BevelGear::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "BevelGear",
         
            "( <scalar:m>, <scalar:z>, <scalar:t>[, <scalar:clearance>] )",
         
            "Creates a spur gear with thickness t."
            " The diameter follows from module m and tooth number z. Optionally, the clearance (as a fraction of module) can be specified. If not specified, 0.2 is used."
        )
    );
}
   
   

}
}
