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

  virtual size_t calcHash() const;
  virtual void build();

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
  Stringer();

  static FeaturePtr create(
      FeaturePtr spine,
      VectorPtr normal,
      ScalarPtr t,
      ScalarPtr w,
      ScalarPtr delta,
      ScalarPtr ext0,
      ScalarPtr ext1
  );


  virtual void insertrule(parser::ISCADParser& ruleset) const;
  virtual FeatureCmdInfoList ruleDocumentation() const;
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_STRINGER_H
