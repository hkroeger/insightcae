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

#include "blockmesh_cylwedge.h"

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



defineType ( blockMeshDict_CylWedge );
addToOpenFOAMCaseElementFactoryTable(blockMeshDict_CylWedge );


blockMeshDict_CylWedge::blockMeshDict_CylWedge ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c, ps ), p_ ( ps )
{
  p0_=p_.geometry.p0;
  ex_=p_.geometry.ex;
  er_=p_.geometry.er;
  ey_=BlockMeshTemplate::correct_trihedron(ex_, er_);
}


std::pair<double,double> blockMeshDict_CylWedge::limit_angles()
{
  std::vector<double> phi;
  {
    auto calc_angle = [&](const arma::mat& p) -> double
    {
      double y=arma::dot(p-p0_, -ey_);
      double x=arma::dot(p-p0_, er_);
      return std::atan2(y, x);
    };

    const int np=20;
    for (int i=0; i<np; i++)
    {
      double d=p_.geometry.d + double(i)/double(np-1)*(p_.geometry.D-p_.geometry.d);
      phi.push_back(calc_angle(point_on_spine(0.5*d)));
    }
  }
  return std::pair<double,double>(
        *std::min_element(phi.begin(), phi.end()) -0.5*p_.geometry.wedge_angle*SI::deg,
        *std::max_element(phi.begin(), phi.end()) +0.5*p_.geometry.wedge_angle*SI::deg
        );
}


Handle_Geom_Curve blockMeshDict_CylWedge::spine()
{
  if (spine_.IsNull())
  {
    // Read or create spine curve
    if (p_.geometry.wedge_spine_curve->isValid())
    {
      insight::cad::FeaturePtr wsc =
          insight::cad::Import::create( p_.geometry.wedge_spine_curve->filePath() );

      wsc->checkForBuildDuringAccess(); // force rebuild
      auto el = wsc->allEdgesSet();

      if (el.size()!=1)
        throw insight::Exception(
            boost::str(boost::format("CAD file %s should contain only one single edge! (It actually contains %d edges)")
                       % p_.geometry.wedge_spine_curve->originalFilePath().string() % el.size() )
            );

      TopoDS_Edge e= TopoDS::Edge(wsc->edge(*el.begin()));

      double t0, t1;
      spine_ = BRep_Tool::Curve(e, t0, t1);

      // align er with beginning (inner) of spine
      arma::mat per=vec3(spine_->Value(t0));
      arma::mat p2=vec3(spine_->Value(t1));
      auto rdist = [&](const arma::mat& p, const arma::mat& ex) -> double
      {
        arma::mat r=p-p0_;
        return arma::norm(r - arma::dot(r,ex)*ex, 2);
      };

      if ( rdist(p2,p0_) < rdist(per,p0_) ) per=p2;

      er_ = per-p0_;
      er_ -= arma::dot(er_,ex_)*ex_;
      ey_ = BlockMeshTemplate::correct_trihedron(ex_, er_);
    }
    else
    {
      spine_ = GC_MakeSegment(
                to_Pnt(p0_ + er_*p_.geometry.d*0.49),
                to_Pnt(p0_ + er_*p_.geometry.D*0.51)
                ).Value();
    }
  }

  return spine_;
}


arma::mat blockMeshDict_CylWedge::point_on_spine(double r)
{
  cout<<"r="<<r<<endl;
  Handle_Geom_Curve sp=spine();

  Handle_Geom_Surface cyl(new  Geom_CylindricalSurface(gp_Ax3(to_Pnt(p0_), to_Vec(ex_), to_Vec(-er_)), r));
  GeomAPI_IntCS isec(sp, cyl);
  cout<<isec.NbPoints()<<endl;
  if (isec.NbPoints()<1)
    throw insight::Exception(boost::str(boost::format(
                                          "No intersection point with spine found for r=%g!"
                                          ) % r));
  std::map<double,arma::mat> res;
  for (int i=0;i<isec.NbPoints();i++)
  {
    arma::mat p=vec3(isec.Point(i+1));
    arma::mat r=p-p0_;
    double odist = arma::norm(r-std::max<double>(0.,dot(r, er_))*er_, 2);
//        cout<<odist<<" => "<<p<<endl;
    res[odist]=p;
  }
  auto p_sel = res.begin()->second;

  p_sel -= dot(p_sel-p0_, ex_)*ex_;
  return p_sel;
};

void blockMeshDict_CylWedge::create_bmd()
{
    this->setDefaultPatch(p_.mesh.defaultPatchName);


    // helper function




    double rc=rCore();

//     std::cout<<pts[0]<<pts[1]<<std::endl;
    Patch* base=nullptr;
    Patch* top=nullptr;
    Patch* outer=nullptr;
    Patch* inner=nullptr;
    Patch* pcyclm=nullptr;
    Patch* pcyclp=nullptr;

    if ( p_.mesh.basePatchName!="" ) {
        base=&this->addOrDestroyPatch ( p_.mesh.basePatchName, new bmd::Patch() );
    }
    if ( p_.mesh.topPatchName!="" ) {
        top=&this->addOrDestroyPatch ( p_.mesh.topPatchName, new bmd::Patch() );
    }
    if ( p_.mesh.outerPatchName!="" ) {
        outer=&this->addOrDestroyPatch ( p_.mesh.outerPatchName, new bmd::Patch() );
    }
    if ( !p_.mesh.innerPatchName.empty() ) {
        inner=&this->addOrDestroyPatch ( p_.mesh.innerPatchName, new bmd::Patch() );
    }
    if ( !p_.mesh.cyclmPatchName.empty() ) {
        pcyclm=&this->addOrDestroyPatch ( p_.mesh.cyclmPatchName, new bmd::Patch() );
    }
    if ( !p_.mesh.cyclpPatchName.empty() ) {
        pcyclp=&this->addOrDestroyPatch ( p_.mesh.cyclpPatchName, new bmd::Patch() );
    }






//    const int np=10;
//    TopoDS_Edge c0;
//    {
//      std::vector<insight::cad::VectorPtr> pts;
//      for (int i=0; i<np; i++)
//      {
//        double r=0.5*(p_.geometry.d + (p_.geometry.D-p_.geometry.d)*double(i)/double(np-1));
//        pts.push_back(insight::cad::matconst(point_on_spine(r)));
//      }
//      cad::FeaturePtr spc=insight::cad::SplineCurve::create(pts);
//      c0=TopoDS::Edge(spc->shape());
//    }

//    gp_Ax1 ax(to_Pnt(p0_), to_Vec(ex_));
//    gp_Trsf trm; trm.SetRotation(ax, -0.5*p_.geometry.wedge_angle*SI::deg);
//    gp_Trsf trp; trp.SetRotation(ax, 0.5*p_.geometry.wedge_angle*SI::deg);
//    gp_Trsf tru; tru.SetTranslation(to_Vec(vL));

//    TopoDS_Edge c0m=TopoDS::Edge(BRepBuilderAPI_Transform(c0, trm).Shape());
//    TopoDS_Edge c0p=TopoDS::Edge(BRepBuilderAPI_Transform(c0, trp).Shape());
//    TopoDS_Face cyclm = BRepFill::Face ( c0m,  TopoDS::Edge(BRepBuilderAPI_Transform(c0m, tru).Shape()));
//    TopoDS_Face cyclp = BRepFill::Face ( c0p,  TopoDS::Edge(BRepBuilderAPI_Transform(c0p, tru).Shape()));

//    {
//      StlAPI_Writer stlwriter;
//      stlwriter.ASCIIMode() = true;
//      BRepMesh_IncrementalMesh Incm(cyclm, 1e-2);
//      stlwriter.Write(cyclm, "cyclm.stl");
//    }
//    {
//      StlAPI_Writer stlwriter;
//      stlwriter.ASCIIMode() = true;
//      BRepMesh_IncrementalMesh Incp(cyclp, 1e-2);
//      stlwriter.Write(cyclp, "cyclp.stl");
//    }


    auto phi_lim = limit_angles();
    double phim=phi_lim.first;
    double phip=phi_lim.second;
    cout<<"phim="<<phim<<", phip="<<phip<<endl;

    arma::mat rp=rotMatrix ( phip /*0.5*p_.geometry.wedge_angle*SI::deg*/, ex_ );
    arma::mat rm=rotMatrix ( phim /*-0.5*p_.geometry.wedge_angle*SI::deg*/, ex_ );

    int nu1, nu2, nx_tot, nr;
    double
        L_r = 0.5*(p_.geometry.D-p_.geometry.d),
        L_u1 = fabs(phim)*0.25*(p_.geometry.d+p_.geometry.D),
        L_u2 = fabs(phip)*0.25*(p_.geometry.d+p_.geometry.D);
    if (const auto* ic = boost::get<Parameters::mesh_type::resolution_individual_type>(&p_.mesh.resolution))
    {
      nu1=int(std::ceil( fabs(phim)/fabs(phip-phim)*double(ic->nu) ));
      nu2=std::max(1, ic->nu - nu1);
      nx_tot=ic->nx;
      nr=ic->nr;
    }
    else if (const auto* ic = boost::get<Parameters::mesh_type::resolution_cubical_size_type>(&p_.mesh.resolution))
    {
      nx_tot=std::max(1, int(std::ceil(p_.geometry.L/ic->delta)));
      nr=std::max(1, int(std::ceil(L_r/ic->delta)));
      nu1=std::max(1, int(std::ceil(L_u1/ic->delta)));
      nu2=std::max(1, int(std::ceil(L_u2/ic->delta)));
    }
    else if (const auto* ic = boost::get<Parameters::mesh_type::resolution_cubical_type>(&p_.mesh.resolution))
    {
      auto Ls={p_.geometry.L, L_r, L_u1, L_u2};
      double delta = *std::max_element(Ls.begin(), Ls.end()) / double(ic->n_max);

      nx_tot=std::max(1, int(std::ceil(p_.geometry.L/delta)));
      nr=std::max(1, int(std::ceil(L_r/delta)));
      nu1=std::max(1, int(std::ceil(L_u1/delta)));
      nu2=std::max(1, int(std::ceil(L_u2/delta)));
    }

    struct zSection
    {
      double z0, z1;
      Patch* outerPatch;
      bool lowerEnd, upperEnd;

      bool operator<(const zSection& o) const
      {
        return z0<o.z0;
      }

      zSection()
        : z0(0.), z1(0.),
          outerPatch(nullptr),
          lowerEnd(false), upperEnd(false)
      {}

    } defaultSection;

    defaultSection.z0=0.;
    defaultSection.z1=p_.geometry.L;
    defaultSection.outerPatch=outer;
    defaultSection.lowerEnd=false;
    defaultSection.upperEnd=false;

    auto defaultSectionBetween = [&](double z0, double z1)
    {
      zSection result = defaultSection;
      result.z0=z0;
      result.z1=z1;
      return result;
    };

    std::set<zSection> sections ; //({defaultSection});

    // no overlap with other section is allowed! => check needed
    for (auto& sec: p_.mesh.outerPatchSections)
    {
      if (sec.x0 > sec.x1)
      {
        std::swap(sec.x0, sec.x1);
        insight::Warning("Corrected section specification: x0 shall be smaller than x1!");
      }
      sec.x0=std::max(0., sec.x0);
      sec.x1=std::min(p_.geometry.L, sec.x1);
    }
//    for (auto i1 = p_.mesh.outerPatchSections.begin(); i1!=p_.mesh.outerPatchSections.end(); i1++)
//    {
//      for (auto i2 = p_.mesh.outerPatchSections.begin(); i2!=p_.mesh.outerPatchSections.end(); i2++)
//      {
//        if (i1!=i2)
//        {
//          if ( (i1->x0 < i2->x0) && (i1->x1
//        }
//      }
//    }

    // insert all sections
    for (const auto& sec: p_.mesh.outerPatchSections)
    {
      zSection zs;
      zs.z0=sec.x0;
      zs.z1=sec.x1;
      if (!sec.name.empty())
      {
        if (sec.name == p_.mesh.outerPatchName)
        {
          zs.outerPatch = outer;
        }
        else
        {
          zs.outerPatch = &this->addOrDestroyPatch ( sec.name, new bmd::Patch() );
        }
      }
      sections.insert(zs);
    }

    if (sections.size()==0)
    {
      sections.insert(defaultSection);
    }
    else
    {
      // check for needed sections between
      std::set<zSection> sectionsToAdd;
      for (auto i=sections.begin(); i!=sections.end(); i++)
      {
        auto j=std::next(i,1);

        if (i==sections.begin())
        {
          if (i->z0 > 0.)
            sectionsToAdd.insert( defaultSectionBetween(0., i->z1) );
        }

        if (j==sections.end())
        {
          if (i->z1 < p_.geometry.L)
            sectionsToAdd.insert( defaultSectionBetween(i->z1, p_.geometry.L) );
        }
        else
        {
          if (fabs(i->z1 - j->z0)>1e-10)
            sectionsToAdd.insert( defaultSectionBetween(i->z1, j->z0) );
        }
      }
      std::copy( sectionsToAdd.begin(), sectionsToAdd.end(),
                 std::inserter(sections, sections.begin()) );
    }

    for (auto & s: sections)
    {
      std::cerr<<"section: "<<s.z0<<" => "<<s.z1<<std::endl;
    }


    {
      auto j=sections.begin();
      zSection first = *j;
      first.lowerEnd = true;
      sections.erase(j);
      sections.insert(first);

      auto i = sections.begin();
      while ( std::next(i,1) != sections.end() ) i++;
      zSection last = *i;
      last.upperEnd = true;
      sections.erase(i);
      sections.insert(last);
    }


    for (auto i=sections.begin(); i!=sections.end(); i++)
    {
      arma::mat vL0 = i->z0 * ex_;
      arma::mat vL1 = i->z1 * ex_;

      int nx = std::max(1, int(double(nx_tot)*( (i->z1 - i->z0)/p_.geometry.L )) );

      arma::mat p_rc, p_i, p_o;
      if (p_.geometry.d < 1e-10)
      {
        // build mesh with core block

        p_i=p0_+0.9*rc*er_; //point_on_spine(0.9*rc);
        p_rc=p0_+rc*er_; //point_on_spine(rc);
        // core block
        {

            Block& bl = this->addBlock
                        (
                            new Block ( P_8 (
                                            p0_+vL0, rm*p_i+vL0, p_rc+vL0, rp*p_i+vL0,
                                            p0_+vL1, rm*p_i+vL1, p_rc+vL1, rp*p_i+vL1
                                        ),
                                        nu1, nu2, nx
                                      )
                        );
            if ( i->lowerEnd && base ) {
                base->addFace ( bl.face ( "0321" ) );
            }
            if ( i->upperEnd && top ) {
                top->addFace ( bl.face ( "4567" ) );
            }
            if (pcyclm) pcyclm->addFace(bl.face("0154"));
            if (pcyclp) pcyclp->addFace(bl.face("0473"));
        }
      }
      else
      {
        p_i=p0_+0.5*p_.geometry.d*er_; //point_on_spine(0.5*p_.geometry.d);
        p_rc=p_i;
      }

      p_o=p0_+0.5*p_.geometry.D*er_; //point_on_spine(0.5*p_.geometry.D);

      {
          Block& bl = this->addBlock
                      (
                          new Block ( P_8 (
                                          rm*p_i+vL0, rm*p_o+vL0, p_o+vL0, p_rc+vL0,
                                          rm*p_i+vL1, rm*p_o+vL1, p_o+vL1, p_rc+vL1
                                      ),
                                      nr, nu1, nx,
                                      list_of<double> ( 1./p_.mesh.gradr ) ( 1 ) ( 1 )
                                    )
                      );
          if ( i->lowerEnd && base ) {
              base->addFace ( bl.face ( "0321" ) );
          }
          if ( i->upperEnd && top ) {
              top->addFace ( bl.face ( "4567" ) );
          }
          if ( i->outerPatch ) {
              i->outerPatch->addFace ( bl.face ( "1265" ) );
          }
          if ( inner ) {
              inner->addFace ( bl.face ( "0473" ) );
          }
          if (pcyclm) pcyclm->addFace(bl.face("0154"));
      }
      {
          Block& bl = this->addBlock
                      (
                          new Block ( P_8 (
                                          p_rc+vL0, p_o+vL0, rp*p_o+vL0, rp*p_i+vL0,
                                          p_rc+vL1, p_o+vL1, rp*p_o+vL1, rp*p_i+vL1
                                      ),
                                      nr, nu2, nx,
                                      list_of<double> ( 1./p_.mesh.gradr ) ( 1 ) ( 1 )
                                    )
                      );
          if ( i->lowerEnd && base ) {
              base->addFace ( bl.face ( "0321" ) );
          }
          if ( i->upperEnd && top ) {
              top->addFace ( bl.face ( "4567" ) );
          }
          if ( i->outerPatch ) {
              i->outerPatch->addFace ( bl.face ( "1265" ) );
          }
          if ( inner ) {
              inner->addFace ( bl.face ( "0473" ) );
          }
          if (pcyclp) pcyclp->addFace(bl.face("2376"));
      }


      std::vector<arma::mat> vls;
      if (i->lowerEnd) vls.push_back(vL0);
      if (i->upperEnd) vls.push_back(vL1);
      for (auto vl: vls)
      {
        this->addEdge ( new ArcEdge ( rm*p_o +vl, p_o +vl, rotMatrix(-0.25*p_.geometry.wedge_angle*SI::deg, ex_)*p_o +vl ) );
        this->addEdge ( new ArcEdge ( p_o +vl, rp*p_o +vl, rotMatrix(0.25*p_.geometry.wedge_angle*SI::deg, ex_)*p_o +vl ) );

        if (! (p_.geometry.d < 1e-10))
        {
          this->addEdge ( new ArcEdge ( rm*p_i +vl, p_i +vl, rotMatrix(-0.25*p_.geometry.wedge_angle*SI::deg, ex_)*p_i +vl ) );
          this->addEdge ( new ArcEdge ( p_i +vl, rp*p_i +vl, rotMatrix(0.25*p_.geometry.wedge_angle*SI::deg, ex_)*p_i +vl ) );
        }
      }
    }




}


double blockMeshDict_CylWedge::rCore() const
{
    return p_.geometry.D*p_.mesh.core_fraction;
}



}
}
