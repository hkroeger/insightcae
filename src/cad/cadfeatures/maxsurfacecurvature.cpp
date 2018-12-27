#include "maxsurfacecurvature.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

#include "Geom2d_BSplineCurve.hxx"
#include "Geom2dAPI_Interpolate.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {




defineType(MaxSurfaceCurvature);
addToFactoryTable(Feature, MaxSurfaceCurvature);


size_t MaxSurfaceCurvature::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*face_;
  return h.getHash();
}


MaxSurfaceCurvature::MaxSurfaceCurvature()
{}




void MaxSurfaceCurvature::build()
{
  TopoDS_Face f=TopoDS::Face(face_->shape());

  class Obj : public ObjectiveND
  {
  public:
    Handle_Geom_Surface surf;
    mutable GeomLProp_SLProps props;
    double u1, u2, v1, v2;

    Obj(const TopoDS_Face& f)
    : surf(BRep_Tool::Surface(f)),
      props(surf, 2, 1e-2)
    {
      surf->Bounds(u1, u2, v1, v2);
      cout<<"u1,...="<<u1<<", "<<u2<<", "<<v1<<", "<<v2<<endl;
    }

    gp_XY uv(const arma::mat& x) const
    {
      double u=std::min(u2, std::max(u1, u1+(u2-u1)*x(0)));
      double v=std::min(v2, std::max(v1, v1+(v2-v1)*x(1)));
      return gp_XY(u, v);
    }

    virtual double operator()(const arma::mat& x) const
    {
      auto p = uv(x);
      props.SetParameters(p.X(), p.Y());
      return -fabs(props.MaxCurvature());
    }

    virtual int numP() const { return 2; }

  } obj(f);

  double delta_uv_max=std::max(obj.u2-obj.u1, obj.v2-obj.v1);

  arma::mat x0;
  x0 << 0.5 << 0.5;
  arma::mat x = nonlinearMinimizeND(obj, x0);
  gp_XY uv0=obj.uv(x);

  std::vector<gp_XY> pts;

  const double itermax=10000;
  const double pdist=1e-2*delta_uv_max;

  for (double dir: {1., -1.})
  {
    gp_XY uv=uv0;
    cout<<"uv="<<uv.X()<<", "<<uv.Y()<<endl;
    int iter=0;
    while ( (uv.X()-obj.u1>pdist)&&(obj.u2-uv.X()>pdist) && (uv.Y()-obj.v1>pdist)&&(obj.v2-uv.Y()>pdist) && (iter<itermax) )
    {
      iter++;
      if (dir>0)
        pts.push_back(uv);
      else if (iter>1)
        pts.insert(pts.begin(), uv);

      obj.props.SetParameters(uv.X(), uv.Y());
      gp_Dir maxc, minc, minc_uv;
      gp_Vec d1u=obj.props.D1U();
      gp_Vec d1v=obj.props.D1V();
      d1u.Normalize();
      d1v.Normalize();

      obj.props.CurvatureDirections(maxc, minc);

      cout<<"maxc=["<<maxc.X()<<","<<maxc.Y()<<","<<maxc.Z()<<"]"<<endl;
      cout<<"minc=["<<minc.X()<<","<<minc.Y()<<","<<minc.Z()<<"]"<<endl;

      gp_XY duv(
            minc.XYZ().Dot(d1u.XYZ()),
            minc.XYZ().Dot(d1v.XYZ())
            );
      duv.Normalize();

      uv += duv*dir * 1e-2*delta_uv_max;

      // minimize lateral
      class Obj2 : public Obj
      {
      public:
        gp_XY a,b;

        Obj2(const TopoDS_Face& f, gp_XY uv_start, gp_XY uv_dir)
          : Obj(f), a(uv_dir), b(uv_start)
        {}

        gp_XY curuv(const arma::mat& t) const
        {
          gp_XY uv=a*t(0)+b;
          double u=std::min(u2, std::max(u1, uv.X()));
          double v=std::min(v2, std::max(v1, uv.Y()));
          return gp_XY(u, v);
        }

        virtual double operator()(const arma::mat& x) const
        {
          gp_XY uv = curuv(x);
          props.SetParameters(uv.X(), uv.Y());
          double C=-std::max(fabs(props.MaxCurvature()), fabs(props.MinCurvature()));
          cout<<"t="<<x(0)<<", uv="<<uv.X()<<" "<<uv.Y()<<", C="<<C<<" "<<props.MinCurvature()<<" "<<props.MaxCurvature()<<endl;
          return C;
        }

        virtual int numP() const { return 1; }
      }
        obj2(f, uv,
             gp_XY(
               maxc.XYZ().Dot(d1u.XYZ()),
               maxc.XYZ().Dot(d1v.XYZ())
               )
             );

      cout<<"uv="<<uv.X()<<", "<<uv.Y()<<" => "<<flush;

      arma::mat t0, steps;
      t0 << 0.;
      steps<<1e-2*delta_uv_max;
      arma::mat t = nonlinearMinimizeND(obj2, t0, 1e-6, steps);
      uv=obj2.curuv(t);

      cout<<"uv="<<uv.X()<<", "<<uv.Y()<<endl;
    }
  }

//  TopoDS_Compound res;
//  BRep_Builder bb;
//  bb.MakeCompound ( res );
//  std::ofstream dbg("uv.txt");
//  for (size_t i=0; i<pts.size(); i++)
//  {
//    dbg<<pts[i].X()<<" "<<pts[i].Y()<<endl;
//    bb.Add(res, BRepBuilderAPI_MakeVertex(obj.surf->Value(pts[i].X(), pts[i].Y())));
//  }
//  setShape(res);


  cout<<"ipol"<<endl;
  Handle_TColgp_HArray1OfPnt2d pts2(new TColgp_HArray1OfPnt2d(1, pts.size()));
  for (size_t i=0; i<pts.size(); i++) pts2->SetValue(i+1, pts[i]);
  Geom2dAPI_Interpolate ip(pts2, false, 1e-3);
  ip.Perform();
  if (!ip.IsDone())
  {
    throw insight::Exception("Building 2D spline failed!");
  }

  cout<<"edge"<<endl;
  TopoDS_Edge ec = BRepBuilderAPI_MakeEdge(ip.Curve(), obj.surf).Edge();
  BRepLib::BuildCurve3d(ec);
//  Handle_Geom_Curve crv;
//  setShape(BRepBuilderAPI_MakeEdge(crv));

  cout<<"done"<<endl;
  setShape(ec);

}




MaxSurfaceCurvature::MaxSurfaceCurvature(FeaturePtr face)
: face_(face)
{
}




FeaturePtr MaxSurfaceCurvature::create(FeaturePtr face)
{
    return FeaturePtr(new MaxSurfaceCurvature(face));
}



MaxSurfaceCurvature::operator const TopoDS_Edge& () const
{
  return TopoDS::Edge(shape());
}




void MaxSurfaceCurvature::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "MaxSurfaceCurvature",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

    ( '(' >> ruleset.r_solidmodel_expression >> ')' )
        [ qi::_val = phx::bind(&MaxSurfaceCurvature::create, qi::_1) ]

    ))
  );
}




FeatureCmdInfoList MaxSurfaceCurvature::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "MaxSurfaceCurvature",

            "( <feature:face> )",

            "Computes the maximum curvature line on a surface originating from the point of maximum curvature."
        )
    );
}



}
}

