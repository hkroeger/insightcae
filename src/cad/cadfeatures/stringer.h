#ifndef INSIGHT_CAD_STRINGER_H
#define INSIGHT_CAD_STRINGER_H

#include "cadparameters.h"
#include "cadfeature.h"


namespace insight {
namespace cad {

class Stringer
    : public Feature
{

  FeaturePtr spine_;
  VectorPtr normal_;
  ScalarPtr t_, w_, delta_, ext0_, ext1_;

  size_t calcHash() const override;
  void build() override;

  Stringer(
      FeaturePtr spine,
      VectorPtr normal,
      ScalarPtr t,
      ScalarPtr w,
      ScalarPtr delta,
      ScalarPtr ext0,
      ScalarPtr ext1
    );

public:
  declareType("Stringer");
  CREATE_FUNCTION(Stringer);

  static void insertrule(parser::ISCADParser& ruleset);
  static FeatureCmdInfoList ruleDocumentation();
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_STRINGER_H
