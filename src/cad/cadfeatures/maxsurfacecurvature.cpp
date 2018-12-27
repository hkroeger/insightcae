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

    Obj(TopoDS_Face f)
    : surf(BRep_Tool::Surface(f)),
      props(surf, 2, 1e-2)
    {
      surf->Bounds(u1, u2, v1, v2);
      cout<<"u1,...="<<u1<<", "<<u2<<", "<<v1<<", "<<v2<<endl;
    }

    std::pair<double,double> uv(const arma::mat& x) const
    {
      double u=std::min(u2, std::max(u1, u1+(u2-u1)*x(0)));
      double v=std::min(v2, std::max(v1, v1+(v2-v1)*x(1)));
      return std::pair<double,double>(u, v);
    }

    virtual double operator()(const arma::mat& x) const
    {
      auto p = uv(x);
      props.SetParameters(p.first, p.second);
      return -fabs(props.MaxCurvature());
    }

    virtual int numP() const { return 2; }

  } obj(f);

  arma::mat x0;
  x0 << 0.5 << 0.5;
  arma::mat x = nonlinearMinimizeND(obj, x0);
  auto uv=obj.uv(x);
  cout<<"uv="<<uv.first<<", "<<uv.second<<endl;

  std::vector<gp_XY> pts;
  double dt=1e-4;

  while ( (uv.first>obj.u1)&&(uv.first<obj.u2) && (uv.second>obj.v1)&&(uv.second<obj.v2) )
  {
    pts.push_back(gp_XY(uv.first, uv.second));

    obj.props.SetParameters(uv.first, uv.second);
    gp_Dir maxc, minc, minc_uv;
    gp_Vec d1u=obj.props.D1U();
    gp_Vec d1v=obj.props.D1V();
    obj.props.CurvatureDirections(maxc, minc);
    double du=maxc.XYZ().Dot(d1u.XYZ());
    double dv=maxc.XYZ().Dot(d1v.XYZ());
    uv.first+=du*dt;
    uv.second+=dv*dt;

    cout<<"uv="<<uv.first<<", "<<uv.second<<endl;
  }
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

