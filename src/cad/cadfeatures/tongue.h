#ifndef INSIGHT_CAD_TONGUE_H
#define INSIGHT_CAD_TONGUE_H

#include "cadfeature.h"

namespace insight {
namespace cad {

class Tongue
: public Feature
{

  FeaturePtr spine_;
  VectorPtr direction_, insidePt_;
  ScalarPtr t_, w_, ovl_, delta_;

  size_t calcHash() const override;
  void build() override;

  Tongue(
      FeaturePtr spine,
      VectorPtr direction,
      VectorPtr insidePt,
      ScalarPtr t,
      ScalarPtr w,
      ScalarPtr ovl,
      ScalarPtr delta
    );

public:
  declareType("Tongue");
  CREATE_FUNCTION(Tongue);

  static void insertrule(parser::ISCADParser& ruleset);
  static FeatureCmdInfoList ruleDocumentation();
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_TONGUE_H
