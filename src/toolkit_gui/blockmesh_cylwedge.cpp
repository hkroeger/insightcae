#include "blockmesh_cylwedge.h"

#include "base/boost_include.h"

#include "base/units.h"

#include "occinclude.h"
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

using namespace boost;
using namespace boost::assign;

namespace insight
{

namespace bmd
{



defineType ( blockMeshDict_CylWedge );
addToOpenFOAMCaseElementFactoryTable(blockMeshDict_CylWedge );


blockMeshDict_CylWedge::blockMeshDict_CylWedge ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c ), p_ ( ps )
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
    if (!p_.geometry.wedge_spine_curve.empty())
    {
      insight::cad::FeaturePtr wsc = insight::cad::Import::create(p_.geometry.wedge_spine_curve);
      wsc->checkForBuildDuringAccess(); // force rebuild
      auto el = wsc->allEdgesSet();

      if (el.size()!=1)
        throw insight::Exception(
            boost::str(boost::format("CAD file %s should contain only one single edge! (It actually contains %d edges)")
                       % p_.geometry.wedge_spine_curve.string() % el.size() )
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
    arma::mat vL=p_.geometry.L*ex_;

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






    const int np=10;
    TopoDS_Edge c0;
    {
      std::vector<insight::cad::VectorPtr> pts;
      for (int i=0; i<np; i++)
      {
        double r=0.5*(p_.geometry.d + (p_.geometry.D-p_.geometry.d)*double(i)/double(np-1));
        pts.push_back(insight::cad::matconst(point_on_spine(r)));
      }
      cad::FeaturePtr spc=insight::cad::SplineCurve::create(pts);
      c0=TopoDS::Edge(spc->shape());
    }

    gp_Ax1 ax(to_Pnt(p0_), to_Vec(ex_));
    gp_Trsf trm; trm.SetRotation(ax, -0.5*p_.geometry.wedge_angle*SI::deg);
    gp_Trsf trp; trp.SetRotation(ax, 0.5*p_.geometry.wedge_angle*SI::deg);
    gp_Trsf tru; tru.SetTranslation(to_Vec(vL));

    TopoDS_Edge c0m=TopoDS::Edge(BRepBuilderAPI_Transform(c0, trm).Shape());
    TopoDS_Edge c0p=TopoDS::Edge(BRepBuilderAPI_Transform(c0, trp).Shape());
    TopoDS_Face cyclm = BRepFill::Face ( c0m,  TopoDS::Edge(BRepBuilderAPI_Transform(c0m, tru).Shape()));
    TopoDS_Face cyclp = BRepFill::Face ( c0p,  TopoDS::Edge(BRepBuilderAPI_Transform(c0p, tru).Shape()));

    {
      StlAPI_Writer stlwriter;
      stlwriter.ASCIIMode() = true;
      BRepMesh_IncrementalMesh Incm(cyclm, 1e-2);
      stlwriter.Write(cyclm, "cyclm.stl");
    }
    {
      StlAPI_Writer stlwriter;
      stlwriter.ASCIIMode() = true;
      BRepMesh_IncrementalMesh Incp(cyclp, 1e-2);
      stlwriter.Write(cyclp, "cyclp.stl");
    }


    auto phi_lim = limit_angles();
    double phim=phi_lim.first;
    double phip=phi_lim.second;
    cout<<"phim="<<phim<<", phip="<<phip<<endl;


    int nu1, nu2, nx, nr;
    double
        L_r = 0.5*(p_.geometry.D-p_.geometry.d),
        L_u1 = fabs(phim)*0.25*(p_.geometry.d+p_.geometry.D),
        L_u2 = fabs(phip)*0.25*(p_.geometry.d+p_.geometry.D);
    if (const auto* ic = boost::get<Parameters::mesh_type::resolution_individual_type>(&p_.mesh.resolution))
    {
      nu1=int(std::ceil( fabs(phim)/fabs(phip-phim)*double(ic->nu) ));
      nu2=std::max(1, ic->nu - nu1);
      nx=ic->nx;
      nr=ic->nr;
    }
    else if (const auto* ic = boost::get<Parameters::mesh_type::resolution_cubical_size_type>(&p_.mesh.resolution))
    {
      nx=std::max(1, int(std::ceil(p_.geometry.L/ic->delta)));
      nr=std::max(1, int(std::ceil(L_r/ic->delta)));
      nu1=std::max(1, int(std::ceil(L_u1/ic->delta)));
      nu2=std::max(1, int(std::ceil(L_u2/ic->delta)));
    }
    else if (const auto* ic = boost::get<Parameters::mesh_type::resolution_cubical_type>(&p_.mesh.resolution))
    {
      auto Ls={p_.geometry.L, L_r, L_u1, L_u2};
      double delta = *std::max_element(Ls.begin(), Ls.end()) / double(ic->n_max);

      nx=std::max(1, int(std::ceil(p_.geometry.L/delta)));
      nr=std::max(1, int(std::ceil(L_r/delta)));
      nu1=std::max(1, int(std::ceil(L_u1/delta)));
      nu2=std::max(1, int(std::ceil(L_u2/delta)));
    }

    arma::mat rp=rotMatrix ( phip /*0.5*p_.geometry.wedge_angle*SI::deg*/, ex_ );
    arma::mat rm=rotMatrix ( phim /*-0.5*p_.geometry.wedge_angle*SI::deg*/, ex_ );

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
                                          p0_, rm*p_i, p_rc, rp*p_i,
                                          p0_+vL, rm*p_i+vL, p_rc+vL, rp*p_i+vL
                                      ),
                                      nu1, nu2, nx
                                    )
                      );
          if ( base ) {
              base->addFace ( bl.face ( "0321" ) );
          }
          if ( top ) {
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
                                        rm*p_i, rm*p_o, p_o, p_rc,
                                        rm*p_i+vL, rm*p_o+vL, p_o+vL, p_rc+vL
                                    ),
                                    nr, nu1, nx,
                                    list_of<double> ( 1./p_.mesh.gradr ) ( 1 ) ( 1 )
                                  )
                    );
        if ( base ) {
            base->addFace ( bl.face ( "0321" ) );
        }
        if ( top ) {
            top->addFace ( bl.face ( "4567" ) );
        }
        if ( outer ) {
            outer->addFace ( bl.face ( "1265" ) );
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
                                        p_rc, p_o, rp*p_o, rp*p_i,
                                        p_rc+vL, p_o+vL, rp*p_o+vL, rp*p_i+vL
                                    ),
                                    nr, nu2, nx,
                                    list_of<double> ( 1./p_.mesh.gradr ) ( 1 ) ( 1 )
                                  )
                    );
        if ( base ) {
            base->addFace ( bl.face ( "0321" ) );
        }
        if ( top ) {
            top->addFace ( bl.face ( "4567" ) );
        }
        if ( outer ) {
            outer->addFace ( bl.face ( "1265" ) );
        }
        if ( inner ) {
            inner->addFace ( bl.face ( "0473" ) );
        }
        if (pcyclp) pcyclp->addFace(bl.face("2376"));
    }



    for (auto vl: {vec3(0,0,0), vL})
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


double blockMeshDict_CylWedge::rCore() const
{
    return p_.geometry.D*p_.mesh.core_fraction;
}





ParameterSet_VisualizerPtr blockMeshDict_CylWedge_visualizer()
{
    return ParameterSet_VisualizerPtr( new blockMeshDict_CylWedge_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(OpenFOAMCaseElement, blockMeshDict_CylWedge, visualizer, blockMeshDict_CylWedge_visualizer);


void blockMeshDict_CylWedge_ParameterSet_Visualizer::update(const ParameterSet& ps)
{
    ParameterSet_Visualizer::update(ps);
}

void blockMeshDict_CylWedge_ParameterSet_Visualizer::updateVisualizationElements(QoccViewWidget* vw, QModelTree* mt)
{

    Parameters p(ps_);

    insight::cad::cache.initRebuild();

    OpenFOAMCase oc(OFEs::getCurrentOrPreferred());
    blockMeshDict_CylWedge bcw( oc, ps_ );


    auto dom =
      cad::Cutaway::create(
       cad::Cutaway::create(
        cad::BooleanSubtract::create(
          cad::Cylinder::create(
                   cad::matconst(p.geometry.p0),
                   cad::matconst(p.geometry.L*bcw.ex_),
                   cad::scalarconst(p.geometry.D),
                   true,
                   false
                   ),
          cad::Cylinder::create(
                   cad::matconst(p.geometry.p0),
                   cad::matconst(p.geometry.L*bcw.ex_),
                   cad::scalarconst(p.geometry.d),
                   true,
                   false
                   )
            ),
          cad::matconst(p.geometry.p0), cad::matconst( rotMatrix(0.5*p.geometry.wedge_angle*SI::deg, bcw.ex_)*-bcw.ey_ )
         ),
        cad::matconst(p.geometry.p0), cad::matconst( rotMatrix(-0.5*p.geometry.wedge_angle*SI::deg, bcw.ex_)*bcw.ey_ )
       );

    mt->onAddFeature( "blockMeshDict_CylWedge",
                        cad::Compound::create(cad::CompoundFeatureList({dom})),
                        true );

    insight::cad::cache.finishRebuild();
}








defineType ( blockMeshDict_CylWedgeOrtho );
addToOpenFOAMCaseElementFactoryTable(blockMeshDict_CylWedgeOrtho );




blockMeshDict_CylWedgeOrtho::blockMeshDict_CylWedgeOrtho ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c ), p_ ( ps )
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
  bool is_highest = !no_top_edg;

  auto cyl_isec = [&](double r, Handle_Geom_Curve spine, gp_Pnt nearp)
  {
      Handle_Geom_Surface cyl(new Geom_CylindricalSurface(gp_Ax3(center, ez), r));

      GeomAPI_ExtremaCurveSurface ecs1(spine, cyl);
      double dist=1e100;
      int ii=-1;
      gp_Pnt isecp;
      for (int i=1; i<=ecs1.NbExtrema(); i++)
      {
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
        throw insight::Exception("Could not find intersection between cylinder and curve!");

      double t1, dummy;
      ecs1.Parameters(ii, t1, dummy, dummy);

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
    std::cout<<"Move t0 from "<<t0;
    t0 = cyl_isec( radius(spine_rvs->Value(t0))+ii->distance, spine_rvs, spine_rvs->Value(t0) );
    std::cout<<" to "<<t0<<" (new r="<<radius(spine_rvs->Value(t0))<<", t1="<<t1<<")"<<std::endl;

    // recompute
    R0=spine_rvs->Value(t0).XYZ()-center.XYZ();
    R0 = R0 - ez.XYZ()*R0.Dot(ez.XYZ());

    if ( (z0 >= ii->z0) && (z1 <= ii->z1) )
    {
      std::cout<<"do inner!"<<std::endl;
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
    std::cout<<"Move t1 from "<<t1;
    t1 = cyl_isec( radius(spine_rvs->Value(t1))-ii->distance, spine_rvs, spine_rvs->Value(t1) );
    std::cout<<" to "<<t1<<" (new r="<<radius(spine_rvs->Value(t1))<<", t0="<<t0<<")"<<std::endl;

    // recompute
    R1=spine_rvs->Value(t1).XYZ()-center.XYZ();
    R1 = R1 - ez.XYZ()*R1.Dot(ez.XYZ());

    if ( (z0 >= ii->z0) && (z1 <= ii->z1) )
    {
      std::cout<<"do outer!"<<std::endl;
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

  std::cout<<"t00, t0, t1, t10 = "<<t00<<", "<<t0<<", "<<t1<<", "<<t10<<std::endl;


  //  Normale am Anfang Spine fwd => Skalarprod mit Kreisnormale
  spine_rvs->D1(t0, p, t); t*=sense;
  gp_Vec en_circ1( R0 );
  gp_Vec en1=ez.Crossed(t);


  // Schnittpunkt Normale/gegenüberliegende Kurve

  double u_sp1;
  if ( en_circ1.Dot(en1) > 0 )
  {
    //rückwärts gekrümmt

//    double delta= en_circ1.Angle(en1); //::acos( en_circ1.Dot(en1) /en_circ1.Magnitude()/en1.Magnitude() );
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

//    spine_rvs->D0( 0.5*(g_begin.fwd_u0+g_begin.fwd_u1), g_begin.ctr );
//    g_begin.ctr.Transform(rot_fwd_ctr);
    g_begin.interf=gp_Pnt( 0.5*(spine_rvs->Value(t0).XYZ() + spine_rvs->Value(u_sp1).Transformed(rot_fwd).XYZ() ) );
    g_begin.ctr=gp_Pnt(
                  0.8 * ( 0.5*(g_begin.interf.XYZ() + spine_rvs->Value(t0).Transformed(rot_fwd_ctr).XYZ() ) )
                  +
                  0.2 * ( spine_rvs->Value(0.5*(t0+u_sp1)).Transformed(rot_fwd).XYZ() )
                  );
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
  }

  // ===========================================================
  // ========== Geometrie Zwickel aussen
  // ===========================================================

  //  Normale am Anfang Spine fwd => Skalarprod mit Kreisnormale
  spine_rvs->D1(t1, p, t); t*=sense;
  gp_Vec en_circ2( R1 );
  gp_Vec en2=ez.Crossed(t);

  // Schnittpunkt Normale/gegenüberliegende Kurve

  double u_sp2;
  if ( en_circ2.Dot(en2) > 0 )
  {
    //rückwärts gekrümmt
//    double delta= en_circ1.Angle(en1); //::acos( en_circ1.Dot(en1) /en_circ1.Magnitude()/en1.Magnitude() );
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
  }
  else
  {
    // vorwärts gekrümmt
    throw insight::Exception("rvs: not implemented");
  }

  // Blockgeometrien
  block.fwd_u0=g_begin.fwd_u1;
  block.fwd_u1=g_end.fwd_u0;
  block.rvs_u0=g_begin.rvs_u1;
  block.rvs_u1=g_end.rvs_u0;


  // Blöcke erzeugen




  //  - Zwickel innen
  if (g_begin.collapse_pt_loc == Gusset::Rvs )
  {
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

  if (do_pro_inner_blocks)
  {
    gp_Pnt
        pa = spine_rvs->Value(t00),
        pb = spine_rvs->Value(t0)
      ;

    int nproi = std::max(1, int(pa.Distance(pb) / deltax));
    std::cout<<"nproi="<<nproi<<" "<<pa.Distance(pb)<<" "<<deltax<<std::endl;


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

  if (do_pro_outer_blocks)
  {
    gp_Pnt
        pa = spine_rvs->Value(t1),
        pb = spine_rvs->Value(t10)
      ;

    int nproo = std::max(1, int(pa.Distance(pb) / deltax));
    std::cout<<"nproo="<<nproo<<" "<<pa.Distance(pb)<<" "<<deltax<<std::endl;

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



  // ============= Blöcke

  {
    arma::mat
        pr0 = vec3(spine_rvs->Value(block.rvs_u0)),
        pr1 = vec3(spine_rvs->Value(block.rvs_u1)),
        pf0 = vec3(spine_rvs->Value(block.fwd_u0).Transformed(rot_fwd)),
        pf1 = vec3(spine_rvs->Value(block.fwd_u1).Transformed(rot_fwd))
      ;
    {
    Block& bl = this->addBlock
                (
                    new Block ( P_8 (
                                  pr0+vL0, pr1+vL0, 0.5*(pr1+pf1)+vL0, 0.5*(pr0+pf0)+vL0,
                                  pr0+vL1, pr1+vL1, 0.5*(pr1+pf1)+vL1, 0.5*(pr0+pf0)+vL1
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
                    new Block ( P_8 (
                                  0.5*(pr0+pf0)+vL0, 0.5*(pr1+pf1)+vL0, pf1+vL0, pf0+vL0,
                                  0.5*(pr0+pf0)+vL1, 0.5*(pr1+pf1)+vL1, pf1+vL1, pf0+vL1
                                ),
                                nr, nuBy2, nx
                              )
                );
    if ( is_lowest && pc.base ) pc.base->addFace ( bl.face ( "0321" ) );
    if ( is_highest && pc.top ) pc.top->addFace ( bl.face ( "4567" ) );
    if ( Patch* cp = pc.pcyclp) cp->addFace(bl.face("2376"));
    }

    auto middleCurve = [&](const PointList& c1, const PointList& c2)
    {
      PointList mc;
      for (size_t i=0; i< c1.size(); i++)
      {
        mc.push_back(0.5*(c1[i]+c2[i]));
      }
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

      auto sp3=new SplineEdge(middleCurve(sp1->allPoints(), sp2->allPoints()));
      this->addEdge ( sp3 );

      if (!no_top_edg)
        this->addEdge ( sp3->transformed( rotMatrix(0), vL1-vL0 ) );
    }
  }

}

void blockMeshDict_CylWedgeOrtho::create_bmd()
{
    this->setDefaultPatch(p_.mesh.defaultPatchName);



    // Read or create spine curve
    Handle_Geom_Curve spine;
    double t0, t1;


    // load geometry
    insight::cad::FeaturePtr wsc = insight::cad::Import::create(p_.geometry.wedge_spine_curve);
    wsc->checkForBuildDuringAccess(); // force rebuild
    auto el = wsc->allEdgesSet();

    if (el.size()!=1)
      throw insight::Exception(
          boost::str(boost::format("CAD file %s should contain only one single edge! (It actually contains %d edges)")
                     % p_.geometry.wedge_spine_curve.string() % el.size() )
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
    double Lu = ( R_midp - ez.Dot(R_midp)*ez ).Modulus() * p_.geometry.wedge_angle*SI::deg;

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
