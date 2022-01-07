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

#include "base/units.h"

using namespace boost;
using namespace boost::assign;

namespace insight
{

namespace bmd
{




BlockMeshTemplate::BlockMeshTemplate ( OpenFOAMCase& c, const ParameterSet& ps )
    : blockMesh ( c, ps )
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
    ez /= arma::norm(ez, 2);
    
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
    ey /= mey;
    return ey;
}




defineType ( blockMeshDict_Cylinder );
addToOpenFOAMCaseElementFactoryTable(blockMeshDict_Cylinder );


blockMeshDict_Cylinder::blockMeshDict_Cylinder ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c, ps ), p_ ( ps )
{}


void blockMeshDict_Cylinder::create_bmd()
{
    this->setDefaultPatch(p_.mesh.defaultPatchName);

    bool hollow = p_.geometry.d > 1e-10;

    int nu, nx, nr;
    double L_r, L_u;

    auto setLrLu = [&](double x)
    {
        L_r = 0.5*(p_.geometry.D-p_.geometry.d),
        L_u = 2.*M_PI * ( (1.-x)*p_.geometry.d + x*p_.geometry.D ) /4.;
    };

    setLrLu(0.5);

    if (const auto* ic = boost::get<Parameters::mesh_type::resolution_individual_type>(&p_.mesh.resolution))
    {
      nu=ic->nu;
      nx=ic->nx;
      nr=ic->nr;
    }
    else if (const auto* ic = boost::get<Parameters::mesh_type::resolution_cubical_size_type>(&p_.mesh.resolution))
    {
      setLrLu(ic->xcubical);
      nx=std::max(1, int(std::ceil(p_.geometry.L/ic->delta)));
      nr=std::max(1, int(std::ceil(L_r/ic->delta)));
      nu=std::max(1, int(std::ceil(L_u/ic->delta)));
    }
    else if (const auto* ic = boost::get<Parameters::mesh_type::resolution_cubical_type>(&p_.mesh.resolution))
    {
      setLrLu(ic->xcubical);

      auto Ls={p_.geometry.L, L_r, L_u};
      double delta = *std::max_element(Ls.begin(), Ls.end()) / double(ic->n_max);

      nx=std::max(1, int(std::ceil(p_.geometry.L/delta)));
      nr=std::max(1, int(std::ceil(L_r/delta)));
      nu=std::max(1, int(std::ceil(L_u/delta)));
    }
    else
    {
      throw insight::Exception("Internal error: unhandled selection!");
    }

    arma::mat p0=p_.geometry.p0;
    arma::mat ex=p_.geometry.ex;
    arma::mat er=p_.geometry.er;
    arma::mat ey=BlockMeshTemplate::correct_trihedron(ex, er);

    double al = M_PI/2.;

    double Lc=rCore();
    if (hollow) Lc=p_.geometry.d*0.5;

    std::map<int, Point> pts = {
          { 1, 	0.5*p_.geometry.D*ey },
          { 0, 	/*::cos ( al/2. ) **/Lc*ey }
    };
    arma::mat vL=p_.geometry.L*ex;

//     std::cout<<pts[0]<<pts[1]<<std::endl;
    Patch* base=nullptr;
    Patch* top=nullptr;
    Patch* outer=nullptr;
    Patch *inner=nullptr;

    if ( p_.mesh.basePatchName!="" ) {
        base=&this->addOrDestroyPatch ( p_.mesh.basePatchName, new bmd::Patch() );
    }
    if ( p_.mesh.topPatchName!="" ) {
        top=&this->addOrDestroyPatch ( p_.mesh.topPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.circumPatchName!="" ) {
        outer=&this->addOrDestroyPatch ( p_.mesh.circumPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.innerPatchName!="" ) {
        inner=&this->addOrDestroyPatch ( p_.mesh.innerPatchName, new bmd::Patch() );
    }

    // core block
    if (!hollow)
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
                                    nu, nu, nx,
                                    {1, 1, p_.mesh.gradax},
                                    p_.mesh.cellZoneName
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
    for ( int i=0; i<4; i++ )
    {
        arma::mat r0=rotMatrix ( double ( i+0.5 ) *al, ex );
        arma::mat r1=rotMatrix ( double ( i+1.5 ) *al, ex );

        {
            Block& bl = this->addBlock
                        (
                            new Block ( P_8 (
                                            p0+r1*pts[0], p0+r0*pts[0], p0+r0*pts[1], p0+r1*pts[1],
                                            p0+( r1*pts[0] )+vL, p0+( r0*pts[0] )+vL, p0+( r0*pts[1] )+vL, p0+( r1*pts[1] )+vL
                                        ),
                                        nu, nr, nx,
                                        { 1,  1./p_.mesh.gradr, p_.mesh.gradax },
                                        p_.mesh.cellZoneName
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
            if (hollow && inner)
            {
              inner->addFace ( bl.face ( "0154" ) );
            }
        }


        arma::mat rmid=rotMatrix ( double ( i+1 ) *al, ex );
        this->addEdge ( new ArcEdge ( p0+r1*pts[1], p0+r0*pts[1], p0+rmid*pts[1] ) );
        this->addEdge ( new ArcEdge ( p0+( r1*pts[1] )+vL, p0+( r0*pts[1] )+vL, p0+( rmid*pts[1] )+vL ) );

        //inner core
        if (hollow)
        {
           this->addEdge(new ArcEdge(p0+r1*pts[0], p0+r0*pts[0], p0+rmid*pts[0]));
           this->addEdge(new ArcEdge(p0+(r1*pts[0])+vL, p0+(r0*pts[0])+vL, p0+(rmid*pts[0])+vL));
        }
        else
        {
          if (p_.mesh.smoothCore)
          {
            auto pstart = p0+r0*pts[0];
            auto pend = p0+r1*pts[0];

            auto pmid = p0+rmid*pts[0];
            auto pmido = p0+rmid*pts[1];
            arma::mat elo=pmid-pmido;
            elo/=arma::norm(elo,2);

            double linfrac=0.2;
            auto& e1 = this->addEdge(new SplineEdge(
                            {pstart, //(1.-linfrac)*pstart + linfrac*pend,
                             pmido + elo*(0.5*p_.geometry.D-Lc),
                             /*linfrac*pstart+(1.-linfrac)*pend, */pend}));
            this->addEdge(e1.transformed(arma::eye(3,3), vL));
          }
        }

    }

}


double blockMeshDict_Cylinder::rCore() const
{
    return p_.geometry.D*p_.mesh.core_fraction;
}







defineType ( blockMeshDict_Box );
addToOpenFOAMCaseElementFactoryTable (blockMeshDict_Box );


blockMeshDict_Box::blockMeshDict_Box ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c, ps ), p_ ( ps )
{
}


void blockMeshDict_Box::create_bmd()
{
    this->setDefaultPatch(p_.mesh.defaultPatchName);
    
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

    Patch *Xp=nullptr, *Xm=nullptr, *Yp=nullptr, *Ym=nullptr, *Zp=nullptr, *Zm=nullptr;

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

    int nx, ny, nz;
    if (const auto* cu = boost::get<Parameters::mesh_type::resolution_cubical_type>(&p_.mesh.resolution))
      {
        double dx=std::max(std::max(p_.geometry.L, p_.geometry.W), p_.geometry.H)/double(cu->n_max);
        nx=int(std::ceil(p_.geometry.L/dx));
        ny=int(std::ceil(p_.geometry.W/dx));
        nz=int(std::ceil(p_.geometry.H/dx));
      }
    else if (const auto* cus = boost::get<Parameters::mesh_type::resolution_cubical_size_type>(&p_.mesh.resolution))
      {
        nx=int(std::ceil(p_.geometry.L/cus->delta));
        ny=int(std::ceil(p_.geometry.W/cus->delta));
        nz=int(std::ceil(p_.geometry.H/cus->delta));
      }
    else if (const auto* ind = boost::get<Parameters::mesh_type::resolution_individual_type>(&p_.mesh.resolution))
      {
        nx=ind->nx;
        ny=ind->ny;
        nz=ind->nz;
      }
    else
      {
        throw insight::Exception("Internal error: unhandled selection.");
      }


    Block& bl = this->addBlock
                (
                    new Block ( P_8 (
                                    pts[0], pts[1], pts[2], pts[3],
                                    pts[4], pts[5], pts[6], pts[7]
                                ),
                                nx, ny, nz
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






defineType ( blockMeshDict_Sphere );
addToOpenFOAMCaseElementFactoryTable (blockMeshDict_Sphere );


blockMeshDict_Sphere::blockMeshDict_Sphere ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c, ps ), p_ ( ps )
{}


void blockMeshDict_Sphere::create_bmd()
{
//    this->setDefaultPatch(p_.mesh.outerPatchName);

    arma::mat ex=p_.geometry.ex; ex/=arma::norm(ex, 2);
    arma::mat ez=p_.geometry.ez; ez/=arma::norm(ez, 2);
    arma::mat ey=BlockMeshTemplate::correct_trihedron(ex, ez);

    std::cout<<ex<<ey<<ez<<p_.geometry.D<<std::endl;

    double ang = ::acos(arma::norm_dot(ex, ez))*180./M_PI;
    if (fabs(90.-ang)>1e-3)
    {
        arma::mat eznew=arma::cross(ey, ex);
        insight::Warning(str(format("blockMeshDict_Sphere: supplied vectors ex and ez do not make a right angle (angle is %gdeg). Rectifying ez from (%g, %g, %g) to (%g, %g, %g)!")
                 % ang % ez(0) % ez(1) % ez(2) % eznew(0) % eznew(1) % eznew(2) )
        );
        ez=eznew;
    }

    int nu=std::max(1, p_.mesh.n_u/4);
    double Lc=p_.geometry.core_fraction*p_.geometry.D;
    double Lr=0.5 *( p_.geometry.D - Lc*std::sqrt(2.) );
    double du=Lc/double(nu);

    GradingAnalyzer ga(p_.mesh.grad_r);
    int nr = ga.calc_n(du, Lr);


    arma::mat c = p_.geometry.center;
    // core
    this->addBlock
                (
                    new Block ( P_8 (
          c + 0.5*Lc*(-ex-ey-ez), c + 0.5*Lc*(-ex+ey-ez), c + 0.5*Lc*(-ex+ey+ez), c + 0.5*Lc*(-ex-ey+ez),
          c + 0.5*Lc*(+ex-ey-ez), c + 0.5*Lc*(+ex+ey-ez), c + 0.5*Lc*(+ex+ey+ez), c + 0.5*Lc*(+ex-ey+ez)
                    ),
                  nu, nu, nu
                  )
                );

    const double R=0.5*p_.geometry.D;
    auto op = [&](double theta, double phi) -> arma::mat {
        return c + R*(ez*std::cos(theta) + ex*std::sin(theta)*std::cos(phi) + ey*std::sin(theta)*std::sin(phi));
      };

    // outer blocks

    outer_ = &this->addOrDestroyPatch ( p_.mesh.outerPatchName, new bmd::Patch() );

    double theta1=p_.mesh.theta_trans*SI::deg, theta2=180*SI::deg-theta1;
    // -X
    {
      auto bpts = P_8 (
      op(theta2, 225*SI::deg), op(theta2, 135*SI::deg), op(theta1, 135*SI::deg), op(theta1, 225*SI::deg),
      c + 0.5*Lc*(-ex -ey -ez), c + 0.5*Lc*(-ex +ey -ez), c + 0.5*Lc*(-ex +ey +ez), c + 0.5*Lc*(-ex -ey +ez)
      );
      Block& bl = this->addBlock(
          new Block ( bpts,
           nu, nu, nr,
           { 1, 1, 1./p_.mesh.grad_r }
          )
      );
      this->addEdge ( new CircularEdge_Center ( bpts[0], bpts[1], c ) );
      this->addEdge ( new CircularEdge_Center ( bpts[1], bpts[2], c ) );
      this->addEdge ( new CircularEdge_Center ( bpts[2], bpts[3], c ) );
      this->addEdge ( new CircularEdge_Center ( bpts[0], bpts[3], c ) );

      outer_->addFace(bl.face("0321"));
    }


    // +X
    {
      auto bpts = P_8 (
      op(theta2, 315*SI::deg), op(theta2, 45*SI::deg), op(theta1, 45*SI::deg), op(theta1, 315*SI::deg),
      c + 0.5*Lc*(ex -ey -ez), c + 0.5*Lc*(ex +ey -ez), c + 0.5*Lc*(ex +ey +ez), c + 0.5*Lc*(ex -ey +ez)
      );
      Block& bl =this->addBlock
                (
                    new Block ( bpts,
                      nu, nu, nr,
                      { 1, 1, 1./p_.mesh.grad_r },
                      "", true
                  )
                );
      this->addEdge ( new CircularEdge_Center ( bpts[0], bpts[1], c ) );
      this->addEdge ( new CircularEdge_Center ( bpts[1], bpts[2], c ) );
      this->addEdge ( new CircularEdge_Center ( bpts[2], bpts[3], c ) );
      this->addEdge ( new CircularEdge_Center ( bpts[0], bpts[3], c ) );
      outer_->addFace(bl.face("0321"));
    }

    // -Y
    {
      auto bpts = P_8 (
      op(theta2, 225*SI::deg), op(theta2, 315*SI::deg), op(theta1, 315*SI::deg), op(theta1, 225*SI::deg),
      c + 0.5*Lc*(-ey -ex -ez), c + 0.5*Lc*(-ey +ex -ez), c + 0.5*Lc*(-ey +ex +ez), c + 0.5*Lc*(-ey -ex +ez)
      );
      Block& bl =this->addBlock
                (
                    new Block ( bpts,
                      nu, nu, nr,
                      { 1, 1, 1./p_.mesh.grad_r },
                      "", true
                  )
                );
      this->addEdge ( new CircularEdge_Center ( bpts[0], bpts[1], c ) );
      this->addEdge ( new CircularEdge_Center ( bpts[2], bpts[3], c ) );
      outer_->addFace(bl.face("0321"));
    }

    // +Y
    {
      auto bpts = P_8 (
       op(theta2, 135*SI::deg), op(theta2, 45*SI::deg), op(theta1, 45*SI::deg), op(theta1, 135*SI::deg),
       c + 0.5*Lc*(+ey -ex -ez), c + 0.5*Lc*(+ey +ex -ez), c + 0.5*Lc*(+ey +ex +ez), c + 0.5*Lc*(+ey -ex +ez)
      );
      Block& bl =this->addBlock
                (
                    new Block ( bpts,
                      nu, nu, nr,
                      { 1, 1, 1./p_.mesh.grad_r }/*,
                      "", true*/
                  )
                );
      this->addEdge ( new CircularEdge_Center ( bpts[0], bpts[1], c ) );
      this->addEdge ( new CircularEdge_Center ( bpts[2], bpts[3], c ) );
      outer_->addFace(bl.face("0321"));
    }


    // -Z
    {
      auto bpts = P_8 (
      op(theta2, 225*SI::deg), op(theta2, 315*SI::deg), op(theta2, 45*SI::deg), op(theta2, 135*SI::deg),
      c + 0.5*Lc*(-ez -ex -ey), c + 0.5*Lc*(-ez +ex -ey), c + 0.5*Lc*(-ez +ex +ey), c + 0.5*Lc*(-ez -ex +ey)
      );
      Block& bl =this->addBlock
                (
                    new Block ( bpts,
                      nu, nu, nr,
                      { 1, 1, 1./p_.mesh.grad_r }
                  )
                );
      outer_->addFace(bl.face("0321"));
    }

    // +Z
    {
      auto bpts = P_8 (
       op(theta1, 225*SI::deg), op(theta1, 315*SI::deg), op(theta1, 45*SI::deg), op(theta1, 135*SI::deg),
       c + 0.5*Lc*(+ez -ex -ey), c + 0.5*Lc*(+ez +ex -ey), c + 0.5*Lc*(+ez +ex +ey), c + 0.5*Lc*(+ez -ex +ey)
      );
      Block& bl =this->addBlock
                (
                    new Block ( bpts,
                      nu, nu, nr,
                      { 1, 1, 1./p_.mesh.grad_r },
                                "", true
                  )
                );
      outer_->addFace(bl.face("0321"));
    }
}

void blockMeshDict_Sphere::addIntoDictionaries ( OFdicts& dictionaries ) const
{
    BlockMeshTemplate::addIntoDictionaries(dictionaries);

    OFDictData::dict& bmd = getBlockMeshDict(dictionaries);

    OFDictData::dict geom;
    OFDictData::dict sphere;
    sphere["type"]="searchableSphere";
    sphere["centre"]=OFDictData::vector3(p_.geometry.center);
    sphere["radius"]=p_.geometry.D*0.5;
    geom["sphere"]=sphere;
    bmd["geometry"]=geom;

    OFDictData::list pfaces;
    PointMap pts(allPoints_);
    numberVertices(pts);
    for (const auto& f: outer_->faces())
      {
        pfaces.push_back("project");
        pfaces.push_back( bmdEntry(f, pts) );
        pfaces.push_back("sphere");
      }
    bmd["faces"]=pfaces;
}



}

}
