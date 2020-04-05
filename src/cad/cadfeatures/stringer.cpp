#include "stringer.h"

#include <memory>

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;

namespace insight {
namespace cad {

defineType(Stringer);
addToFactoryTable(Feature, Stringer);

size_t Stringer::calcHash() const
{
  ParameterListHash h;
  h+=*spine_;
  h+=normal_->value();
  h+=t_->value();
  h+=w_->value();
  h+=delta_->value();
  h+=ext0_->value();
  h+=ext1_->value();
  return h.getHash();
}

void Stringer::build()
{
  double t = t_->value(), w=w_->value(), delta=delta_->value(),
      ext0=ext0_->value(),
      ext1=ext1_->value()
      ;

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
  gp_Vec t0, n0=to_Vec(normal_->value());
  n0.Normalize();
  spinec.D1(spinec.FirstParameter(), p0, t0);
  t0.Normalize();
  gp_Vec ey = t0.Crossed(n0);

  if (ext0>0.0)
  {
    gp_Pnt p00=p0.Translated(t0.Scaled(-ext0));
    edgs.Append(BRepBuilderAPI_MakeEdge(p0, p00).Edge());
    p0=p00;
  }

  if (ext1>0.0)
  {
    gp_Pnt p1;
    gp_Vec t1;
    spinec.D1(spinec.LastParameter(), p1, t1);
    t1.Normalize();
    gp_Pnt p11=p1.Translated(t1.Scaled(ext1));
    edgs.Append(BRepBuilderAPI_MakeEdge(p1, p11).Edge());
  }

  {
    BRepBuilderAPI_MakeWire w;
    w.Add(edgs);
    spine=w.Wire();
  }

  TopoDS_Wire xsec;
  {
    BRepBuilderAPI_MakeWire xsec_w;
    gp_Pnt p[4];
    p[0]=p0 .Translated(ey.Scaled(delta)) .Translated(n0.Scaled(-t*0.5));
    p[1]=p[0].Translated(ey.Scaled(w));
    p[2]=p[1].Translated(n0.Scaled(t));
    p[3]=p[0].Translated(n0.Scaled(t));
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

Stringer::Stringer(
    FeaturePtr spine,
    VectorPtr normal,
    ScalarPtr t,
    ScalarPtr w,
    ScalarPtr delta,
    ScalarPtr ext0,
    ScalarPtr ext1
)
  : spine_(spine),
    normal_(normal),
    t_(t), w_(w), delta_(delta),
    ext0_(ext0), ext1_(ext1)
{
}

Stringer::Stringer()
{
}

FeaturePtr Stringer::create(
    FeaturePtr spine,
    VectorPtr normal,
    ScalarPtr t,
    ScalarPtr w,
    ScalarPtr delta,
    ScalarPtr ext0,
    ScalarPtr ext1
)
{
  return FeaturePtr(new Stringer(spine, normal, t, w, delta, ext0, ext1));
}


void Stringer::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
      "Stringer",
      typename parser::ISCADParser::ModelstepRulePtr(
          new typename parser::ISCADParser::ModelstepRule(

                  ( '('
                    >> ruleset.r_solidmodel_expression >> ',' // 1
                    >> ruleset.r_vectorExpression >> ',' // 2
                    >> ruleset.r_scalarExpression >> ',' // 3
                    >> ruleset.r_scalarExpression >> ',' // 4
                    >> ruleset.r_scalarExpression // 5
                    >> ( ( ',' >> qi::lit("ext0") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0)) ) // 6
                    >> ( ( ',' >> qi::lit("ext1") >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0)) ) // 7
                    >> ')' )
                  [ qi::_val = phx::bind(&Stringer::create,
                                   qi::_1, qi::_2,
                                   qi::_3, qi::_4, qi::_5,
                                   qi::_6, qi::_7
                               ) ]

              ))
  );
}

FeatureCmdInfoList Stringer::ruleDocumentation() const
{
  return boost::assign::list_of
  (
      FeatureCmdInfo
      (
          "Stringer",

          "( <feature:spine>, <vector:n>, <scalar:t>, scalar:w> [, ext0 <scalar:ext0> ] [, ext1 <scalar:ext1> ] )",

          "Creates a stringer along a curve or a wire on a surface."
      )
  );
}



} // namespace cad
} // namespace insightStringer::
