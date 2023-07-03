#include "stitchedcompound.h"

#include "occinclude.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/tools.h"
#include "base/translations.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(StitchedCompound);
//addToFactoryTable(Feature, StitchedCompound);
addToStaticFunctionTable(Feature, StitchedCompound, insertrule);
addToStaticFunctionTable(Feature, StitchedCompound, ruleDocumentation);


size_t StitchedCompound::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*faces_;
  h+=tol_->value();
  return h.getHash();
}


StitchedCompound::StitchedCompound(FeatureSetPtr faces, ScalarPtr tol)
:faces_(faces), tol_(tol)
{}

void StitchedCompound::build()
{
  ExecTimer t("StitchedCompound::build() ["+featureSymbolName()+"]");

  BRepBuilderAPI_Sewing sew(tol_->value());

//   TopoDS_Compound aRes;
//   BRep_Builder aBuilder;
//   aBuilder.MakeCompound(aRes);

  for (const FeatureID& fi: faces_->data())
  {
    sew.Add(faces_->model()->face(fi));
//     aBuilder.Add(aRes, bladeFace_[s]);
  }

  sew.Perform();
  sew.Dump();

  //TopoDS_Shell sshell = TopoDS::Shell(sew.SewedShape());
//   BRepCheck_Shell acheck(sshell);


  setShape(/*sshell*/sew.SewedShape());
}

void StitchedCompound::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "StitchedCompound",
    std::make_shared<parser::ISCADParser::ModelstepRule>(

    ( '(' >> ruleset.r_faceFeaturesExpression  >> ( (',' >> ruleset.r_scalarExpression) | qi::attr(scalarconst(1e-3)) ) >> ')' )
                  [ qi::_val = phx::bind(
                       &StitchedCompound::create<FeatureSetPtr, ScalarPtr>,
                       qi::_1, qi::_2) ]

    )
  );
}

FeatureCmdInfoList StitchedCompound::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "StitchedCompound",

            "( <faceSelection> [ <scalar:tol> | 0.001 ] )",

          _("Create stitched shell from selected faces.")
        )
    };
}

}
}

