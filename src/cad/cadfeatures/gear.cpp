
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
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {



    
defineType(ExternalGear);
addToFactoryTable(Feature, ExternalGear, NoParameters);




ExternalGear::ExternalGear(const NoParameters&)
{}




ExternalGear::ExternalGear(VectorPtr p0, VectorPtr n, ScalarPtr m, ScalarPtr z)
: p0_(p0), n_(n), m_(m), z_(z)
{}




ExternalGear::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape());
}




FeaturePtr ExternalGear::create(VectorPtr p0, VectorPtr n, ScalarPtr m, ScalarPtr z)
{
    return FeaturePtr(new ExternalGear(p0, n, m, z));
}




void ExternalGear::build()
{
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
