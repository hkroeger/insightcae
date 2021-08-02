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

#include "blockmesh_cylwedgeortho.h"

#include "base/boost_include.h"

#include "base/units.h"

#include "occinclude.h"
#include "geotest.h"
#include "GeomAPI_IntCS.hxx"
#include "Geom_CylindricalSurface.hxx"
#include "cadfeatures/importsolidmodel.h"
#include "cadfeatures/splinecurve.h"
#include "cadfeatures/wire.h"
#include "cadfeatures/cylinder.h"
#include "cadfeatures/cutaway.h"
#include "cadfeatures/compound.h"
#include "cadfeatures/booleansubtract.h"

#include "GeomAPI_ExtremaCurveSurface.hxx"
#include "BRepBuilderAPI_NurbsConvert.hxx"

#include "qmodeltree.h"

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace insight
{

namespace bmd
{


defineType ( blockMeshDict_CylWedgeOrtho );
addToOpenFOAMCaseElementFactoryTable(blockMeshDict_CylWedgeOrtho );




blockMeshDict_CylWedgeOrtho::blockMeshDict_CylWedgeOrtho ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c, ps ), p_ ( ps )
{}




struct Gusset
{
  enum CollapsePoint { Fwd, Rvs, None } collapse_pt_loc;
  double fwd_u0, fwd_u1;
  gp_Pnt ctr, interf;
  double rvs_u0, rvs_u1;

  Gusset()
  {
    collapse_pt_loc=None;
  }
};




struct RegularBlock
{
  double fwd_u0, fwd_u1;
  double rvs_u0, rvs_u1;
  gp_Pnt interf0, interf1;
};




bmd::SplineEdge* createEdgeAlongCurve
(
    Handle_Geom_Curve& crv,
    double t0, double t1,
    std::function<gp_Pnt(const gp_Pnt&)> trsf = [](const gp_Pnt& p) { return p; },
    int np=50
)
{
  bmd::PointList pts;
  for (int i=0; i<np; i++)
  {
    double t = t0 + (t1-t0)*double(i)/double(np-1);
    pts.push_back( vec3( trsf(crv->Value(t)) ) );
  }

  return new bmd::SplineEdge(pts);
}




void blockMeshDict_CylWedgeOrtho::insertBlocks
(
    Handle_Geom_Curve spine_rvs,  // first param: inside@r0, last param: outside@r1
    double t0, double t1,
    double angle,
    gp_Vec ez, gp_Pnt center,
    double z0, double z1,
    Patches& pc,
    const Parameters::geometry_type::inner_interface_type& pro_inner,
    const Parameters::geometry_type::outer_interface_type& pro_outer,
    bool no_top_edg,
    bool is_lowest,
    int nuBy2, int nx, int nr, double deltax
)
{
  insight::CurrentExceptionContext ex(boost::str(boost::format(
     "inserting blocks between z0=%g and z1=%g"
    ) % z0 % z1) );

  bool is_highest = !no_top_edg;

  auto cyl_isec = [&](double r, Handle_Geom_Curve spine, gp_Pnt nearp)
  {
    insight::CurrentExceptionContext ex(boost::str(boost::format(
       "determining point on spine for radius r=%g (endpoints %s and %s); starting at point %s"
      ) % r
        % vector_to_string( vec3(spine->Value(spine->FirstParameter())) )
        % vector_to_string( vec3(spine->Value(spine->LastParameter())) )
        % vector_to_string( vec3(nearp) )
      ), false
    );

    gp_Ax3 cyl_cs(center, ez);
    Handle_Geom_Surface cyl(new Geom_CylindricalSurface(cyl_cs, r));

    double t1;

    auto searchIntersection = [&]()
    {
      GeomAPI_ExtremaCurveSurface ecs1(spine, cyl);
      double dist=1e100;
      int ii=-1;
      gp_Pnt isecp;
      for (int i=1; i<=ecs1.NbExtrema(); i++)
      {
        gp_Pnt pc, ps;
        ecs1.Points(i, pc, ps);
        std::cout<<ecs1.Distance(i)<<" pc="<<vector_to_string(vec3(pc))<<" ps="<<vector_to_string(vec3(ps))<<std::endl;
        if (ecs1.Distance(i)<1e-6)
        {
          gp_Pnt p2;
          ecs1.Points(i, isecp, p2);
          if (nearp.Distance(isecp)<dist)
          {
            dist=nearp.Distance(isecp);
            ii=i;
          }
        }
      }
      if (ii<0)
      {
  //      BRepTools::Write( BRepBuilderAPI_MakeEdge(spine).Edge(), "dbg_spine.brep");
  //      BRepTools::Write( BRepBuilderAPI_MakeFace(cyl, 1e-7).Face(), "dbg_cyl.brep");
        throw insight::Exception("Could not find intersection between cylinder and curve!");
      }

      double dummy;
      ecs1.Parameters(ii, t1, dummy, dummy);
    };

    try
    {
      searchIntersection();
    }
    catch (const insight::Exception& e)
    {
      // try cylinder rotated
      gp_Trsf tr;
      tr.SetRotation(gp_Ax1(center, ez), 45*SI::deg);
      cyl_cs.Transform(tr);
      cyl = new Geom_CylindricalSurface(cyl_cs, r);
      searchIntersection();
    }

    return t1;
  };

  auto radius = [&](const gp_Pnt& p)
  {
    gp_XYZ R=p.XYZ()-center.XYZ();
    return ( R - ez.XYZ()*R.Dot(ez.XYZ()) ).Modulus();
  };



  ez.Normalize();

  // ===========================================================
  // ========== Geometrie Zwickel innen
  // ===========================================================

  gp_Pnt p;
  gp_Vec t;

  gp_Trsf rot_fwd, rot_fwd_ctr, rot_rvs;
  rot_fwd.SetRotation( gp_Ax1(center, ez), angle );
  rot_rvs=rot_fwd.Inverted();
  rot_fwd_ctr.SetRotation( gp_Ax1(center, ez), angle*0.5 );

  // Zwischenspeicher für Infos über zu erzeugende Blöcke
  Gusset g_begin, g_end;
  RegularBlock block;


  // Richtungssinn feststellen
  gp_XYZ R0=spine_rvs->Value(t0).XYZ()-center.XYZ();
  gp_XYZ R1=spine_rvs->Value(t1).XYZ()-center.XYZ();

  double z_spl = R0.Dot(ez.XYZ()); // delta_z center=>spline
  gp_Pnt center0 = center.Translated(z_spl*ez); // center at height of spline
  arma::mat
      vL0 = vec3(ez) * (z0-z_spl), // distance spline => z0
      vL1 = vec3(ez) * (z1-z_spl) // distance spline => z1
    ;

  R0 = R0 - ez.XYZ()*R0.Dot(ez.XYZ());
  R1 = R1 - ez.XYZ()*R1.Dot(ez.XYZ());


  double sense=1.0;
  if (R0.Modulus() > R1.Modulus())
  {
    std::swap(t0, t1);
    sense=-1.0;
  }
  double t00=t0, t10=t1;


  bool has_pro_inner=false, do_pro_inner_blocks=false, pro_inner_lo_edgs=false, pro_inner_hi_edgs=false;
  if (const auto* ii = boost::get<Parameters::geometry_type::inner_interface_extend_type>(&pro_inner))
  {
    has_pro_inner=true;
    t0 = cyl_isec( radius(spine_rvs->Value(t0))+ii->distance, spine_rvs, spine_rvs->Value(t0) );

    // recompute
    R0=spine_rvs->Value(t0).XYZ()-center.XYZ();
    R0 = R0 - ez.XYZ()*R0.Dot(ez.XYZ());

    if ( (z0 >= ii->z0) && (z1 <= ii->z1) )
    {
      do_pro_inner_blocks=true;
    }

    pro_inner_lo_edgs=true;
    pro_inner_hi_edgs=!no_top_edg;
    if (fabs(z0-ii->z0)<1e-10)
    {
      pro_inner_lo_edgs=true;
    }
    if (fabs(z1-ii->z1)<1e-10)
    {
      pro_inner_hi_edgs=true;
    }
  }


  bool has_pro_outer=false, do_pro_outer_blocks=false, pro_outer_lo_edgs=false, pro_outer_hi_edgs=false;
  if (const auto* ii = boost::get<Parameters::geometry_type::outer_interface_extend_type>(&pro_outer))
  {
    has_pro_outer=true;
    t1 = cyl_isec( radius(spine_rvs->Value(t1))-ii->distance, spine_rvs, spine_rvs->Value(t1) );

    // recompute
    R1=spine_rvs->Value(t1).XYZ()-center.XYZ();
    R1 = R1 - ez.XYZ()*R1.Dot(ez.XYZ());

    if ( (z0 >= ii->z0) && (z1 <= ii->z1) )
    {
      do_pro_outer_blocks=true;
    }

    pro_outer_lo_edgs=true;
    pro_outer_hi_edgs=!no_top_edg;
    if (fabs(z0-ii->z0)<1e-10)
    {
      pro_outer_lo_edgs=true;
    }
    if (fabs(z1-ii->z1)<1e-10)
    {
      pro_outer_hi_edgs=true;
    }
  }


  //  Normale am Anfang Spine fwd => Skalarprod mit Kreisnormale
  spine_rvs->D1(t0, p, t); t*=sense;
  gp_Vec en_circ1( R0.Normalized() );
  gp_Vec en1=ez.Crossed(t).Normalized();


  // Schnittpunkt Normale/gegenüberliegende Kurve

  auto removeInnerGusset = [&]()
  {
    cout<<"remove inner gusset"<<endl;
    g_begin.collapse_pt_loc=Gusset::None;
    g_begin.fwd_u0 = g_begin.fwd_u1 = g_begin.rvs_u0 = g_begin.rvs_u1 = t0;
    g_begin.ctr = g_begin.interf = gp_Pnt( spine_rvs->Value(t0).Transformed(rot_fwd_ctr).XYZ() );
  };

  double u_sp1;
  cout<<"Begin: scalar product = "<<en_circ1.Dot(en1)<<endl;
  if ( fabs(en_circ1.Dot(en1)) < 0.1 )
  {
    // radial auslaufend
    removeInnerGusset();
  }
  else if ( en_circ1.Dot(en1) > 0 )
  {
    //rückwärts gekrümmt

    double l=1000; //r0*sin(angle)/sin(delta-angle);
    Handle_Geom_TrimmedCurve oc(
          GC_MakeSegment(
            p.Transformed(rot_rvs),
            p.Transformed(rot_rvs).Translated(l*en1.Transformed(rot_rvs).XYZ())
          ).Value()
        );
    GeomAPI_ExtremaCurveCurve ecc(spine_rvs, oc);
    if (ecc.NbExtrema()<1)
    {
      throw insight::Exception(boost::str(boost::format(
        "No intersection between spine and normal curve found!")
                                          ));
    }
    double uoc;
    ecc.TotalLowerDistanceParameters(u_sp1, uoc);

    g_begin.collapse_pt_loc=Gusset::Rvs;
    g_begin.fwd_u0 = t0;
    g_begin.fwd_u1 = u_sp1;
    g_begin.rvs_u0 = g_begin.rvs_u1 = t0;

    g_begin.interf=gp_Pnt( 0.5*(spine_rvs->Value(t0).XYZ() + spine_rvs->Value(u_sp1).Transformed(rot_fwd).XYZ() ) );
    g_begin.ctr=gp_Pnt(
                  0.8 * ( 0.5*(g_begin.interf.XYZ() + spine_rvs->Value(t0).Transformed(rot_fwd_ctr).XYZ() ) )
                  +
                  0.2 * ( spine_rvs->Value(0.5*(t0+u_sp1)).Transformed(rot_fwd).XYZ() )
                  );

    // check
    double dr=spine_rvs->Value(t0).Distance(spine_rvs->Value(u_sp1));
    double du=spine_rvs->Value(t0).Distance( spine_rvs->Value(t0).Transformed(rot_fwd) );
    cout<<"dr="<<dr<<", du="<<du<<endl;
    if (dr/du<0.1) removeInnerGusset();
  }
  else
  {
    // vorwärts gekrümmt
    double l=1000; //r0*sin(angle)/sin(delta-angle);
    Handle_Geom_TrimmedCurve oc(
          GC_MakeSegment(
            p.Transformed(rot_fwd),
            p.Transformed(rot_fwd).Translated(-l*en1.Transformed(rot_fwd).XYZ())
          ).Value()
        );
    GeomAPI_ExtremaCurveCurve ecc(spine_rvs, oc);
    if (ecc.NbExtrema()<1)
    {
      throw insight::Exception(boost::str(boost::format(
        "No intersection between spine and normal curve found!")
                                          ));
    }
    double uoc;
    ecc.TotalLowerDistanceParameters(u_sp1, uoc);

    g_begin.collapse_pt_loc=Gusset::Fwd;
    g_begin.fwd_u0 = g_begin.fwd_u1 = t0;
    g_begin.rvs_u0 = t0;
    g_begin.rvs_u1 = u_sp1;

    g_begin.interf=gp_Pnt( 0.5*(spine_rvs->Value(t0).Transformed(rot_fwd).XYZ() + spine_rvs->Value(u_sp1).XYZ() ) );
    g_begin.ctr=gp_Pnt(
                0.8* ( 0.5*(g_begin.interf.XYZ() + spine_rvs->Value(t0).Transformed(rot_fwd_ctr).XYZ() ) )
                +
                0.2* ( spine_rvs->Value(0.5*(u_sp1 + t0)).XYZ() )
                );

    // check
    double dr=spine_rvs->Value(t0).Distance(spine_rvs->Value(u_sp1));
    double du=spine_rvs->Value(t0).Distance( spine_rvs->Value(t0).Transformed(rot_fwd) );
    cout<<"dr="<<dr<<", du="<<du<<endl;
    if (dr/du<0.1) removeInnerGusset();
  }




  // ===========================================================
  // ========== Geometrie Zwickel aussen
  // ===========================================================

  //  Normale am Anfang Spine fwd => Skalarprod mit Kreisnormale
  spine_rvs->D1(t1, p, t); t*=sense;
  gp_Vec en_circ2( R1.Normalized() );
  gp_Vec en2=ez.Crossed(t).Normalized();

  // Schnittpunkt Normale/gegenüberliegende Kurve

  double u_sp2;
  cout<<"End: scalar product = "<<en_circ2.Dot(en2)<<endl;

  auto removeOuterGusset = [&]()
  {
    cout<<"remove outer gusset"<<endl;
    g_end.collapse_pt_loc=Gusset::None;
    g_end.fwd_u0 = g_end.fwd_u1 = g_end.rvs_u0 = g_end.rvs_u1 = t1;
    g_end.ctr = g_end.interf = gp_Pnt( spine_rvs->Value(t1).Transformed(rot_fwd_ctr).XYZ() );
  };

  if ( fabs(en_circ2.Dot(en2)) < 0.1 )
  {
    // radial auslaufend
    removeOuterGusset();
  }
  else if ( en_circ2.Dot(en2) > 0 )
  {
    //rückwärts gekrümmt
    double l=1000; //r0*sin(angle)/sin(delta-angle);
    Handle_Geom_TrimmedCurve oc(
          GC_MakeSegment(
            p.Transformed(rot_fwd),
            p.Transformed(rot_fwd).Translated(-l*en2.Transformed(rot_fwd).XYZ())
          ).Value()
        );
    GeomAPI_ExtremaCurveCurve ecc(spine_rvs, oc);
    if (ecc.NbExtrema()<1)
    {
      throw insight::Exception(boost::str(boost::format(
        "No intersection between spine and normal curve found!")
                                          ));
    }
    double uoc;
    ecc.TotalLowerDistanceParameters(u_sp2, uoc);

    g_end.collapse_pt_loc=Gusset::Fwd;
    g_end.fwd_u0 = g_end.fwd_u1 = t1;
    g_end.rvs_u0 = u_sp2;
    g_end.rvs_u1 = t1;

    g_end.interf=gp_Pnt( 0.5*(spine_rvs->Value(t1).Transformed(rot_fwd).XYZ() + spine_rvs->Value(u_sp2).XYZ() ) );
    g_end.ctr=gp_Pnt(
                0.8* ( 0.5*(g_end.interf.XYZ() + spine_rvs->Value(t1).Transformed(rot_fwd_ctr).XYZ() ) )
                +
                0.2* ( spine_rvs->Value(0.5*(u_sp2 + t1)).XYZ() )
                );

    // check
    double dr=spine_rvs->Value(t1).Distance(spine_rvs->Value(u_sp2));
    double du=spine_rvs->Value(t1).Distance( spine_rvs->Value(t1).Transformed(rot_fwd) );
    cout<<"dr="<<dr<<", du="<<du<<endl;
    if (dr/du<0.1) removeOuterGusset();
  }
  else
  {
    // vorwärts gekrümmt
    double l=1000; //r0*sin(angle)/sin(delta-angle);
    Handle_Geom_TrimmedCurve oc(
          GC_MakeSegment(
            p.Transformed(rot_rvs),
            p.Transformed(rot_rvs).Translated(-l*en2.Transformed(rot_rvs).XYZ())
          ).Value()
        );
    GeomAPI_ExtremaCurveCurve ecc(spine_rvs, oc);
    if (ecc.NbExtrema()<1)
    {
      throw insight::Exception(boost::str(boost::format(
        "No intersection between spine and normal curve found!")
                                          ));
    }
    double uoc;
    ecc.TotalLowerDistanceParameters(u_sp2, uoc);

    g_end.collapse_pt_loc=Gusset::Rvs;
    g_end.rvs_u0 = g_end.rvs_u1 = t1;
    g_end.fwd_u0 = u_sp2;
    g_end.fwd_u1 = t1;

    g_end.interf=gp_Pnt( 0.5*(spine_rvs->Value(t1).XYZ() + spine_rvs->Value(u_sp2).Transformed(rot_fwd).XYZ() ) );
    g_end.ctr=gp_Pnt(
                0.8* ( 0.5*(g_end.interf.XYZ() + spine_rvs->Value(t1).Transformed(rot_fwd_ctr).XYZ() ) )
                +
                0.2* ( spine_rvs->Value(0.5*(u_sp2 + t1)).Transformed(rot_fwd).XYZ() )
                );

    // check
    double dr=spine_rvs->Value(t1).Distance(spine_rvs->Value(u_sp2));
    double du=spine_rvs->Value(t1).Distance( spine_rvs->Value(t1).Transformed(rot_fwd) );
    cout<<"dr="<<dr<<", du="<<du<<endl;
    if (dr/du<0.1) removeOuterGusset();
  }

  // Blockgeometrien
  block.fwd_u0=g_begin.fwd_u1;
  block.rvs_u0=g_begin.rvs_u1;
  block.interf0=g_begin.interf;
  block.fwd_u1=g_end.fwd_u0;
  block.rvs_u1=g_end.rvs_u0;
  block.interf1=g_end.interf;


  // Blöcke erzeugen


  // ============= Blöcke

  double dr;
  {
    CurrentExceptionContext ex(
          str(format("Creating intermediate blocks between %s and %s")%toStr(vL0)%toStr(vL1))
          );

    arma::mat
        pr0 = vec3(spine_rvs->Value(block.rvs_u0)),
        pr1 = vec3(spine_rvs->Value(block.rvs_u1)),
        prf0 = vec3(block.interf0),
        pf0 = vec3(spine_rvs->Value(block.fwd_u0).Transformed(rot_fwd)),
        pf1 = vec3(spine_rvs->Value(block.fwd_u1).Transformed(rot_fwd)),
        prf1 = vec3(block.interf1)
      ;

    dr = 0.5*
                (
                  ::insight::cad::edgeLength(BRepBuilderAPI_MakeEdge(spine_rvs, block.rvs_u0, block.rvs_u1).Edge())
                  +
                  ::insight::cad::edgeLength(BRepBuilderAPI_MakeEdge(spine_rvs, block.fwd_u0, block.fwd_u1).Edge())
                  ) / double(nr);

    {
    Block& bl = this->addBlock
                (
                    new Block ( P_8_DZ (
                                  pr0, pr1, prf1, prf0,
                                  vL0, vL1
                                ),
                                nr, nuBy2, nx
                              )
                );
    if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
    if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
    if (pc.pcyclm) pc.pcyclm->addFace(bl.face("0154"));
    }
    {
    Block& bl = this->addBlock
                (
                    new Block ( P_8_DZ (
                                  prf0, prf1, pf1, pf0,
                                  vL0, vL1
                                ),
                                nr, nuBy2, nx
                              )
                );
    if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
    if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
    if ( Patch* cp = pc.pcyclp) cp->addFace(bl.face("2376"));
    }

    auto middleCurve = [&](const PointList& c1, const PointList& c2, const arma::mat vLEnd)
                        {
                          PointList mc;
                          mc.push_back(prf0+vLEnd);
                          for (size_t i=1; i< c1.size()-1; i++)
                          {
                            mc.push_back(0.5*(c1[i]+c2[i]));
                          }
                          mc.push_back(prf1+vLEnd);
                          return mc;
                        };

    {
      auto sp1=createEdgeAlongCurve(spine_rvs, block.rvs_u0, block.rvs_u1,
                                    [&](const gp_Pnt& p) { return p.Translated(to_Vec(vL0)); } );
      this->addEdge ( sp1 );

      if (!no_top_edg)
        this->addEdge ( createEdgeAlongCurve(spine_rvs, block.rvs_u0, block.rvs_u1,
                                      [&](const gp_Pnt& p) { return p.Translated(to_Vec(vL1)); } ) );

      auto sp2=createEdgeAlongCurve(spine_rvs, block.fwd_u0, block.fwd_u1,
                                    [&](const gp_Pnt& p) { return p.Transformed(rot_fwd).Translated(to_Vec(vL0)); } );
      this->addEdge ( sp2 );

      if (!no_top_edg)
        this->addEdge ( createEdgeAlongCurve(spine_rvs, block.fwd_u0, block.fwd_u1,
                                             [&](const gp_Pnt& p) { return p.Transformed(rot_fwd).Translated(to_Vec(vL1)); } ) );

      auto sp3=new SplineEdge(middleCurve(sp1->allPoints(), sp2->allPoints(), vL0));
      this->addEdge ( sp3 );

      if (!no_top_edg)
        this->addEdge ( sp3->transformed( rotMatrix(0), vL1-vL0 ) );
    }
  }




  //  - Zwickel innen
  if (g_begin.collapse_pt_loc == Gusset::Rvs )
  {
    CurrentExceptionContext ex(
          str(format("Creating blocks at inner gusset (reverse orientation) between %s and %s")%toStr(vL0)%toStr(vL1))
          );

    gp_Pnt
        pa = spine_rvs->Value(g_begin.fwd_u0),
        pab = spine_rvs->Value((g_begin.fwd_u0+g_begin.fwd_u1)*0.5),
        pb = spine_rvs->Value(g_begin.fwd_u1)
      ;

    this->addEdge ( createEdgeAlongCurve(spine_rvs, g_begin.fwd_u0, (g_begin.fwd_u0+g_begin.fwd_u1)*0.5,
                    [&](const gp_Pnt& p) { return p.Transformed(rot_fwd).Translated(to_Vec(vL0)); } ));
    this->addEdge ( createEdgeAlongCurve(spine_rvs, (g_begin.fwd_u0+g_begin.fwd_u1)*0.5, g_begin.fwd_u1,
                    [&](const gp_Pnt& p) { return p.Transformed(rot_fwd).Translated(to_Vec(vL0)); } ));
    this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd))+vL0,
                                             vec3(pa.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(center0)+vL0 ) );
    this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(pa)+vL0,
                                             vec3(center0)+vL0 ) );

    if (!no_top_edg)
    {
      this->addEdge ( createEdgeAlongCurve(spine_rvs, g_begin.fwd_u0, (g_begin.fwd_u0+g_begin.fwd_u1)*0.5,
                      [&](const gp_Pnt& p) { return p.Transformed(rot_fwd).Translated(to_Vec(vL1)); } ));
      this->addEdge ( createEdgeAlongCurve(spine_rvs, (g_begin.fwd_u0+g_begin.fwd_u1)*0.5, g_begin.fwd_u1,
                      [&](const gp_Pnt& p) { return p.Transformed(rot_fwd).Translated(to_Vec(vL1)); } ));
      this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd))+vL1,
                                               vec3(pa.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(center0)+vL1 ) );
      this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(pa)+vL1,
                                               vec3(center0)+vL1) );
    }


    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pa.Transformed(rot_fwd) )+vL0,
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( g_begin.ctr )+vL0,
                                    vec3( pab.Transformed(rot_fwd) )+vL0,

                                    vec3( pa.Transformed(rot_fwd) )+vL1,
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( g_begin.ctr )+vL1,
                                    vec3( pab.Transformed(rot_fwd) )+vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );

      if (!do_pro_inner_blocks)
      { if ( Patch* cp = pc.inner) cp->addFace(bl.face("0154")); }

      if ( Patch* cp = pc.pcyclp) cp->addFace(bl.face("0473"));

    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pab.Transformed(rot_fwd) )+vL0,
                                    vec3( g_begin.ctr )+vL0,
                                    vec3( g_begin.interf )+vL0,
                                    vec3( pb.Transformed(rot_fwd) )+vL0,

                                    vec3( pab.Transformed(rot_fwd) )+vL1,
                                    vec3( g_begin.ctr )+vL1,
                                    vec3( g_begin.interf )+vL1,
                                    vec3( pb.Transformed(rot_fwd) )+vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.pcyclp) cp->addFace(bl.face("0473"));
    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( pa )+vL0,
                                    vec3( g_begin.interf )+vL0,
                                    vec3( g_begin.ctr )+vL0,

                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( pa )+vL1,
                                    vec3( g_begin.interf )+vL1,
                                    vec3( g_begin.ctr )+vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if (!do_pro_inner_blocks)
      { if ( Patch* cp = pc.inner) cp->addFace(bl.face("0154")); }

    }


  }
  else if (g_begin.collapse_pt_loc == Gusset::Fwd )
  {
    CurrentExceptionContext ex(
          str(format("Creating blocks at inner gusset (forward orientation) between %s and %s")%toStr(vL0)%toStr(vL1))
          );

    gp_Pnt
        pa = spine_rvs->Value(g_begin.rvs_u0),
        pab = spine_rvs->Value((g_begin.rvs_u0+g_begin.rvs_u1)*0.5),
        pb = spine_rvs->Value(g_begin.rvs_u1)
      ;

    this->addEdge ( createEdgeAlongCurve(spine_rvs, g_begin.rvs_u0, (g_begin.rvs_u0+g_begin.rvs_u1)*0.5,
                    [&](const gp_Pnt& p) { return p.Translated(to_Vec(vL0)); } ));
    this->addEdge ( createEdgeAlongCurve(spine_rvs, (g_begin.rvs_u0+g_begin.rvs_u1)*0.5, g_begin.rvs_u1,
                    [&](const gp_Pnt& p) { return p.Translated(to_Vec(vL0)); } ));
    this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd))+vL0,
                                             vec3(pa.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(center0)+vL0 ) );
    this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(pa)+vL0,
                                             vec3(center0)+vL0 ) );

    if (!no_top_edg)
    {
      this->addEdge ( createEdgeAlongCurve(spine_rvs, g_begin.rvs_u0, (g_begin.rvs_u0+g_begin.rvs_u1)*0.5,
                      [&](const gp_Pnt& p) { return p.Translated(to_Vec(vL1)); } ));
      this->addEdge ( createEdgeAlongCurve(spine_rvs, (g_begin.rvs_u0+g_begin.rvs_u1)*0.5, g_begin.rvs_u1,
                      [&](const gp_Pnt& p) { return p.Translated(to_Vec(vL1)); } ));
      this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd))+vL1,
                                               vec3(pa.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(center0)+vL1 ) );
      this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(pa)+vL1,
                                               vec3(center0)+vL1 ) );
    }

    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pa.Transformed(rot_fwd) )+vL0,
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( g_begin.ctr )+vL0,
                                    vec3( g_begin.interf )+vL0,

                                    vec3( pa.Transformed(rot_fwd) )+vL1,
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( g_begin.ctr )+vL1,
                                    vec3( g_begin.interf )+vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if (!do_pro_inner_blocks)
      { if ( Patch* cp = pc.inner) cp->addFace(bl.face("0154")); }


    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( pa )+vL0,
                                    vec3( pab )+vL0,
                                    vec3( g_begin.ctr )+vL0,

                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( pa )+vL1,
                                    vec3( pab )+vL1,
                                    vec3( g_begin.ctr )+vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if (!do_pro_inner_blocks)
      { if ( Patch* cp = pc.inner) cp->addFace(bl.face("0154")); }

      if ( Patch* cp = pc.pcyclm) cp->addFace(bl.face("1265"));
    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pab )+vL0,
                                    vec3( pb )+vL0,
                                    vec3( g_begin.interf )+vL0,
                                    vec3( g_begin.ctr )+vL0,

                                    vec3( pab )+vL1,
                                    vec3( pb )+vL1,
                                    vec3( g_begin.interf )+vL1,
                                    vec3( g_begin.ctr )+vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.pcyclm) cp->addFace(bl.face("0154"));
    }
  }
  else if (g_begin.collapse_pt_loc == Gusset::None )
  {
    CurrentExceptionContext ex(
          str(format("Creating blocks at inner gusset (radial orientation) between %s and %s")%toStr(vL0)%toStr(vL1))
          );

    gp_Pnt
        pa = spine_rvs->Value(t0)
      ;


    this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd))+vL0,
                                             vec3(pa.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(center0)+vL0 ) );
    this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(pa)+vL0,
                                             vec3(center0)+vL0 ) );

    if (!no_top_edg)
    {
      this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd))+vL1,
                                               vec3(pa.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(center0)+vL1 ) );
      this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(pa)+vL1,
                                               vec3(center0)+vL1 ) );
    }
  }

  if (do_pro_inner_blocks)
  {
    CurrentExceptionContext ex(
          str(format("Creating blocks at inner protrusion between %s and %s")%toStr(vL0)%toStr(vL1))
          );

    gp_Pnt
        pa = spine_rvs->Value(t00),
        pb = spine_rvs->Value(t0)
      ;

    int nproi = std::max(1, int( pa.Distance(pb) / dr ));
    std::cout<<"nproi="<<nproi<<" "<<pa.Distance(pb)<<" "<<dr<<std::endl;


    if (pro_inner_lo_edgs)
    {
      this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd))+vL0,
                                               vec3(pa.Transformed(rot_fwd_ctr))+vL0,
                                               vec3(center0)+vL0 ) );
      this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd_ctr))+vL0,
                                               vec3(pa)+vL0,
                                               vec3(center0)+vL0 ) );
    }
    if (pro_inner_hi_edgs)
    {
      this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd))+vL1,
                                               vec3(pa.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(center0)+vL1 ) );
      this->addEdge ( new CircularEdge_Center( vec3(pa.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(pa)+vL1,
                                               vec3(center0)+vL1 ) );
    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pa.Transformed(rot_fwd) )+vL0,
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( pb.Transformed(rot_fwd) )+vL0,

                                    vec3( pa.Transformed(rot_fwd) )+vL1,
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( pb.Transformed(rot_fwd) )+vL1
                                  ),
                                  nuBy2, nproi, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.innerif) cp->addFace(bl.face("0154"));
      if ( Patch* cp = pc.pcyclp) cp->addFace(bl.face("0473"));
    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( pa )+vL0,
                                    vec3( pb )+vL0,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL0,

                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( pa )+vL1,
                                    vec3( pb )+vL1,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL1
                                  ),
                                  nuBy2, nproi, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.innerif) cp->addFace(bl.face("0154"));
      if ( Patch* cp = pc.pcyclm) cp->addFace(bl.face("1265"));
    }
  }



  //  - Zwickel aussen
  if (g_end.collapse_pt_loc == Gusset::Fwd )
  {
    CurrentExceptionContext ex(
          str(format("Creating blocks at outer gusset (forward orientation) between %s and %s")%toStr(vL0)%toStr(vL1))
          );

    gp_Pnt
        pa = spine_rvs->Value(g_end.rvs_u0),
        pab = spine_rvs->Value((g_end.rvs_u0+g_end.rvs_u1)*0.5),
        pb = spine_rvs->Value(g_end.rvs_u1)
      ;

    this->addEdge ( createEdgeAlongCurve(spine_rvs, g_end.rvs_u0, (g_end.rvs_u0+g_end.rvs_u1)*0.5,
                    [&](const gp_Pnt& p) { return p.Translated(to_Vec(vL0)); } ));
    this->addEdge ( createEdgeAlongCurve(spine_rvs, (g_end.rvs_u0+g_end.rvs_u1)*0.5, g_end.rvs_u1,
                    [&](const gp_Pnt& p) { return p.Translated(to_Vec(vL0)); } ));
    this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd))+vL0,
                                             vec3(pb.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(center0)+vL0 ) );
    this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(pb)+vL0,
                                             vec3(center0)+vL0 ) );

    if (!no_top_edg)
    {
      this->addEdge ( createEdgeAlongCurve(spine_rvs, g_end.rvs_u0, (g_end.rvs_u0+g_end.rvs_u1)*0.5,
                      [&](const gp_Pnt& p) { return p.Translated(to_Vec(vL1)); } ));
      this->addEdge ( createEdgeAlongCurve(spine_rvs, (g_end.rvs_u0+g_end.rvs_u1)*0.5, g_end.rvs_u1,
                      [&](const gp_Pnt& p) { return p.Translated(to_Vec(vL1)); } ));
      this->addEdge ( new CircularEdge_Center(
                        vec3(pb.Transformed(rot_fwd))+vL1,
                        vec3(pb.Transformed(rot_fwd_ctr))+vL1,
                        vec3(center0)+vL1 ) );
      this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(pb)+vL1,
                                               vec3(center0)+vL1 ) );
    }

    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( g_end.interf )+vL0,
                                    vec3( g_end.ctr )+vL0,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( pb.Transformed(rot_fwd) )+vL0,

                                    vec3( g_end.interf )+vL1,
                                    vec3( g_end.ctr )+vL1,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( pb.Transformed(rot_fwd) )+vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.outer) cp->addFace(bl.face("2376"));
    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pa )+vL0,
                                    vec3( pab )+vL0,
                                    vec3( g_end.ctr )+vL0,
                                    vec3( g_end.interf )+vL0,

                                    vec3( pa )+vL1,
                                    vec3( pab )+vL1,
                                    vec3( g_end.ctr )+vL1,
                                    vec3( g_end.interf )+vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.pcyclm) cp->addFace(bl.face("0154"));
    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pab )+vL0,
                                    vec3( pb )+vL0,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( g_end.ctr )+vL0,

                                    vec3( pab )+vL1,
                                    vec3( pb )+vL1,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( g_end.ctr )+vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.pcyclm) cp->addFace(bl.face("0154"));
      if ( Patch* cp = pc.outer) cp->addFace(bl.face("1265"));
    }
  }
  else if (g_end.collapse_pt_loc == Gusset::Rvs )
  {
    CurrentExceptionContext ex(
          str(format("Creating blocks at outer gusset (reverse orientation) between %s and %s")%toStr(vL0)%toStr(vL1))
          );

    gp_Pnt
        pa = spine_rvs->Value(g_end.fwd_u0),
        pab = spine_rvs->Value((g_end.fwd_u0+g_end.fwd_u1)*0.5),
        pb = spine_rvs->Value(g_end.fwd_u1)
      ;

    this->addEdge ( createEdgeAlongCurve(spine_rvs, g_end.fwd_u0, (g_end.fwd_u0+g_end.fwd_u1)*0.5,
                    [&](const gp_Pnt& p) { return p.Transformed(rot_fwd).Translated(to_Vec(vL0)); } ));
    this->addEdge ( createEdgeAlongCurve(spine_rvs, (g_end.fwd_u0+g_end.fwd_u1)*0.5, g_end.fwd_u1,
                    [&](const gp_Pnt& p) { return p.Transformed(rot_fwd).Translated(to_Vec(vL0)); } ));
    this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd))+vL0,
                                             vec3(pb.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(center0)+vL0 ) );
    this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(pb)+vL0,
                                             vec3(center0)+vL0 ) );

    if (!no_top_edg)
    {
      this->addEdge ( createEdgeAlongCurve(spine_rvs, g_end.fwd_u0, (g_end.fwd_u0+g_end.fwd_u1)*0.5,
                      [&](const gp_Pnt& p) { return p.Transformed(rot_fwd).Translated(to_Vec(vL1)); } ));
      this->addEdge ( createEdgeAlongCurve(spine_rvs, (g_end.fwd_u0+g_end.fwd_u1)*0.5, g_end.fwd_u1,
                      [&](const gp_Pnt& p) { return p.Transformed(rot_fwd).Translated(to_Vec(vL1)); } ));
      this->addEdge ( new CircularEdge_Center(
                        vec3(pb.Transformed(rot_fwd))+vL1,
                        vec3(pb.Transformed(rot_fwd_ctr))+vL1,
                        vec3(center0)+vL1 ) );
      this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(pb)+vL1,
                                               vec3(center0)+vL1 ) );
    }

    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8_DZ (
                                    vec3( g_end.interf ),
                                    vec3( pb ),
                                    vec3( pb.Transformed(rot_fwd_ctr) ),
                                    vec3( g_end.ctr ),

                                    vL0, vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.outer) cp->addFace(bl.face("1265"));
    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8_DZ (
                                    vec3( g_end.interf ),
                                    vec3( g_end.ctr ),
                                    vec3( pab.Transformed(rot_fwd) ),
                                    vec3( pa.Transformed(rot_fwd) ),

                                    vL0, vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.pcyclp) cp->addFace(bl.face("2376"));
    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8_DZ (
                                    vec3( pab.Transformed(rot_fwd) ),
                                    vec3( g_end.ctr ),
                                    vec3( pb.Transformed(rot_fwd_ctr) ),
                                    vec3( pb.Transformed(rot_fwd) ),

                                    vL0, vL1
                                  ),
                                  nuBy2, nuBy2, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.pcyclp) cp->addFace(bl.face("0473"));
      if ( Patch* cp = pc.outer) cp->addFace(bl.face("2376"));
    }
  }
  else if (g_end.collapse_pt_loc == Gusset::None )
  {
    CurrentExceptionContext ex(
          str(format("Creating blocks at outer gusset (radial orientation) between %s and %s")%toStr(vL0)%toStr(vL1))
          );

    gp_Pnt
        pb = spine_rvs->Value(t1)
      ;

    this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd))+vL0,
                                             vec3(pb.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(center0)+vL0 ) );
    this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd_ctr))+vL0,
                                             vec3(pb)+vL0,
                                             vec3(center0)+vL0 ) );

    if (!no_top_edg)
    {

      this->addEdge ( new CircularEdge_Center(
                        vec3(pb.Transformed(rot_fwd))+vL1,
                        vec3(pb.Transformed(rot_fwd_ctr))+vL1,
                        vec3(center0)+vL1 ) );
      this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(pb)+vL1,
                                               vec3(center0)+vL1 ) );
    }

  }

  if (do_pro_outer_blocks)
  {
    CurrentExceptionContext ex(
          str(format("Creating blocks at outer protrusion between %s and %s")%toStr(vL0)%toStr(vL1))
          );

    gp_Pnt
        pa = spine_rvs->Value(t1),
        pb = spine_rvs->Value(t10)
      ;

    int nproo = std::max(1, int( pa.Distance(pb) / dr ));
    std::cout<<"nproo="<<nproo<<" "<<pa.Distance(pb)<<" "<<dr<<std::endl;

    if (pro_outer_lo_edgs)
    {
      this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd))+vL0,
                                               vec3(pb.Transformed(rot_fwd_ctr))+vL0,
                                               vec3(center0)+vL0 ) );
      this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd_ctr))+vL0,
                                               vec3(pb)+vL0,
                                               vec3(center0)+vL0 ) );
    }
    if (pro_outer_hi_edgs)
    {
      this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd))+vL1,
                                               vec3(pb.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(center0)+vL1 ) );
      this->addEdge ( new CircularEdge_Center( vec3(pb.Transformed(rot_fwd_ctr))+vL1,
                                               vec3(pb)+vL1,
                                               vec3(center0)+vL1 ) );
    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pa.Transformed(rot_fwd) )+vL0,
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( pb.Transformed(rot_fwd) )+vL0,

                                    vec3( pa.Transformed(rot_fwd) )+vL1,
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( pb.Transformed(rot_fwd) )+vL1
                                  ),
                                  nuBy2, nproo, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.outerif) cp->addFace(bl.face("2376"));
      if ( Patch* cp = pc.pcyclp) cp->addFace(bl.face("0473"));
    }
    {
      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL0,
                                    vec3( pa )+vL0,
                                    vec3( pb )+vL0,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL0,

                                    vec3( pa.Transformed(rot_fwd_ctr) )+vL1,
                                    vec3( pa )+vL1,
                                    vec3( pb )+vL1,
                                    vec3( pb.Transformed(rot_fwd_ctr) )+vL1
                                  ),
                                  nuBy2, nproo, nx
                                )
                  );
      if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
      if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
      if ( Patch* cp = pc.outerif) cp->addFace(bl.face("2376"));
      if ( Patch* cp = pc.pcyclm) cp->addFace(bl.face("1265"));
    }
  }


}

void blockMeshDict_CylWedgeOrtho::create_bmd()
{
    insight::CurrentExceptionContext("creating blockMeshDict from spine "+p_.geometry.wedge_spine_curve->fileName().string());

    this->setDefaultPatch(p_.mesh.defaultPatchName);



    // Read or create spine curve
    Handle_Geom_Curve spine;
    double t0, t1;


    // load geometry
    insight::cad::FeaturePtr wsc =
        insight::cad::Import::create( p_.geometry.wedge_spine_curve->filePath() );

    wsc->checkForBuildDuringAccess(); // force rebuild
    auto el = wsc->allEdgesSet();

    if (el.size()!=1)
      throw insight::Exception(
          boost::str(boost::format("CAD file %s should contain only one single edge! (It actually contains %d edges)")
                     % p_.geometry.wedge_spine_curve->fileName().string() % el.size() )
          );

    TopoDS_Edge e= TopoDS::Edge(BRepBuilderAPI_NurbsConvert(wsc->edge(*el.begin())).Shape());

    // == Determine characteristic lengths
    auto ew = insight::cad::CumulativeEdgeLength(wsc);
    double Lr = ew.value();

    gp_Pnt midp
        (
          0.5*
          (
            BRep_Tool::Pnt(TopExp::FirstVertex(e)).XYZ()
            +
            BRep_Tool::Pnt(TopExp::LastVertex(e)).XYZ()
          )
         );
    gp_XYZ R_midp = midp.XYZ() - to_Pnt(p_.geometry.p0).XYZ();
    gp_XYZ ez = to_Vec(p_.geometry.ex).XYZ();
    double rmid = ( R_midp - ez.Dot(R_midp)*ez ).Modulus();
    double Lu = rmid * p_.geometry.wedge_angle*SI::deg;

    double Lx = p_.geometry.L;

    int nr, nuBy2;
    double deltax;

    if (const auto* m = boost::get<Parameters::mesh_type::resolution_cubical_size_type>(&p_.mesh.resolution))
    {
      nr = std::max(1, int(Lr/m->delta));
      nuBy2 = std::max(1, int(0.5*Lu/m->delta));
      deltax = m->delta;
    }
    else if (const auto* m = boost::get<Parameters::mesh_type::resolution_cubical_type>(&p_.mesh.resolution))
    {
      double delta = std::max( Lr, std::max(Lu, Lx) ) / double(m->n_max);
      nr = std::max(1, int(Lr/delta));
      nuBy2 = std::max(1, int(0.5*Lu/delta));
      deltax = delta;
    }
    else if (const auto* m = boost::get<Parameters::mesh_type::resolution_individual_type>(&p_.mesh.resolution))
    {
      nr = m->nr;
      nuBy2 = std::max(1, m->nu/2);
      deltax = Lx/double(m->nx);
    }
    else
    {
      throw insight::Exception("Internal error: unhandled selection for mesh resolution");
    }


    spine = BRep_Tool::Curve(e, t0, t1);

    Patches pc;
    if ( p_.mesh.basePatchName!="" ) {
        pc.base=&this->addOrDestroyPatch ( p_.mesh.basePatchName, new bmd::Patch() );
    }
    if ( p_.mesh.topPatchName!="" ) {
        pc.top=&this->addOrDestroyPatch ( p_.mesh.topPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.outerPatchName!="" ) {
        pc.outer=&this->addOrDestroyPatch ( p_.mesh.outerPatchName, new bmd::Patch() );
    }
    if ( !p_.mesh.innerPatchName.empty() ) {
        pc.inner=&this->addOrDestroyPatch ( p_.mesh.innerPatchName, new bmd::Patch() );
    }
    if ( !p_.mesh.outerInterfacePatchName.empty() ) {
        pc.outerif=&this->addOrDestroyPatch ( p_.mesh.outerInterfacePatchName, new bmd::Patch() );
    }
    if ( !p_.mesh.innerInterfacePatchName.empty() ) {
        pc.innerif=&this->addOrDestroyPatch ( p_.mesh.innerInterfacePatchName, new bmd::Patch() );
    }
    if ( !p_.mesh.cyclmPatchName.empty() ) {
        pc.pcyclm=&this->addOrDestroyPatch ( p_.mesh.cyclmPatchName, new bmd::Patch() );
    }
    if ( !p_.mesh.cyclpPatchName.empty() ) {
        pc.pcyclp=&this->addOrDestroyPatch ( p_.mesh.cyclpPatchName, new bmd::Patch() );
    }


    std::set<double> zs = {0., p_.geometry.L}; // (ordered) list of z-coordinates

    if (const auto* ii = boost::get<Parameters::geometry_type::inner_interface_extend_type>(&p_.geometry.inner_interface))
    {
      if (ii->z0 < 0)
        throw insight::Exception(
            boost::str(boost::format("Error in definition of inner interface protrusion: z0 has to be positive (%g)") % ii->z0)
            );
      if (ii->z1 > p_.geometry.L)
        throw insight::Exception(
            boost::str(boost::format("Error in definition of inner interface protrusion: z1 has to be smaller than L (%g<%g!)") % ii->z1 % p_.geometry.L)
            );
      if (ii->z0 >= ii->z1)
        throw insight::Exception(
            boost::str(boost::format("Error in definition of inner interface protrusion: z1 has to be larger than z0 (%g>%g!)") % ii->z1 % ii->z0)
            );

      zs.insert(ii->z0);
      zs.insert(ii->z1);
    }
    if (const auto* ii = boost::get<Parameters::geometry_type::outer_interface_extend_type>(&p_.geometry.outer_interface))
    {
      if (ii->z0 < 0)
        throw insight::Exception(
            boost::str(boost::format("Error in definition of outer interface protrusion: z0 has to be positive (%g)") % ii->z0)
            );
      if (ii->z1 > p_.geometry.L)
        throw insight::Exception(
            boost::str(boost::format("Error in definition of outer interface protrusion: z1 has to be smaller than L (%g<%g!)") % ii->z1 % p_.geometry.L)
            );
      if (ii->z0 >= ii->z1)
        throw insight::Exception(
            boost::str(boost::format("Error in definition of outer interface protrusion: z1 has to be larger than z0 (%g>%g!)") % ii->z1 % ii->z0)
            );

      zs.insert(ii->z0);
      zs.insert(ii->z1);
    }

    for (decltype(zs)::const_iterator i=++zs.begin(); i!=zs.end(); i++)
    {
      auto i0=i; i0--;
      auto i1=i; i1++;

      double dx = *i - *i0;
      int nx = std::max( 1, int(dx/deltax) );

      insertBlocks(
            spine, t0, t1,
            p_.geometry.wedge_angle*SI::deg,
            to_Vec(p_.geometry.ex), to_Pnt(p_.geometry.p0),
            *i0, *i,
            pc,
            p_.geometry.inner_interface,
            p_.geometry.outer_interface,
            i1!=zs.end(),
            i0==zs.begin(),
            nuBy2, nx, nr, deltax
       );
    }

//    writeVTK("debug_bmd.vtk");
}


double blockMeshDict_CylWedgeOrtho::rCore() const
{
//    return p_.geometry.D*p_.mesh.core_fraction;
  return 0;
}

}

}
