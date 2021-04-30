#include "cutup.h"
#include "cylinder.h"
#include "datum.h"

#include "booleanintersection.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/tools.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {




defineType(CutUp);
addToFactoryTable(Feature, CutUp);


size_t CutUp::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*model_;
  for (const auto& c: clips_)
  {
    h+=c->value();
  }
  h+=n_->value();
  h+=t_->value();
  return h.getHash();
}


CutUp::CutUp(): Feature()
{}





CutUp::CutUp(FeaturePtr model, VectorPtr n, ScalarPtr t, Clips clips)
: Feature(), model_(model), clips_(clips), n_(n), t_(t)
{}





FeaturePtr CutUp::create ( FeaturePtr model, VectorPtr n, ScalarPtr t, Clips clips )
{
    return FeaturePtr(new CutUp(model, n, t, clips));
}





void CutUp::build()
{
  ExecTimer t("CutUp::build() ["+featureSymbolName()+"]");

  if (!cache.contains(hash()))
  {

    auto bb = model_->modelBndBox();
    auto n = n_->value();
    double ln=arma::norm(n, 2);
    insight::assertion(fabs(ln)>1e-10, "normal vector must not be zero");
    n/=ln;

    arma::mat Ldiag = bb.col(1)-bb.col(0);
    arma::mat pctr = 0.5*(bb.col(1)+bb.col(0));
    pctr -= arma::dot(pctr, n)*n;

    double Ln = arma::dot(Ldiag, n);

    double Lmax = Ldiag.max();


    // collect section coordinates
    std::vector<double> mks = { arma::dot(bb.col(0)-pctr, n) }, ts={0};
    for (const auto& c: clips_)
    {
      arma::mat p = c->value();

      double x = arma::dot(p-pctr, n);

      mks.push_back(x);
      ts.push_back(t_->value());
    }
    mks.push_back(arma::dot(bb.col(1)-pctr, n));
    ts.push_back(0);

    for (size_t i=1; i<mks.size(); i++)
    {
      double x0=mks[i-1] + ts[i-1] * 0.5;
      double x1=mks[i]   - ts[i]   * 0.5;

      auto p0 = matconst(pctr + x0 * n);
      auto p1 = matconst(pctr + x1 * n);

      auto tool=Cylinder::create(p0, p1, scalarconst(Lmax), false, false);

      refpoints_[str(format("p0_%d")%i)]=x0 * n;
      refpoints_[str(format("p1_%d")%i)]=x1 * n;

      providedSubshapes_[str(format("tool_%d")%i)]=tool;
      providedSubshapes_[str(format("cut_%d")%i)]=BooleanIntersection::create(model_, tool);
    }

    setShape(TopoDS_Shape());
  }
  else
  {
    this->operator=(*cache.markAsUsed<CutUp>(hash()));
  }
}




/** @addtogroup cad_parser
  * @{
  *
  * @section CutUp Cut away a halfspace
  * Remove everything beyond a plane from a feature
  *
  * Syntax:
  * ~~~~
  * CutUp(<feature expression: model>, <vector: p0>, <vector: n>) : feature
  * ~~~~
  * @}
  */
void CutUp::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "CutUp",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

    ( '(' >>
        ruleset.r_solidmodel_expression >> ',' >>
        ruleset.r_vectorExpression >> ',' >>
        ruleset.r_scalarExpression >> ',' >>
        ruleset.r_vectorExpression % ','
      >> ')' )
      [ qi::_val = phx::bind(&CutUp::create, qi::_1, qi::_2, qi::_3, qi::_4) ]
    ))
  );
}




FeatureCmdInfoList CutUp::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "CutUp",

            "( <feature:base>, <vector:n>, <scalar:t>, <vector:p0> [, ..., <vector:pn>] )",

            "Cuts up the base feature into several pieces with planar cuts at p0 to pn with normal n and cut thickness t."
            " The result pieces are stored in subshapes of name \"cut_<int:i>\"."
        )
    );
}




} // namespace cad
} // namespace insight
