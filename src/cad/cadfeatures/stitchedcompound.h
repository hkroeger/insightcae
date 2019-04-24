#ifndef STITCHEDCOMPOUND_H
#define STITCHEDCOMPOUND_H

#include "cadfeature.h"

namespace insight {
namespace cad {


class StitchedCompound
: public Feature
{
  FeatureSetPtr faces_;
  ScalarPtr tol_;

  virtual size_t calcHash() const;
  virtual void build();

public:
  declareType("StitchedCompound");
  StitchedCompound();
  StitchedCompound(FeatureSetPtr faces, ScalarPtr tol=scalarconst(1e-3) );

  virtual FeatureCmdInfoList ruleDocumentation() const;
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

}
}

#endif // STITCHEDCOMPOUND_H
