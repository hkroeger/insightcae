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

  virtual size_t calcHash() const;
  virtual void build();

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
  Tongue();

  static FeaturePtr create(
      FeaturePtr spine,
      VectorPtr direction,
      VectorPtr insidePt,
      ScalarPtr t,
      ScalarPtr w,
      ScalarPtr ovl,
      ScalarPtr delta
  );


  virtual void insertrule(parser::ISCADParser& ruleset) const;
  virtual FeatureCmdInfoList ruleDocumentation() const;
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_TONGUE_H
