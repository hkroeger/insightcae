
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

double involute_intersect_angle (double base_radius, double radius)
{
    return ::sqrt (::pow (radius/base_radius, 2) - 1.);
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

TopoDS_Face involute_gear_tooth
(
    double pitch_radius =110,
    double root_radius = 105,
    double base_radius = 100,
    double outer_radius =120,
    double half_thick_angle = 15.*M_PI/180.,
    int resolution=10
)
{
	double min_radius = std::max (base_radius,root_radius);

	arma::mat pitch_point = involute (base_radius, involute_intersect_angle (base_radius, pitch_radius));
	double pitch_angle = ::atan2 (pitch_point[1], pitch_point[0]);
	double centre_angle = pitch_angle + half_thick_angle;

	double start_angle = involute_intersect_angle (base_radius, min_radius);
	double stop_angle = involute_intersect_angle (base_radius, outer_radius);
    
    Handle_TColgp_HArray1OfPnt pts1 = new TColgp_HArray1OfPnt( 1, resolution );
    Handle_TColgp_HArray1OfPnt pts2 = new TColgp_HArray1OfPnt( 1, resolution );

    //     GeomAPI_PointsToBSpline splbuilder ( pts_col );
//     BRep_Builder bb;
//     TopoDS_Compound result;
//     bb.MakeCompound ( result );
    TopTools_ListOfShape edgs;
    
    for (int i=0; i<resolution; i++)
    {
        arma::mat point=involute (base_radius,start_angle+(stop_angle - start_angle)* double(i)/double(resolution-1) );
        
        arma::mat side1_point=rotate_point (centre_angle, point);
        arma::mat side2_point=mirror_point (side1_point);
        
        std::cout<<i<<side1_point<<side2_point<<std::endl;
        
        pts1->SetValue ( i+1, gp_Pnt( side1_point[0], side1_point[1], 0 ) );
        pts2->SetValue ( i+1, gp_Pnt( side2_point[0], side2_point[1], 0 ) );
        
        if (i==0)
        {
            GC_MakeArcOfCircle arc(
                gp_Pnt(side1_point[0], side1_point[1], 0), 
                gp_Pnt(min_radius, 0, 0), 
                gp_Pnt(side2_point[0], side2_point[1], 0)
                );
            edgs.Append(BRepBuilderAPI_MakeEdge ( arc.Value() ));
//             bb.Add ( result, BRepBuilderAPI_MakeEdge ( arc.Value() ) );
        }
        else if (i==resolution-1)
        {
            GC_MakeArcOfCircle arc(
                gp_Pnt(side1_point[0], side1_point[1], 0), 
                gp_Pnt(outer_radius, 0, 0), 
                gp_Pnt(side2_point[0], side2_point[1], 0)
                );
            edgs.Append(BRepBuilderAPI_MakeEdge ( arc.Value() ));
//             bb.Add ( result, BRepBuilderAPI_MakeEdge ( arc.Value() ) );
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
    
    return fb.Face();
}


TopoDS_Shape gear_shape
(
  int number_of_teeth,
  double pitch_radius,
  double root_radius,
  double base_radius,
  double outer_radius,
  double half_thick_angle,
  int resolution
)
{
  BRep_Builder bb;
  TopoDS_Compound result;
  bb.MakeCompound ( result );

//     rotate (half_thick_angle) circle ($fn=number_of_teeth*2, r=root_radius);

  for ( int i = 0; i<number_of_teeth; i++ )
    {
      gp_Trsf tr;
      tr.SetRotation ( gp_Ax1 ( gp_Pnt ( 0,0,0 ), gp_Vec ( 0,0,1 ) ), i*2.*M_PI/double ( number_of_teeth ) );
      bb.Add (
        result,
        BRepBuilderAPI_Transform
        (
          involute_gear_tooth
          (
            pitch_radius,
            root_radius,
            base_radius,
            outer_radius,
            half_thick_angle,
            resolution
          ),
          tr
        ).Shape() );
    }

  return result;
}


TopoDS_Shape gear 
(
	int number_of_teeth=17,
	double pitch=500.*M_PI/180., 
    bool is_diametral_pitch=false,
	double pressure_angle=28.*M_PI/180.,
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
{
// 	if (is_circular_pitch==false && is_diametral_pitch==false) 
// 		throw insight::Exception("Error: gear module needs either a diametral_pitch or circular_pitch");

	//Convert diametrial pitch to our native circular pitch
	double circular_pitch = (is_diametral_pitch==false)?pitch:M_PI/pitch;

	// Pitch diameter: Diameter of pitch circle.
	double pitch_diameter  =  number_of_teeth * circular_pitch / M_PI;
	double pitch_radius = pitch_diameter/2.;
	std::cout<<"Teeth:"<< number_of_teeth<< " Pitch radius:"<< pitch_radius<<std::endl;

	// Base Circle
	double base_radius = pitch_radius*::cos(pressure_angle);

	// Diametrial pitch: Number of teeth per unit length.
	double pitch_diametrial = number_of_teeth / pitch_diameter;

	// Addendum: Radial distance from pitch circle to outside circle.
	double addendum = 1./pitch_diametrial;

	//Outer Circle
	double outer_radius = pitch_radius+addendum;

	// Dedendum: Radial distance from pitch circle to root diameter
	double dedendum = addendum + clearance;

	// Root diameter: Diameter of bottom of tooth spaces.
	double root_radius = pitch_radius-dedendum;
	double backlash_angle = backlash / pitch_radius;
	double half_thick_angle = (2.*M_PI / number_of_teeth - backlash_angle) / 4.;

	// Variables controlling the rim.
	double rim_radius = root_radius - rim_width;

	// Variables controlling the circular holes in the gear.
	double circle_orbit_diameter=hub_diameter/2.+rim_radius;
	double circle_orbit_curcumference=M_PI*circle_orbit_diameter;

	// Limit the circle size to 90% of the gear face.
	double circle_diameter=
		min (
			0.70*circle_orbit_curcumference/circles,
			(rim_radius-hub_diameter/2.)*0.9);


    TopoDS_Shape s1=BRepPrimAPI_MakePrism
    (
        gear_shape 
        (
            number_of_teeth,
            pitch_radius,
            root_radius,
            base_radius,
            outer_radius,
            half_thick_angle,
            resolution
        ),
        gp_Vec(0,0,rim_thickness)
    ).Shape();
// 				linear_extrude (height=rim_thickness, convexity=10, twist=twist)

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
                    tr2.Multiplied(tr1)
                ).Shape()
            );
        }
    }
    
    return s1;

}


void ExternalGear::build()
{
    

    setShape(gear());

//     bb.Add ( result, BRepBuilderAPI_MakeEdge ( crv1, crv1->FirstParameter(), crv1->LastParameter() ) );
//     bb.Add ( result, BRepBuilderAPI_MakeEdge ( crv2, crv2->FirstParameter(), crv2->LastParameter() ) );
    
//     setShape(result);

 /*
    double phi=0.0;
    bool split=true;
    double Z=z_->value();
    
    //# ****** external gear specifications
    double addendum = m_->value();//              # distance from pitch circle to tip circle
    double dedendum = 1.25 * m_->value(); //         # pitch circle to root, sets clearance
    double clearance = dedendum - addendum;

    //# Calculate radii
    double Rpitch = Z * m_->value() / 2.; //            # pitch circle radius
    double Rb = Rpitch*::cos(phi * M_PI / 180.); //  # base circle radius
    double Ra = Rpitch + addendum; //    # tip (addendum) circle radius
    double Rroot = Rpitch - dedendum; // # root circle radius
    double fRad = 1.5 * clearance; // # fillet radius, max 1.5*clearance
    double Rf = ::sqrt((Rroot + fRad)**2 - fRad**2); // # radius at top of fillet
    if (Rb < Rf)
        Rf = Rroot + clearance

    //# ****** calculate angles (all in radians)
    double pitchAngle = 2. * M_PI / Z; //  # angle subtended by whole tooth (rads)
    double baseToPitchAngle = (::sqrt(Rpitch*Rpitch - Rb*Rb) / Rb) - ::acos(Rb / Rpitch); //genInvolutePolar(Rb, Rpitch)
    double pitchToFilletAngle = baseToPitchAngle; //  # profile starts at base circle
    if (Rf > Rb) //        # start profile at top of fillet (if its greater)
        pitchToFilletAngle -= (::sqrt(Rf*Rf - Rb*Rb) / Rb) - ::acos(Rb / Rf); //genInvolutePolar(Rb, Rf);

    double filletAngle = ::atan(fRad / (fRad + Rroot)); //  # radians

    //# ****** generate Higuchi involute approximation
    double fe = 1.; //       # fraction of profile length at end of approx
    double fs = 0.01; //    # fraction of length offset from base to avoid singularity
    if (Rf > Rb)
        fs = (Rf*Rf - Rb*Rb) / (Ra*Ra - Rb*Rb); //  # offset start to top of fillet

    if (split)
    {
        //# approximate in 2 sections, split 25% along the involute
        double fm = fs + (fe - fs) / 4.; //   # fraction of length at junction (25% along profile)
        dedInv = BezCoeffs(m, Z, phi, 3, fs, fm)
        addInv = BezCoeffs(m, Z, phi, 3, fm, fe)

        # join the 2 sets of coeffs (skip duplicate mid point)
        inv = dedInv + addInv[1:]
    }
    else
    {
        inv = BezCoeffs(m, Z, phi, 4, fs, fe)
    }
     # create the back profile of tooth (mirror image)
    invR = []
    for i, pt in enumerate(inv):
        # rotate all points to put pitch point at y = 0
        ptx, pty = inv[i] = rotate(pt, -baseToPitchAngle - pitchAngle / 4)
        # generate the back of tooth profile nodes, mirror coords in X axis
        invR.append((ptx, -pty))

    # ****** calculate section junction points R=back of tooth, Next=front of next tooth)
    fillet = toCartesian(Rf, -pitchAngle / 4 - pitchToFilletAngle) # top of fillet
    filletR = [fillet[0], -fillet[1]]   # flip to make same point on back of tooth
    rootR = toCartesian(Rroot, pitchAngle / 4 + pitchToFilletAngle + filletAngle)
    rootNext = toCartesian(Rroot, 3 * pitchAngle / 4 - pitchToFilletAngle - filletAngle)
    filletNext = rotate(fillet, pitchAngle)  # top of fillet, front of next tooth

    # Build the shapes using FreeCAD.Part
    t_inc = 2.0 * pi / float(Z)
    thetas = [(x * t_inc) for x in range(Z)]

    w.move(fillet) # start at top of fillet

    for theta in thetas:
        w.theta = theta
        if (Rf < Rb):
            w.line(inv[0]) # line from fillet up to base circle

        if split:
            w.curve(inv[1], inv[2], inv[3])
            w.curve(inv[4], inv[5], inv[6])
            w.arc(invR[6], Ra, 1) # arc across addendum circle
            w.curve(invR[5], invR[4], invR[3])
            w.curve(invR[2], invR[1], invR[0])
        else:
            w.curve(*inv[1:])
            w.arc(invR[-1], Ra, 1) # arc across addendum circle
            w.curve(*invR[-2::-1])

        if (Rf < Rb):
            w.line(filletR) # line down to topof fillet

        if (rootNext[1] > rootR[1]):    # is there a section of root circle between fillets?
            w.arc(rootR, fRad, 0) # back fillet
            w.arc(rootNext, Rroot, 1) # root circle arc

        w.arc(filletNext, fRad, 0)

    w.close()
 
  refpoints_["p0"]=p0_->value();
  
  BRepBuilderAPI_MakeWire w;
  w.Add(BRepBuilderAPI_MakeEdge(c));
  
  setShape(BRepBuilderAPI_MakeFace(w.Wire()));*/
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
