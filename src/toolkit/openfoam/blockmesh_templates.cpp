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
 *
 */

#include "base/boost_include.h"
#include "blockmesh_templates.h"

using namespace boost;
using namespace boost::assign;

namespace insight
{

namespace bmd
{




BlockMeshTemplate::BlockMeshTemplate ( OpenFOAMCase& c )
    : blockMesh ( c )
{
}




void BlockMeshTemplate::addIntoDictionaries ( OFdicts& dictionaries ) const
{
    const_cast<BlockMeshTemplate*> ( this ) -> create_bmd();
    blockMesh::addIntoDictionaries ( dictionaries );
}


arma::mat BlockMeshTemplate::correct_trihedron(arma::mat& ex, arma::mat &ez)
{
    ex /= arma::norm(ex, 2);    
    ez/= arma::norm(ez,2);
    
    arma::mat ey = -arma::cross(ex, ez);
    double mey=arma::norm(ey,2);
    if (mey<1e-6)
    {
        throw insight::Exception
        (
            str(format("BlockMeshTemplate: supplied vectors ex=(%g, %g, %g) and ez=(%g, %g, %g) have a vanishing cross product => cannot determine third direction!")
                 % ex(0) % ex(1) % ex(2) % ez(0) % ez(1) % ez(2) )
        );
    }
    ey/=mey;
    return ey;
}


defineType ( blockMeshDict_Cylinder );
addToOpenFOAMCaseElementFactoryTable(blockMeshDict_Cylinder );


blockMeshDict_Cylinder::blockMeshDict_Cylinder ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c ), p_ ( ps )
{}


void blockMeshDict_Cylinder::create_bmd()
{
    this->setDefaultPatch(p_.mesh.defaultPatchName);

    arma::mat p0=p_.geometry.p0;
    arma::mat ex=p_.geometry.ex;
    arma::mat er=p_.geometry.er;
    arma::mat ey=BlockMeshTemplate::correct_trihedron(ex, er);

    double al = M_PI/2.;

    double Lc=rCore();

    std::map<int, Point> pts;
    pts = boost::assign::map_list_of
          ( 1, 	0.5*p_.geometry.D*ey )
          ( 0, 	::cos ( al/2. ) *Lc*ey )
          .convert_to_container<std::map<int, Point> >()
          ;
    arma::mat vL=p_.geometry.L*ex;

//     std::cout<<pts[0]<<pts[1]<<std::endl;
    Patch* base=NULL;
    Patch* top=NULL;
    Patch* outer=NULL;

    if ( p_.mesh.basePatchName!="" ) {
        base=&this->addOrDestroyPatch ( p_.mesh.basePatchName, new bmd::Patch() );
    }
    if ( p_.mesh.topPatchName!="" ) {
        top=&this->addOrDestroyPatch ( p_.mesh.topPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.circumPatchName!="" ) {
        outer=&this->addOrDestroyPatch ( p_.mesh.circumPatchName, new bmd::Patch() );
    }

    // core block
    {
        arma::mat r0=rotMatrix ( 0.5*al, ex );
        arma::mat r1=rotMatrix ( 1.5*al, ex );
        arma::mat r2=rotMatrix ( 2.5*al, ex );
        arma::mat r3=rotMatrix ( 3.5*al, ex );

        Block& bl = this->addBlock
                    (
                        new Block ( P_8 (
                                        p0+r1*pts[0], p0+r2*pts[0], p0+r3*pts[0], p0+r0*pts[0],
                                        p0+( r1*pts[0] )+vL, p0+( r2*pts[0] )+vL, p0+( r3*pts[0] )+vL, p0+( r0*pts[0] )+vL
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
        arma::mat r0=rotMatrix ( double ( i+0.5 ) *al, ex );
        arma::mat r1=rotMatrix ( double ( i+1.5 ) *al, ex );

        {
            Block& bl = this->addBlock
                        (
                            new Block ( P_8 (
                                            p0+r1*pts[0], p0+r0*pts[0], p0+r0*pts[1], p0+r1*pts[1],
                                            p0+( r1*pts[0] )+vL, p0+( r0*pts[0] )+vL, p0+( r0*pts[1] )+vL, p0+( r1*pts[1] )+vL
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


        arma::mat rmid=rotMatrix ( double ( i+1 ) *al, ex );
        this->addEdge ( new ArcEdge ( p0+r1*pts[1], p0+r0*pts[1], p0+rmid*pts[1] ) );
        this->addEdge ( new ArcEdge ( p0+( r1*pts[1] )+vL, p0+( r0*pts[1] )+vL, p0+( rmid*pts[1] )+vL ) );

        //inner core
//     bmd->addEdge(new ArcEdge(r1*pts[0], r0*pts[0], rmid*pts[]));
//     bmd->addEdge(new ArcEdge((r1*pts[0])+vL, (r0*pts[0])+vL, (rmid*pts[])+vL));

    }

}


double blockMeshDict_Cylinder::rCore() const
{
    return p_.geometry.D*p_.mesh.core_fraction;
}





defineType ( blockMeshDict_Box );
addToOpenFOAMCaseElementFactoryTable (blockMeshDict_Box );


blockMeshDict_Box::blockMeshDict_Box ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c ), p_ ( ps )
{}


void blockMeshDict_Box::create_bmd()
{
    this->setDefaultPatch(p_.mesh.defaultPatchName);
    
    double al = M_PI/2.;

    arma::mat ex=p_.geometry.ex;
    arma::mat ez=p_.geometry.ez;
    arma::mat ey=BlockMeshTemplate::correct_trihedron(ex, ez);
    
    double ang = ::acos(arma::norm_dot(ex, ez))*180./M_PI;
    if (fabs(90.-ang)>1e-3)
    {
        arma::mat eznew=arma::cross(ey, ex);
        insight::Warning(str(format("blockMeshDict_Box: supplied vectors ex and ez do not make a right angle (angle is %gdeg). Rectifying ez from (%g, %g, %g) to (%g, %g, %g)!")
                 % ang % ez(0) % ez(1) % ez(2) % eznew(0) % eznew(1) % eznew(2) )
        );         
        ez=eznew;
    }

    std::map<int, Point> pts;
    pts = boost::assign::map_list_of
          ( 0, 	p_.geometry.p0 )
          ( 1, 	p_.geometry.p0 +p_.geometry.L*ex )
          ( 2, 	p_.geometry.p0 +p_.geometry.L*ex +p_.geometry.W*ey )
          ( 3, 	p_.geometry.p0 +p_.geometry.W*ey )
          ( 4, 	p_.geometry.p0 +p_.geometry.H*ez )
          ( 5, 	p_.geometry.p0 +p_.geometry.H*ez +p_.geometry.L*ex )
          ( 6, 	p_.geometry.p0 +p_.geometry.H*ez +p_.geometry.L*ex +p_.geometry.W*ey )
          ( 7, 	p_.geometry.p0 +p_.geometry.H*ez +p_.geometry.W*ey )
          .convert_to_container<std::map<int, Point> >()
          ;

    Patch *Xp=NULL, *Xm=NULL, *Yp=NULL, *Ym=NULL, *Zp=NULL, *Zm=NULL;

    if ( p_.mesh.XpPatchName!="" ) {
        Xp=&this->addOrDestroyPatch ( p_.mesh.XpPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.XmPatchName!="" ) {
        Xm=&this->addOrDestroyPatch ( p_.mesh.XmPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.YpPatchName!="" ) {
        Yp=&this->addOrDestroyPatch ( p_.mesh.YpPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.YmPatchName!="" ) {
        Ym=&this->addOrDestroyPatch ( p_.mesh.YmPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.ZpPatchName!="" ) {
        Zp=&this->addOrDestroyPatch ( p_.mesh.ZpPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.ZmPatchName!="" ) {
        Zm=&this->addOrDestroyPatch ( p_.mesh.ZmPatchName, new bmd::Patch() );
    }

    Block& bl = this->addBlock
                (
                    new Block ( P_8 (
                                    pts[0], pts[1], pts[2], pts[3],
                                    pts[4], pts[5], pts[6], pts[7]
                                ),
                                p_.mesh.nx, p_.mesh.ny, p_.mesh.nz
                                )
                );
    if ( Xp ) {
        Xp->addFace ( bl.face ( "1265" ) );
    }
    if ( Xm ) {
        Xm->addFace ( bl.face ( "0473" ) );
    }
    if ( Yp ) {
        Yp->addFace ( bl.face ( "0154" ) );
    }
    if ( Ym ) {
        Ym->addFace ( bl.face ( "2376" ) );
    }
    if ( Zp ) {
        Zp->addFace ( bl.face ( "4567" ) );
    }
    if ( Zm ) {
        Zm->addFace ( bl.face ( "0321" ) );
    }

}

}

}
