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

  Tongue(const Tongue&o, TreeCloneMap& tcm);
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
#ifndef SWIG
  DEPENDS((spine_,direction_, insidePt_,t_, w_, ovl_, delta_));
#endif
  CREATE_FUNCTION(Tongue);
  CLONEABLE(Tongue);

  static void insertrule(parser::ISCADParser& ruleset);
  static FeatureCmdInfoList ruleDocumentation();
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_TONGUE_H
