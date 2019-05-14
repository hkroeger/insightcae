#include "blockmesh_cylwedge.h"

#include "base/boost_include.h"

#include "base/units.h"

#include "occinclude.h"
#include "GeomAPI_IntCS.hxx"
#include "Geom_CylindricalSurface.hxx"
#include "cadfeatures/importsolidmodel.h"
#include "cadfeatures/splinecurve.h"

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
{}


void blockMeshDict_CylWedge::create_bmd()
{
    this->setDefaultPatch(p_.mesh.defaultPatchName);

    arma::mat p0=p_.geometry.p0;
    arma::mat ex=p_.geometry.ex;
    arma::mat er=p_.geometry.er;
    arma::mat ey=BlockMeshTemplate::correct_trihedron(ex, er);



    // Read or create spine curve
    Handle_Geom_Curve spine;
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
      spine = BRep_Tool::Curve(e, t0, t1);

      // align er with beginning (inner) of spine
      arma::mat per=vec3(spine->Value(t0));
      arma::mat p2=vec3(spine->Value(t1));
      auto rdist = [&](const arma::mat& p, const arma::mat& ex) -> double
      {
        arma::mat r=p-p0;
        return arma::norm(r - arma::dot(r,ex)*ex, 2);
      };

      if ( rdist(p2,p0) < rdist(per,p0) ) per=p2;

      er = per-p0;
      er -= arma::dot(er,ex)*ex;
      ey = BlockMeshTemplate::correct_trihedron(ex, er);
    }
    else
    {
      spine = GC_MakeSegment(
                to_Pnt(p0 + er*p_.geometry.d*0.5),
                to_Pnt(p0 + er*p_.geometry.D*0.5)
                ).Value();
    }



    // helper function
    auto point_on_spine = [&](double r) {
      cout<<"r="<<r<<endl;
      Handle_Geom_Surface cyl(new  Geom_CylindricalSurface(gp_Ax3(to_Pnt(p0), to_Vec(ex), to_Vec(er)), r));
      GeomAPI_IntCS isec(spine, cyl);
      cout<<isec.NbPoints()<<endl;
      if (isec.NbPoints()<1)
        throw insight::Exception(boost::str(boost::format(
                                              "No intersection point with spine found for r=%g!"
                                              ) % r));
      std::map<double,arma::mat> res;
      for (int i=0;i<isec.NbPoints();i++)
      {
        arma::mat p=vec3(isec.Point(i+1));
        arma::mat r=p-p0;
        double odist = arma::norm(r-std::max<double>(0.,dot(r, er))*er, 2);
//        cout<<odist<<" => "<<p<<endl;
        res[odist]=p;
      }
      auto p_sel = res.begin()->second;

      p_sel -= dot(p_sel-p0, ex)*ex;
      return p_sel;
    };



    double rc=rCore();
    arma::mat vL=p_.geometry.L*ex;

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

    gp_Ax1 ax(to_Pnt(p0), to_Vec(ex));
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

    std::vector<double> phi;
//    double phim, phip;
    {
      auto calc_angle = [&](const arma::mat& p) -> double
      {
        double y=arma::dot(p-p0, -ey);
        double x=arma::dot(p-p0, er);
        return std::atan2(y, x);
      };

//      struct Obj : public Objective1D {

//        double sign=1.;
//        std::function<arma::mat(double)> point;
//        std::function<double(const arma::mat& p)> calc_angle;

//        virtual double operator()(double d) const
//        {
//          double phi=calc_angle(point(0.5*d));
//          cout<<">> " << d<<" => "<<phi<<endl;
//          return sign*phi;
//        }
//      } objExtrAngle;

//      objExtrAngle.point=point_on_spine;
//      objExtrAngle.calc_angle=calc_angle;

//      objExtrAngle.sign=1.0;
//      phim=nonlinearMinimize1D(objExtrAngle, p_.geometry.d, p_.geometry.D) -0.5*p_.geometry.wedge_angle*SI::deg;
//      objExtrAngle.sign=-1.0;
//      phip=nonlinearMinimize1D(objExtrAngle, p_.geometry.d, p_.geometry.D) +0.5*p_.geometry.wedge_angle*SI::deg;
      const int np=20;
      for (int i=0; i<np; i++)
      {
        double d=p_.geometry.d + double(i)/double(np-1)*(p_.geometry.D-p_.geometry.d);
        phi.push_back(calc_angle(point_on_spine(0.5*d)));
      }

//      phi.push_back(calc_angle(vec3(BRep_Tool::Pnt(TopExp::FirstVertex(c0m)))));
//      phi.push_back(calc_angle(vec3(BRep_Tool::Pnt(TopExp::LastVertex(c0m)))));
//      phi.push_back(calc_angle(vec3(BRep_Tool::Pnt(TopExp::FirstVertex(c0p)))));
//      phi.push_back(calc_angle(vec3(BRep_Tool::Pnt(TopExp::LastVertex(c0p)))));
    }
    double phim=*std::min_element(phi.begin(), phi.end()) -0.5*p_.geometry.wedge_angle*SI::deg;
    double phip=*std::max_element(phi.begin(), phi.end()) +0.5*p_.geometry.wedge_angle*SI::deg;

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

    arma::mat rp=rotMatrix ( phip /*0.5*p_.geometry.wedge_angle*SI::deg*/, ex );
    arma::mat rm=rotMatrix ( phim /*-0.5*p_.geometry.wedge_angle*SI::deg*/, ex );

    arma::mat p_rc, p_i, p_o;
    if (p_.geometry.d < 1e-10)
    {
      // build mesh with core block

      p_i=p0+0.9*rc*er; //point_on_spine(0.9*rc);
      p_rc=p0+rc*er; //point_on_spine(rc);
      // core block
      {

          Block& bl = this->addBlock
                      (
                          new Block ( P_8 (
                                          p0, rm*p_i, p_rc, rp*p_i,
                                          p0+vL, rm*p_i+vL, p_rc+vL, rp*p_i+vL
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
          if (pcyclm) pcyclm->addFace(bl.face("0473"));
      }
    }
    else
    {
      p_i=p0+0.5*p_.geometry.d*er; //point_on_spine(0.5*p_.geometry.d);
      p_rc=p_i;
    }

    p_o=p0+0.5*p_.geometry.D*er; //point_on_spine(0.5*p_.geometry.D);

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
      this->addEdge ( new ArcEdge ( rm*p_o +vl, p_o +vl, rotMatrix(-0.25*p_.geometry.wedge_angle*SI::deg, ex)*p_o +vl ) );
      this->addEdge ( new ArcEdge ( p_o +vl, rp*p_o +vl, rotMatrix(0.25*p_.geometry.wedge_angle*SI::deg, ex)*p_o +vl ) );

      if (! (p_.geometry.d < 1e-10))
      {
        this->addEdge ( new ArcEdge ( rm*p_i +vl, p_i +vl, rotMatrix(-0.25*p_.geometry.wedge_angle*SI::deg, ex)*p_i +vl ) );
        this->addEdge ( new ArcEdge ( p_i +vl, rp*p_i +vl, rotMatrix(0.25*p_.geometry.wedge_angle*SI::deg, ex)*p_i +vl ) );
      }
    }




}


double blockMeshDict_CylWedge::rCore() const
{
    return p_.geometry.D*p_.mesh.core_fraction;
}

}
}
