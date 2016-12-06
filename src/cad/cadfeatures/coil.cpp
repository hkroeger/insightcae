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

#include <cmath>

#include "coil.h"
#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

#include "Geom_TrimmedCurve.hxx"
#include "GC_MakeSegment.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    


defineType(CoilPath);
addToFactoryTable(Feature, CoilPath);




CoilPath::CoilPath()
: Feature()
{
}




CoilPath::CoilPath
(
    ScalarPtr l,
    ScalarPtr dcore,
    ScalarPtr n,
    ScalarPtr d,
    ScalarPtr R,
    ScalarPtr rmin,
    ScalarPtr nl,
    ScalarPtr dr
)
: l_(l), dcore_(dcore), n_(n), d_(d), R_(R), rmin_(rmin), nl_(nl), dr_(dr)
{}




FeaturePtr CoilPath::create
(
    ScalarPtr l,
    ScalarPtr dcore,
    ScalarPtr n,
    ScalarPtr d,
    ScalarPtr R,
    ScalarPtr rmin,
    ScalarPtr nl,
    ScalarPtr dr
)
{
    return FeaturePtr(new CoilPath(l, dcore, n, d, R, rmin, nl, dr));
}




Handle_Geom_TrimmedCurve MakeArc_Projected ( gp_Pnt p1, gp_Vec n1, gp_Pnt p2, double R1, double R2, gp_Pnt pc, gp_Vec el, double rmin )
{
    std::vector<Handle_Geom_Curve> ocrvs;
    double dist=p1.Distance(p2);
    gp_Vec eel = gp_Vec(p2.XYZ()-p1.XYZ()).Normalized(); // direction vector p1 => p2

    gp_Pnt pe1(p1.Translated(eel*rmin + n1*rmin));
    gp_Pnt pe2(p2.Translated(-eel*rmin + n1*rmin));
    
    ocrvs.push_back(GC_MakeArcOfCircle ( p1, n1, pe1 ).Value());
    if (dist>2.*rmin)
    {
        ocrvs.push_back(GC_MakeSegment(pe1, pe2).Value());
    }
    ocrvs.push_back(GC_MakeArcOfCircle ( p2, n1, pe2 ).Value()->Reversed());


    int n=10;
    TColgp_Array1OfPnt pts ( 1, ocrvs.size()*n - (ocrvs.size()-1));
    
    int j=1;
    for (int l=0; l<ocrvs.size(); l++)
    {
        const Handle_Geom_Curve& ocrv = ocrvs[l];
        for ( int i=0; i<n; i++ ) {
//             if ( ! ((l>0)&&(l<ocrvs.size()-1)&&((i==0)||(i==n-1))) ) {
            if ( ! ((l>0)&&((i==0))) ) {
                double c = ( double ( i ) /double ( n-1 ) ) * ( ocrv->LastParameter() - ocrv->FirstParameter() ) + ocrv->FirstParameter();
                gp_Pnt p=ocrv->Value ( c );
                 
                double x=eel.XYZ().Dot(p.XYZ()-p1.XYZ())/dist; // project on connection line
                double Rcur=x*R2+(1.-x)*R1;
                
                gp_Pnt p0cur ( pc.XYZ() + ( ( p.XYZ()-pc.XYZ() ).Dot ( el.XYZ() ) *el.XYZ() ) );
                gp_Pnt pproj ( p0cur.XYZ() + ( p.XYZ()-p0cur.XYZ() ).Normalized() *Rcur );
                pts.SetValue ( j++, pproj );
                
//                 std::cerr<<"j="<<j<<":"<<pproj.X()<<" "<<pproj.Y()<<" "<<pproj.Z()<<std::endl;
            }
        }
    }
    
    GeomAPI_PointsToBSpline ipol ( pts, 2, 3 );
    return Handle_Geom_TrimmedCurve ( new Geom_TrimmedCurve ( ipol.Curve(), ipol.Curve()->FirstParameter(), ipol.Curve()->LastParameter() ) );
}




// #define MCOMP

void CoilPath::build()
{

    // some sanity checks
    double l=l_->value();
    if ( l<=0 ) {
        throw insight::Exception ( str ( format ( "Negative coil length (L=%g) is invalid!" ) %l ) );
    }
    
    double dcore=dcore_->value();
    if ( dcore<=0 ) {
        throw insight::Exception ( str ( format ( "Negative coil core width (dcore=%g) is invalid!" ) %dcore ) );
    }
    
    double d=d_->value();
    if ( d<=0 ) {
        throw insight::Exception ( str ( format ( "Negative conductor distance (d=%g) is invalid!" ) %d ) );
    }
    
    double nrd=n_->value();
    if ( fabs ( nrd-round ( nrd ) ) > 0 ) {
        throw insight::Exception ( str ( format ( "number of turn has to be integer! (n=%g)" ) %nrd ) );
    }
    int nr=int ( nrd );
    
    double R=R_->value();
    if ( R<=0 ) {
        throw insight::Exception ( str ( format ( "Negative yoke radius radius (R=%g) is invalid!" ) %R ) );
    }
    
    double rmin=rmin_->value();
    if ( rmin<=0 ) {
        throw insight::Exception ( str ( format ( "Negative coil bending radius (rmin=%g) is invalid!" ) %rmin ) );
    }
    if ( (dcore - 2*rmin)<0 ) {
        throw insight::Exception ( str ( format ( "Core width must be larger than 2x coil bending radius (dcore=%g, rmin=%g)!" ) %dcore % rmin ) );
    }
        
    double nld=nl_->value();
    if ( fabs ( nld-round ( nld ) ) > 0 ) {
        throw insight::Exception ( str ( format ( "number of layers has to be integer! (n=%g)" ) %nld ) );
    }
    int nl=int ( nld );
    
    double dr=0.0;
    if (nl>1)
    {
        if ( !dr_ ) {
            throw insight::Exception ( str ( format ( "Multiple layers were requested (nl=%d) but radial spacing is undefined!" ) %nl ) );
        }       
        else
        {
            dr=dr_->value();
        }
    }

#ifndef MCOMP
    TopTools_ListOfShape edgs;
#define INS_ADDWIRE(x) edgs.Append(x)
#else
    BRep_Builder bb;
    TopoDS_Compound result;
    bb.MakeCompound ( result );
#define INS_ADDWIRE(x) bb.Add(result, x)
#endif

    arma::mat el=vec3 ( 0,0,1 );
    arma::mat er=vec3 ( 1,0,0 );
    arma::mat et=arma::cross ( el, er );

    arma::mat p0=vec3 ( 0,0,0 );
    arma::mat pc=p0-R*er;
    arma::mat L=l*el;

    arma::mat l_ps_n;
    double l_R;
    double curR=R;
    double direction=1.;
    for ( int j=0; j<nr; j++ ) {
        
        double dc=0.5*dcore + double ( j+1 ) *d; // current distance from CL
        double rminc = rmin + double ( j+1 ) *d; // current bending radius
        
        for (int k=0; k<nl; k++)
        {
            if (k>0) curR += direction*dr;
            double phic=::asin ( dc / curR );
            arma::mat ps_p=rotMatrix ( phic, el ) * ( p0+curR*er ) - R*er;
            arma::mat ps_n=rotMatrix ( -phic, el ) * ( p0+curR*er ) - R*er;

            INS_ADDWIRE ( BRepBuilderAPI_MakeEdge ( GC_MakeSegment ( to_Pnt ( ps_p ), to_Pnt ( ps_p+L ) ).Value() ).Edge() );

            if ( (j==0) && (k==0) ) {
                refpoints_["p0"]=ps_p;
            } else {
                INS_ADDWIRE ( BRepBuilderAPI_MakeEdge ( MakeArc_Projected ( to_Pnt ( ps_p ), to_Vec ( -el ), to_Pnt ( l_ps_n ), curR, l_R, to_Pnt ( pc ), to_Vec ( el ), rminc ) ) );
            }

            INS_ADDWIRE ( BRepBuilderAPI_MakeEdge ( MakeArc_Projected ( to_Pnt ( ps_p+L ), to_Vec ( el ), to_Pnt ( ps_n+L ), curR, curR, to_Pnt ( pc ), to_Vec ( el ), rminc ) ) );

            INS_ADDWIRE ( BRepBuilderAPI_MakeEdge ( GC_MakeSegment ( to_Pnt ( ps_n+L ), to_Pnt ( ps_n ) ).Value() ).Edge() );

            if ( (j==nr-1) && (k==nl-1) ) {
                refpoints_["p1"]=ps_n;
            }

            l_ps_n=ps_n;
            l_R=curR;
        }
        direction*=-1.;
    }

#ifndef MCOMP
    BRepBuilderAPI_MakeWire wb;
    wb.Add ( edgs );
    setShape ( wb.Wire() );
#else
    setShape ( result );
#endif
#undef INS_ADDWIRE
}




void CoilPath::insertrule ( parser::ISCADParser& ruleset ) const
{
    ruleset.modelstepFunctionRules.add
    (
        "CoilPath",
        typename parser::ISCADParser::ModelstepRulePtr ( new typename parser::ISCADParser::ModelstepRule (
                    ( '('
                      > ruleset.r_scalarExpression > ','
                      > ruleset.r_scalarExpression > ','
                      > ruleset.r_scalarExpression > ','
                      > ruleset.r_scalarExpression > ','
                      > ruleset.r_scalarExpression > ','
                      > ruleset.r_scalarExpression > ','
                      > ruleset.r_scalarExpression > ',' 
                      > ruleset.r_scalarExpression
                      > ')' )
                    [ qi::_val = phx::bind ( &CoilPath::create, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6, qi::_7, qi::_8 ) ]
                ) )
    );
}




FeatureCmdInfoList CoilPath::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "CoilPath",
         
            "( <scalar:l>, <scalar:dcore>, <scalar:nr>, <scalar:d>, <scalar:R>, <scalar:rmin>, <scalar:nl>, <scalar:dr> )",
         
            "Creates a wire which represents the path of a coil in an electric motor. The straight part of the windings has length l, the core width is dcore, the number of turns nr and the wire distance (approx. equal to wire diameter) is d. The coil is wound on a yoke of radius R. The smallest bending radius is rmin. Multiple radial layers are activated by giving a value for nl. Radial spacing between layers is then dr.\n\n"
            "The motor axis is along EZ while the radial direction is EX."
        )
    );
}




bool CoilPath::isSingleCloseWire() const
{
  return false;
}




bool CoilPath::isSingleOpenWire() const
{
  return true;
}


    
    

defineType(Coil);
addToFactoryTable(Feature, Coil);

Coil::Coil()
: Feature()
{
}


Coil::Coil
(
  VectorPtr p0,
  VectorPtr b,
  VectorPtr l,
  ScalarPtr r,
  ScalarPtr d,
  ScalarPtr nv, 
  ScalarPtr nr
)
: p0_(p0), b_(b), l_(l), r_(r), d_(d), nv_(nv), nr_(nr)
{}

void Coil::build()
{
  
  arma::mat p0=p0_->value();
  arma::mat b=b_->value();
  arma::mat l=l_->value();
  arma::mat eb=b/arma::norm(b,2);
  arma::mat el=l/arma::norm(l,2);
  arma::mat ez=arma::cross(el, eb);
  
  double r=r_->value();
  double d=d_->value();
  
  if (r>norm(b,2)/2)
    throw insight::Exception("Invalid definition: coil bending radius r>width/2!");
  
  double cb=norm(b,2)+d;
  arma::mat lp0=p0+0.5*l+0.5*cb*eb;
  
  BRepBuilderAPI_MakeWire wb;

  int vdir=-1;
  int nr=int(nr_->value());
  int nv=int(nv_->value());
  double cl=norm(l,2);
  
  for (int j=0; j<nr; j++)
  {
    vdir*=-1;
    for (int i=0; i<nv; i++)
    {
      
      arma::mat p[9], m[4];
      p[0]=lp0;
      
      p[1]=p[0]-cl*el;
      m[1]=p[1]-r*eb;
      p[2]=m[1]-r*el;
      
      p[4]=p[1]-cb*eb;
      m[2]=p[4]+r*eb;
      p[3]=m[2]-r*el;
      
      arma::mat ebu=eb;
      double fac=1.;
      if (i<(nv-1)) 
      {
	ebu=cb*eb+vdir*d*ez;
	fac=1./::cos(::atan(d/cb));
      }
      else
      {
	cb+=0.5*d;
	cl+=0.5*d;
      }
      ebu/=norm(ebu,2);
      p[5]=p[4]+cl*el;
      m[3]=p[5]+(r*fac)*ebu;
      p[6]=m[3]+r*el;
      p[7]=p[6]+((cb-2*r)*fac)*ebu;
      m[0]=p[7]-r*el;
      if (i==(nv-1)) 
      {
	p[7]+=d*ebu;
	cb+=0.5*d;
	cl+=0.5*d;
      }
      
      
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p[0]), to_Pnt(p[1])).Value()).Edge());
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(to_Pnt(p[1]), to_Vec(-el), to_Pnt(p[2])).Value()));
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p[2]), to_Pnt(p[3])).Value()).Edge());
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(to_Pnt(p[4]), to_Vec(-el), to_Pnt(p[3])).Value()));
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p[4]), to_Pnt(p[5])).Value()).Edge());
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(to_Pnt(p[5]), to_Vec(el), to_Pnt(p[6])).Value()));
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p[6]), to_Pnt(p[7])).Value()).Edge());
      
      arma::mat np0=m[0]+r*ebu;
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(to_Pnt(p[7]), to_Vec(ebu), to_Pnt(np0)).Value()));
      lp0=np0;
  //     cb+=d;
    }
  }
  
  setShape(wb.Wire());
}

void Coil::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Coil",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 
      ( '(' > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_scalarExpression > ',' 
	    > ruleset.r_scalarExpression > ',' 
	    > ruleset.r_scalarExpression > ',' 
	    > ruleset.r_scalarExpression > ')' ) 
	[ qi::_val = phx::construct<FeaturePtr>(phx::new_<Coil>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6, qi::_7)) ]
    ))
  );
}

bool Coil::isSingleCloseWire() const
{
  return false;
}

bool Coil::isSingleOpenWire() const
{
  return true;
}

}
}
