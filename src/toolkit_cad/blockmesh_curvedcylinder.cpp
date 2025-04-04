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

#include "blockmesh_curvedcylinder.h"
#include "base/boost_include.h"

#include "base/units.h"
#include "base/spatialtransformation.h"

#include "occinclude.h"

using namespace boost;
using namespace boost::assign;

namespace insight
{

namespace bmd
{



defineType ( blockMeshDict_CurvedCylinder );
addToOpenFOAMCaseElementFactoryTable(blockMeshDict_CurvedCylinder );


blockMeshDict_CurvedCylinder::blockMeshDict_CurvedCylinder ( OpenFOAMCase& c, ParameterSetInput ip )
    : BlockMeshTemplate ( c, ip.forward<Parameters>() )
{}


CoordinateSystem blockMeshDict_CurvedCylinder::calc_end_CS() const
{
  gp_Pnt P0=to_Pnt(p().geometry.p0), P1=to_Pnt(p().geometry.p1);
  Handle_Geom_TrimmedCurve spine = GC_MakeArcOfCircle
      (
        P0,
        to_Vec(p().geometry.ex/arma::norm(p().geometry.ex,2)),
        P1
       ).Value();

  GeomAdaptor_Curve ca(spine, spine->FirstParameter(), spine->LastParameter());
  gp_Circ c=ca.Circle();

  gp_XYZ r0=P0.XYZ()-c.Axis().Location().XYZ();
  gp_XYZ r1=P1.XYZ()-c.Axis().Location().XYZ();

  arma::mat R = rotMatrix( std::acos(r0.Dot(r1)/r0.Modulus()/r1.Modulus()), vec3(c.Axis().Direction()) );

  CoordinateSystem result(
              p().geometry.p1,
              R*p().geometry.ex );

  result.ez=R*p().geometry.er;
  result.ey=BlockMeshTemplate::correct_trihedron(result.ex, result.ez);

  return result;
}


void blockMeshDict_CurvedCylinder::create_bmd()
{
    this->setDefaultPatch(p().mesh.defaultPatchName);

    arma::mat p0=p().geometry.p0;
    arma::mat ex0=p().geometry.ex;
    arma::mat er0=p().geometry.er;
    arma::mat ey0=BlockMeshTemplate::correct_trihedron(ex0, er0);

    CoordinateSystem ec = calc_end_CS();
//    arma::mat p1=p().geometry.p1;
//    arma::mat ex1=R*p().geometry.ex;
//    arma::mat er1=R*p().geometry.er;
//    arma::mat ey1=BlockMeshTemplate::correct_trihedron(ex1, er1);

    double al = M_PI/2.;

    double Lc=rCore();

//     std::cout<<pts[0]<<pts[1]<<std::endl;
    Patch* base=nullptr;
    Patch* top=nullptr;
    Patch* outer=nullptr;

    if ( p().mesh.basePatchName!="" ) {
        base=&this->addOrDestroyPatch ( p().mesh.basePatchName, new bmd::Patch() );
    }
    if ( p().mesh.topPatchName!="" ) {
        top=&this->addOrDestroyPatch ( p().mesh.topPatchName, new bmd::Patch() );
    }
    if ( p().mesh.circumPatchName!="" ) {
        outer=&this->addOrDestroyPatch ( p().mesh.circumPatchName, new bmd::Patch() );
    }

    arma::mat r00=rotMatrix ( 0.5*al, ex0 );
    arma::mat r10=rotMatrix ( 1.5*al, ex0 );
    arma::mat r20=rotMatrix ( 2.5*al, ex0 );
    arma::mat r30=rotMatrix ( 3.5*al, ex0 );

    arma::mat r01=rotMatrix ( 0.5*al, ec.ex );
    arma::mat r11=rotMatrix ( 1.5*al, ec.ex );
    arma::mat r21=rotMatrix ( 2.5*al, ec.ex );
    arma::mat r31=rotMatrix ( 3.5*al, ec.ex );

    // core block
    {
      arma::mat yc0=Lc*er0;
      arma::mat yc1=Lc*ec.ez;

      Block& bl = this->addBlock
                  (
                      new Block ( P_8 (
                                    p0+r10*yc0, p0+r20*yc0, p0+r30*yc0, p0+r00*yc0,
                                    ec.origin+r11*yc1, ec.origin+r21*yc1, ec.origin+r31*yc1, ec.origin+r01*yc1
                                  ),
                                  p().mesh.nu, p().mesh.nu, p().mesh.nx
                                )
                  );

        if ( base ) {
            base->addFace ( bl.face ( "0321" ) );
        }
        if ( top ) {
            top->addFace ( bl.face ( "4567" ) );
        }
    }

    // radial blocks
    for ( int i=0; i<4; i++ ) {
        arma::mat r00=rotMatrix ( double ( i+0.5 ) *al, ex0 );
        arma::mat r10=rotMatrix ( double ( i+1.5 ) *al, ex0 );
        arma::mat r01=rotMatrix ( double ( i+0.5 ) *al, ec.ex );
        arma::mat r11=rotMatrix ( double ( i+1.5 ) *al, ec.ex );

        arma::mat yc0=Lc*er0, yo0=0.5*p().geometry.D*er0;
        arma::mat yc1=Lc*ec.ez, yo1=0.5*p().geometry.D*ec.ez;

        {
            Block& bl = this->addBlock
            (
                new Block ( P_8 (
                                p0+r10*yc0, p0+r00*yc0, p0+r00*yo0, p0+r10*yo0,
                                ec.origin+r11*yc1, ec.origin+r01*yc1, ec.origin+r01*yo1, ec.origin+r11*yo1
                            ),
                            p().mesh.nu, p().mesh.nr, p().mesh.nx,
                            list_of<double> ( 1 ) ( 1./p().mesh.gradr ) ( 1 )
                          )
            );
            if ( base ) {
                base->addFace ( bl.face ( "0321" ) );
            }
            if ( top ) {
                top->addFace ( bl.face ( "4567" ) );
            }
            if ( outer ) {
                outer->addFace ( bl.face ( "2376" ) );
            }
        }

        {
          arma::mat ps=p0+r10*yc0;
          arma::mat pe=ec.origin+r11*yc1;
          Handle_Geom_TrimmedCurve crv = GC_MakeArcOfCircle( to_Pnt(ps), to_Vec(ex0), to_Pnt(pe) ).Value();
          this->addEdge ( new ArcEdge (
                            ps,
                            pe,
                            vec3(crv->Value(0.5*(crv->FirstParameter()+crv->LastParameter())))
                            ) );
        }

        {
          arma::mat ps=p0+r10*yo0;
          arma::mat pe=ec.origin+r11*yo1;
          Handle_Geom_TrimmedCurve crv = GC_MakeArcOfCircle( to_Pnt(ps), to_Vec(ex0), to_Pnt(pe) ).Value();
          this->addEdge ( new ArcEdge (
                            ps,
                            pe,
                            vec3(crv->Value(0.5*(crv->FirstParameter()+crv->LastParameter())))
                            ) );
        }



        arma::mat rmid0=rotMatrix ( double ( i+1 ) *al, ex0 );
        arma::mat rmid1=rotMatrix ( double ( i+1 ) *al, ec.ex );

        this->addEdge ( new ArcEdge ( p0+r10*yo0, p0+r00*yo0, p0+rmid0*yo0 ) );
        this->addEdge ( new ArcEdge ( ec.origin+r11*yo1, ec.origin+r01*yo1, ec.origin+rmid1*yo1 ) );

        //inner core
//     bmd->addEdge(new ArcEdge(r1*pts[0], r0*pts[0], rmid*pts[]));
//     bmd->addEdge(new ArcEdge((r1*pts[0])+vL, (r0*pts[0])+vL, (rmid*pts[])+vL));

    }

}


double blockMeshDict_CurvedCylinder::rCore() const
{
    return p().geometry.D*p().mesh.core_fraction;
}



}
}
