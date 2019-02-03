
#include "blockmesh_curvedcylinder.h"
#include "base/boost_include.h"

#include "base/units.h"

#include "occinclude.h"

using namespace boost;
using namespace boost::assign;

namespace insight
{

namespace bmd
{



defineType ( blockMeshDict_CurvedCylinder );
addToOpenFOAMCaseElementFactoryTable(blockMeshDict_CurvedCylinder );


blockMeshDict_CurvedCylinder::blockMeshDict_CurvedCylinder ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c ), p_ ( ps )
{}


CoordinateSystem blockMeshDict_CurvedCylinder::calc_end_CS() const
{
  gp_Pnt P0=to_Pnt(p_.geometry.p0), P1=to_Pnt(p_.geometry.p1);
  Handle_Geom_TrimmedCurve spine = GC_MakeArcOfCircle
      (
        P0,
        to_Vec(p_.geometry.ex/arma::norm(p_.geometry.ex,2)),
        P1
       ).Value();

  GeomAdaptor_Curve ca(spine, spine->FirstParameter(), spine->LastParameter());
  gp_Circ c=ca.Circle();

  gp_XYZ r0=P0.XYZ()-c.Axis().Location().XYZ();
  gp_XYZ r1=P1.XYZ()-c.Axis().Location().XYZ();

  arma::mat R = rotMatrix( std::acos(r0.Dot(r1)/r0.Modulus()/r1.Modulus()), vec3(c.Axis().Direction()) );

  CoordinateSystem result;
  result.origin=p_.geometry.p1;
  result.ex=R*p_.geometry.ex;
  result.ez=R*p_.geometry.er;
  result.ey=BlockMeshTemplate::correct_trihedron(result.ex, result.ez);
  return result;
}


void blockMeshDict_CurvedCylinder::create_bmd()
{
    this->setDefaultPatch(p_.mesh.defaultPatchName);

    arma::mat p0=p_.geometry.p0;
    arma::mat ex0=p_.geometry.ex;
    arma::mat er0=p_.geometry.er;
    arma::mat ey0=BlockMeshTemplate::correct_trihedron(ex0, er0);

    CoordinateSystem ec = calc_end_CS();
//    arma::mat p1=p_.geometry.p1;
//    arma::mat ex1=R*p_.geometry.ex;
//    arma::mat er1=R*p_.geometry.er;
//    arma::mat ey1=BlockMeshTemplate::correct_trihedron(ex1, er1);

    double al = M_PI/2.;

    double Lc=rCore();

//     std::cout<<pts[0]<<pts[1]<<std::endl;
    Patch* base=nullptr;
    Patch* top=nullptr;
    Patch* outer=nullptr;

    if ( p_.mesh.basePatchName!="" ) {
        base=&this->addOrDestroyPatch ( p_.mesh.basePatchName, new bmd::Patch() );
    }
    if ( p_.mesh.topPatchName!="" ) {
        top=&this->addOrDestroyPatch ( p_.mesh.topPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.circumPatchName!="" ) {
        outer=&this->addOrDestroyPatch ( p_.mesh.circumPatchName, new bmd::Patch() );
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
                                  p_.mesh.nu, p_.mesh.nu, p_.mesh.nx
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

        arma::mat yc0=Lc*er0, yo0=0.5*p_.geometry.D*er0;
        arma::mat yc1=Lc*ec.ez, yo1=0.5*p_.geometry.D*ec.ez;

        {
            Block& bl = this->addBlock
            (
                new Block ( P_8 (
                                p0+r10*yc0, p0+r00*yc0, p0+r00*yo0, p0+r10*yo0,
                                ec.origin+r11*yc1, ec.origin+r01*yc1, ec.origin+r01*yo1, ec.origin+r11*yo1
                            ),
                            p_.mesh.nu, p_.mesh.nr, p_.mesh.nx,
                            list_of<double> ( 1 ) ( 1./p_.mesh.gradr ) ( 1 )
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
    return p_.geometry.D*p_.mesh.core_fraction;
}



}
}
