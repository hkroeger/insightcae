#include "tongue.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;

namespace insight {
namespace cad {

defineType(Tongue);
addToFactoryTable(Feature, Tongue);

size_t Tongue::calcHash() const
{
  ParameterListHash h;
  h+=*spine_;
  h+=direction_->value();
  h+=insidePt_->value();
  h+=t_->value();
  h+=w_->value();
  h+=ovl_->value();
  h+=delta_->value();
  return h.getHash();
}

void Tongue::build()
{
  gp_Pnt ctr = to_Pnt(insidePt_->value());
  double t = t_->value(), w=w_->value(), ovl=ovl_->value(), delta=delta_->value();

  TopTools_ListOfShape edgs;
  for (TopExp_Explorer ex(spine_->shape(), TopAbs_EDGE); ex.More(); ex.Next())
  {
      TopoDS_Edge e=TopoDS::Edge(ex.Current());
      edgs.Append(e);
  }

  TopoDS_Wire spine;

  {
    BRepBuilderAPI_MakeWire w;
    w.Add(edgs);
    spine=w.Wire();
  }

  BRepAdaptor_CompCurve spinec(spine);
  gp_Pnt p0;
  gp_Vec t0, ex=to_Vec(direction_->value());
  ex.Normalize();
  spinec.D1(spinec.FirstParameter(), p0, t0);
  t0.Normalize();
  gp_Vec ey = t0.Crossed(ex);

  if ( (p0.XYZ()-ctr.XYZ()).Dot(ey.XYZ())>0 )
  {
    ey.Reverse();
  }

  TopoDS_Wire xsec;
  {
    BRepBuilderAPI_MakeWire xsec_w;
    gp_Pnt p[4];
    p[0]=p0 .Translated(ey.Scaled(delta)) .Translated(ex.Scaled(-ovl));
    p[1]=p[0].Translated(ey.Scaled(t));
    p[2]=p[1].Translated(ex.Scaled(ovl+w));
    p[3]=p[2].Translated(ey.Scaled(-t));
    TopTools_ListOfShape xsecedgs;
    xsecedgs.Append(BRepBuilderAPI_MakeEdge(p[0], p[1]).Edge());
    xsecedgs.Append(BRepBuilderAPI_MakeEdge(p[1], p[2]).Edge());
    xsecedgs.Append(BRepBuilderAPI_MakeEdge(p[2], p[3]).Edge());
    xsecedgs.Append(BRepBuilderAPI_MakeEdge(p[3], p[0]).Edge());
    xsec_w.Add(xsecedgs);
//    xsec=BRepBuilderAPI_MakeFace(xsec_w.Wire()).Face();
    xsec=xsec_w.Wire();
  }

  TopoDS_Compound result;
  BRep_Builder builder;
  builder.MakeCompound( result );

  BRepOffsetAPI_MakePipeShell p(spine);
  p.SetTransitionMode(BRepBuilderAPI_RightCorner);
  p.Add(xsec);
  p.Build();
  p.MakeSolid();
  builder.Add(result, p.Shape());

  setShape(result);
}

Tongue::Tongue(
    FeaturePtr spine,
    VectorPtr direction,
    VectorPtr insidePt,
    ScalarPtr t,
    ScalarPtr w,
    ScalarPtr ovl,
    ScalarPtr delta
  )
  : spine_(spine), direction_(direction), insidePt_(insidePt),
    t_(t), w_(w), ovl_(ovl), delta_(delta)
{
}


Tongue::Tongue()
{
}

FeaturePtr Tongue::create(
    FeaturePtr spine,
    VectorPtr direction,
    VectorPtr insidePt,
    ScalarPtr t,
    ScalarPtr w,
    ScalarPtr ovl,
    ScalarPtr delta
)
{
   return FeaturePtr(new Tongue(spine, direction, insidePt, t, w, ovl, delta));
}


void Tongue::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
      "Tongue",
      typename parser::ISCADParser::ModelstepRulePtr(
          new typename parser::ISCADParser::ModelstepRule(

                  ( '('
                    >> ruleset.r_solidmodel_expression >> ',' // 1
                    >> ruleset.r_vectorExpression >> ',' // 2
                    >> ruleset.r_vectorExpression >> ',' // 3
                    >> ruleset.r_scalarExpression >> ',' // 4
                    >> ruleset.r_scalarExpression >> ',' // 5
                    >> ruleset.r_scalarExpression >> ',' // 6
                    >> ruleset.r_scalarExpression // 7
                    >> ')' )
                  [ qi::_val = phx::bind(&Tongue::create,
                                   qi::_1, qi::_2,
                                   qi::_3, qi::_4, qi::_5,
                                   qi::_6, qi::_7
                               ) ]

              ))
  );
}

FeatureCmdInfoList Tongue::ruleDocumentation() const
{
  return boost::assign::list_of
  (
      FeatureCmdInfo
      (
          "Tongue",

          "( <feature:spine>, <vector:direction>, <vector:insidePt>, <scalar:t>, scalar:w>, <scalar:overlap>, <scalar:delta> )",

          "Creates a tongue extension along a section curve between two surface parts."
      )
  );
}

} // namespace cad
} // namespace insight
